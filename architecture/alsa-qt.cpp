/* link with : "" */
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <errno.h>
#include <time.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <alsa/asoundlib.h>
#include <pwd.h>
#include <sys/types.h>
#include <assert.h>
#include <pthread.h> 
#include <sys/wait.h>
#include <list>

#include <iostream>
#include <fstream>

#include "faustqt.h"


#define check_error(err) if (err) { printf("%s:%d, alsa error %d : %s\n", __FILE__, __LINE__, err, snd_strerror(err)); exit(1); }
#define check_error_msg(err,msg) if (err) { fprintf(stderr, "%s:%d, %s : %s(%d)\n", __FILE__, __LINE__, msg, snd_strerror(err), err); exit(1); }
#define display_error_msg(err,msg) if (err) { fprintf(stderr, "%s:%d, %s : %s(%d)\n", __FILE__, __LINE__, msg, snd_strerror(err), err); }

#define max(x,y) (((x)>(y)) ? (x) : (y))
#define min(x,y) (((x)<(y)) ? (x) : (y))

// abs is now predefined
//template<typename T> T abs (T a)	{ return (a<T(0)) ? -a : a; }


//inline int lsr (int x, int n)		{ return int(((unsigned int)x) >> n); }
//inline int int2pow2 (int x)		{ int r=0; while ((1<<r)<x) r++; return r; }


/**
 * Used to set the priority and scheduling of the audio thread 
 */
bool setRealtimePriority ()
{
    struct passwd *         pw;
    int                     err;
    uid_t                   uid;
    struct sched_param      param;  

    uid = getuid ();
    pw = getpwnam ("root");
    setuid (pw->pw_uid); 
    param.sched_priority = 98; /* 0 to 99  */
    err = sched_setscheduler(0, SCHED_RR, &param); 
    setuid (uid);
    return (err != -1);
}


/******************************************************************************
*******************************************************************************

							       VECTOR INTRINSICS

*******************************************************************************
*******************************************************************************/

inline void *aligned_calloc(size_t nmemb, size_t size) { return (void*)((unsigned)(calloc((nmemb*size)+15,sizeof(char)))+15 & 0xfffffff0); }


<<includeIntrinsic>>



/******************************************************************************
*******************************************************************************

								AUDIO INTERFACE

*******************************************************************************
*******************************************************************************/

enum { kRead = 1, kWrite = 2, kReadWrite = 3 };

/**
 * A convenient class to pass parameters to AudioInterface
 */
class AudioParam
{
  public:
			
	const char*		fCardName;					
	unsigned int	fFrequency;
	int				fBuffering; 
	
	unsigned int	fSoftInputs;
	unsigned int	fSoftOutputs;
	
  public :
	AudioParam() : 
		fCardName("hw:0"),
		fFrequency(44100),
		fBuffering(512),
		fSoftInputs(2),
		fSoftOutputs(2)
	{}
	
	AudioParam&	cardName(const char* n)	{ fCardName = n; 		return *this; }
	AudioParam&	frequency(int f)		{ fFrequency = f; 		return *this; }
	AudioParam&	buffering(int fpb)		{ fBuffering = fpb; 	return *this; }
	AudioParam&	inputs(int n)			{ fSoftInputs = n; 		return *this; }
	AudioParam&	outputs(int n)			{ fSoftOutputs = n; 	return *this; }
};


/**
 * An ALSA audio interface
 */
class AudioInterface : public AudioParam
{
 public :
	snd_pcm_t*				fOutputDevice ;		
	snd_pcm_t*				fInputDevice ;			
	snd_pcm_hw_params_t* 	fInputParams;
	snd_pcm_hw_params_t* 	fOutputParams;
	
	snd_pcm_format_t 		fSampleFormat;
	snd_pcm_access_t 		fSampleAccess;
	
	unsigned int			fCardInputs;
	unsigned int			fCardOutputs;
	
	unsigned int			fChanInputs;
	unsigned int			fChanOutputs;
	
	// interleaved mode audiocard buffers
	void*		fInputCardBuffer;
	void*		fOutputCardBuffer;
	
	// non interleaved mode audiocard buffers
	void*		fInputCardChannels[256];
	void*		fOutputCardChannels[256];
	
	// non interleaved mod, floating point software buffers
	float*		fInputSoftChannels[256];
	float*		fOutputSoftChannels[256];

 public :
 
	const char*	cardName()				{ return fCardName;  	}
 	int			frequency()				{ return fFrequency; 	}
	int			buffering()				{ return fBuffering;  	}
	
	float**		inputSoftChannels()		{ return fInputSoftChannels;	}
	float**		outputSoftChannels()	{ return fOutputSoftChannels;	}

	
	AudioInterface(const AudioParam& ap = AudioParam()) : AudioParam(ap)
	{
		
		fInputDevice 			= 0;
		fOutputDevice 			= 0;
		fInputParams			= 0;
		fOutputParams			= 0;
	}
	
	/**
	 * Open the audio interface
	 */
	void open()
	{
		int err;
		
		// Open the input and output audio streams
		err = snd_pcm_open( &fInputDevice,  fCardName, SND_PCM_STREAM_CAPTURE, 0 ); 	check_error(err)
		err = snd_pcm_open( &fOutputDevice, fCardName, SND_PCM_STREAM_PLAYBACK, 0 ); 	check_error(err)

		// search for optimal input parameters
		err = snd_pcm_hw_params_malloc	( &fInputParams ); 	check_error(err);
		setAudioParams(fInputDevice, fInputParams);
		snd_pcm_hw_params_get_channels(fInputParams, &fCardInputs);

		// search for optimal output parameters
		err = snd_pcm_hw_params_malloc	( &fOutputParams ); 		check_error(err)
		setAudioParams(fOutputDevice, fOutputParams);
		snd_pcm_hw_params_get_channels(fOutputParams, &fCardOutputs);

		printf("inputs : %ud, outputs : %ud\n", fCardInputs, fCardOutputs);

		// setup input-output parameters
		err = snd_pcm_hw_params (fInputDevice,  fInputParams );	 	check_error (err);
		err = snd_pcm_hw_params (fOutputDevice, fOutputParams );	check_error (err);

		//assert(snd_pcm_hw_params_get_period_size(fInputParams,NULL) == snd_pcm_hw_params_get_period_size(fOutputParams,NULL));

		// allocation of alsa buffers
		if (fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED) {
			fInputCardBuffer = aligned_calloc(interleavedBufferSize(fInputParams), 1);
	 		fOutputCardBuffer = aligned_calloc(interleavedBufferSize(fOutputParams), 1);
			
		} else {
			for (unsigned int i = 0; i < fCardInputs; i++) {
				fInputCardChannels[i] = aligned_calloc(noninterleavedBufferSize(fInputParams), 1);
			}
			for (unsigned int i = 0; i < fCardOutputs; i++) {
				fOutputCardChannels[i] = aligned_calloc(noninterleavedBufferSize(fOutputParams), 1);
			}
			
		}
		
		// allocation of floating point buffers needed by the dsp code
		
		fChanInputs = max(fSoftInputs, fCardInputs);		assert (fChanInputs < 256);
		fChanOutputs = max(fSoftOutputs, fCardOutputs);		assert (fChanOutputs < 256);

		for (unsigned int i = 0; i < fChanInputs; i++) {
			fInputSoftChannels[i] = (float*) aligned_calloc (fBuffering, sizeof(float));
			for (int j = 0; j < fBuffering; j++) {
				fInputSoftChannels[i][j] = 0.0;
			}
		}

		for (unsigned int i = 0; i < fChanOutputs; i++) {
			fOutputSoftChannels[i] = (float*) aligned_calloc (fBuffering, sizeof(float));
			for (int j = 0; j < fBuffering; j++) {
				fOutputSoftChannels[i][j] = 0.0;
			}
		}


	}
	
	
	void setAudioParams(snd_pcm_t* stream, snd_pcm_hw_params_t* params)
	{	
		int	err;

		// set params record with initial values
		err = snd_pcm_hw_params_any	( stream, params ); 	
		check_error_msg(err, "unable to init parameters")

		// set alsa access mode (and fSampleAccess field) either to non interleaved or interleaved
				
		err = snd_pcm_hw_params_set_access (stream, params, SND_PCM_ACCESS_RW_NONINTERLEAVED );
		if (err) {
			err = snd_pcm_hw_params_set_access (stream, params, SND_PCM_ACCESS_RW_INTERLEAVED );
			check_error_msg(err, "unable to set access mode neither to non-interleaved or to interleaved");
		}
		snd_pcm_hw_params_get_access(params, &fSampleAccess);
		

		// search for 32-bits or 16-bits format
		err = snd_pcm_hw_params_set_format (stream, params, SND_PCM_FORMAT_S32);
		if (err) {
			err = snd_pcm_hw_params_set_format (stream, params, SND_PCM_FORMAT_S16);
		 	check_error_msg(err, "unable to set format to either 32-bits or 16-bits");
		}
		snd_pcm_hw_params_get_format(params, &fSampleFormat);
		// set sample frequency
		snd_pcm_hw_params_set_rate_near (stream, params, &fFrequency, 0); 

		// set period and period size (buffering)
		err = snd_pcm_hw_params_set_period_size	(stream, params, fBuffering, 0); 	
		check_error_msg(err, "period size not available");
		
		err = snd_pcm_hw_params_set_periods (stream, params, 2, 0); 			
		check_error_msg(err, "number of periods not available");

	}


	ssize_t interleavedBufferSize (snd_pcm_hw_params_t* params)
	{
		_snd_pcm_format 	format;  	snd_pcm_hw_params_get_format(params, &format);
		snd_pcm_uframes_t 	psize;		snd_pcm_hw_params_get_period_size(params, &psize, NULL);
		unsigned int 		channels; 	snd_pcm_hw_params_get_channels(params, &channels);
		ssize_t bsize = snd_pcm_format_size (format, psize * channels);
		return bsize;
	}


	ssize_t noninterleavedBufferSize (snd_pcm_hw_params_t* params)
	{
		_snd_pcm_format 	format;  	snd_pcm_hw_params_get_format(params, &format);
		snd_pcm_uframes_t 	psize;		snd_pcm_hw_params_get_period_size(params, &psize, NULL);
		ssize_t bsize = snd_pcm_format_size (format, psize);
		return bsize;
	}


	void close()
	{
	}



	/**
	 * Read audio samples from the audio card. Convert samples to floats and take 
	 * care of interleaved buffers
	 */
	void read()
	{
		
		if (fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED) {
			
			int count = snd_pcm_readi(fInputDevice, fInputCardBuffer, fBuffering); 	
			if (count<0) { 
				display_error_msg(count, "reading samples");
				 int err = snd_pcm_prepare(fInputDevice);	
				 check_error_msg(err, "preparing input stream");
			}
			
			if (fSampleFormat == SND_PCM_FORMAT_S16) {

				short* 	buffer16b = (short*) fInputCardBuffer;
				for (int s = 0; s < fBuffering; s++) {
					for (unsigned int c = 0; c < fCardInputs; c++) {
						fInputSoftChannels[c][s] = float(buffer16b[c + s*fCardInputs])*(1.0/float(SHRT_MAX));
					}
				}

			} else { // SND_PCM_FORMAT_S32

				long* 	buffer32b = (long*) fInputCardBuffer;
				for (int s = 0; s < fBuffering; s++) {
					for (unsigned int c = 0; c < fCardInputs; c++) {
						fInputSoftChannels[c][s] = float(buffer32b[c + s*fCardInputs])*(1.0/float(LONG_MAX));
					}
				}
			}
			
		} else if (fSampleAccess == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
			
			int count = snd_pcm_readn(fInputDevice, fInputCardChannels, fBuffering); 	
			if (count<0) { 
				display_error_msg(count, "reading samples");
				 int err = snd_pcm_prepare(fInputDevice);	
				 check_error_msg(err, "preparing input stream");
			}
			
			if (fSampleFormat == SND_PCM_FORMAT_S16) {

				for (unsigned int c = 0; c < fCardInputs; c++) {
					short* 	chan16b = (short*) fInputCardChannels[c];
					for (int s = 0; s < fBuffering; s++) {
						fInputSoftChannels[c][s] = float(chan16b[s])*(1.0/float(SHRT_MAX));
					}
				}

			} else { // SND_PCM_FORMAT_S32

				for (unsigned int c = 0; c < fCardInputs; c++) {
					long* 	chan32b = (long*) fInputCardChannels[c];
					for (int s = 0; s < fBuffering; s++) {
						fInputSoftChannels[c][s] = float(chan32b[s])*(1.0/float(LONG_MAX));
					}
				}
			}
			
		} else {
			check_error_msg(-10000, "unknow access mode");
		}


	}


	/**
	 * write the output soft channels to the audio card. Convert sample 
	 * format and interleaves buffers when needed
	 */
	void write()
	{
		recovery :
				
		if (fSampleAccess == SND_PCM_ACCESS_RW_INTERLEAVED) {
			
			if (fSampleFormat == SND_PCM_FORMAT_S16) {

				short* buffer16b = (short*) fOutputCardBuffer;
				for (int f = 0; f < fBuffering; f++) {
					for (unsigned int c = 0; c < fCardOutputs; c++) {
						float x = fOutputSoftChannels[c][f];
						buffer16b[c + f*fCardOutputs] = short( max(min(x,1.0),-1.0) * float(SHRT_MAX) ) ;
					}
				}

			} else { // SND_PCM_FORMAT_S32

				long* buffer32b = (long*) fOutputCardBuffer;
				for (int f = 0; f < fBuffering; f++) {
					for (unsigned int c = 0; c < fCardOutputs; c++) {
						float x = fOutputSoftChannels[c][f];
						buffer32b[c + f*fCardOutputs] = long( max(min(x,1.0),-1.0) * float(LONG_MAX) ) ;
					}
				}
			}

			int count = snd_pcm_writei(fOutputDevice, fOutputCardBuffer, fBuffering); 	
			if (count<0) { 
				display_error_msg(count, "w3"); 
				int err = snd_pcm_prepare(fOutputDevice);	
				check_error_msg(err, "preparing output stream");
				goto recovery;
			}
			
			
		} else if (fSampleAccess == SND_PCM_ACCESS_RW_NONINTERLEAVED) {
			
			if (fSampleFormat == SND_PCM_FORMAT_S16) {

				for (unsigned int c = 0; c < fCardOutputs; c++) {
					short* chan16b = (short*) fOutputCardChannels[c];
					for (int f = 0; f < fBuffering; f++) {
						float x = fOutputSoftChannels[c][f];
						chan16b[f] = short( max(min(x,1.0),-1.0) * float(SHRT_MAX) ) ;
					}
				}

			} else { // SND_PCM_FORMAT_S32

				for (unsigned int c = 0; c < fCardOutputs; c++) {
					long* chan32b = (long*) fOutputCardChannels[c];
					for (int f = 0; f < fBuffering; f++) {
						float x = fOutputSoftChannels[c][f];
						chan32b[f] = long( max(min(x,1.0),-1.0) * float(LONG_MAX) ) ;
					}
				}
			}

			int count = snd_pcm_writen(fOutputDevice, fOutputCardChannels, fBuffering); 	
			if (count<0) { 
				display_error_msg(count, "w3"); 
				int err = snd_pcm_prepare(fOutputDevice);	
				check_error_msg(err, "preparing output stream");
				goto recovery;
			}
			
		} else {
			check_error_msg(-10000, "unknow access mode");
		}
	}


	
	/**
	 *  print short information on the audio device
	 */
	void shortinfo()
	{
		int						err;
		snd_ctl_card_info_t*	card_info;
    	snd_ctl_t*				ctl_handle;
		err = snd_ctl_open (&ctl_handle, fCardName, 0);		check_error(err);
		snd_ctl_card_info_alloca (&card_info);
		err = snd_ctl_card_info(ctl_handle, card_info);		check_error(err);
		printf("%s|%d|%d|%d|%d|%s\n", 
				snd_ctl_card_info_get_driver(card_info),
				fCardInputs, fCardOutputs,
				fFrequency, fBuffering,
				snd_pcm_format_name((_snd_pcm_format)fSampleFormat));
	}
					
	/**
	 *  print more detailled information on the audio device
	 */
	void longinfo()
	{
		int						err;
		snd_ctl_card_info_t*	card_info;
    	snd_ctl_t*				ctl_handle;

		printf("Audio Interface Description :\n");
		printf("Sampling Frequency : %d, Sample Format : %s, buffering : %d\n", 
				fFrequency, snd_pcm_format_name((_snd_pcm_format)fSampleFormat), fBuffering);
		printf("Software inputs : %2d, Software outputs : %2d\n", fSoftInputs, fSoftOutputs);
		printf("Hardware inputs : %2d, Hardware outputs : %2d\n", fCardInputs, fCardOutputs);
		printf("Channel inputs  : %2d, Channel outputs  : %2d\n", fChanInputs, fChanOutputs);
		
		// affichage des infos de la carte
		err = snd_ctl_open (&ctl_handle, fCardName, 0);		check_error(err);
		snd_ctl_card_info_alloca (&card_info);
		err = snd_ctl_card_info(ctl_handle, card_info);		check_error(err);
		printCardInfo(card_info);

		// affichage des infos liees aux streams d'entree-sortie
		if (fSoftInputs > 0)	printHWParams(fInputParams);
		if (fSoftOutputs > 0)	printHWParams(fOutputParams);
	}
	
	void printCardInfo(snd_ctl_card_info_t*	ci)
	{
		printf("Card info (address : %p)\n", ci);
		printf("\tID         = %s\n", snd_ctl_card_info_get_id(ci));
		printf("\tDriver     = %s\n", snd_ctl_card_info_get_driver(ci));
		printf("\tName       = %s\n", snd_ctl_card_info_get_name(ci));
		printf("\tLongName   = %s\n", snd_ctl_card_info_get_longname(ci));
		printf("\tMixerName  = %s\n", snd_ctl_card_info_get_mixername(ci));
		printf("\tComponents = %s\n", snd_ctl_card_info_get_components(ci));
		printf("--------------\n");
	}

	void printHWParams( snd_pcm_hw_params_t* params )
	{
		printf("HW Params info (address : %p)\n", params);
#if 0
		printf("\tChannels    = %d\n", snd_pcm_hw_params_get_channels(params));
		printf("\tFormat      = %s\n", snd_pcm_format_name((_snd_pcm_format)snd_pcm_hw_params_get_format(params)));
		printf("\tAccess      = %s\n", snd_pcm_access_name((_snd_pcm_access)snd_pcm_hw_params_get_access(params)));
		printf("\tRate        = %d\n", snd_pcm_hw_params_get_rate(params, NULL));
		printf("\tPeriods     = %d\n", snd_pcm_hw_params_get_periods(params, NULL));
		printf("\tPeriod size = %d\n", (int)snd_pcm_hw_params_get_period_size(params, NULL));
		printf("\tPeriod time = %d\n", snd_pcm_hw_params_get_period_time(params, NULL));
		printf("\tBuffer size = %d\n", (int)snd_pcm_hw_params_get_buffer_size(params));
		printf("\tBuffer time = %d\n", snd_pcm_hw_params_get_buffer_time(params, NULL));
#endif
		printf("--------------\n");
	}

	
};



/******************************************************************************
*******************************************************************************

								DSP

*******************************************************************************
*******************************************************************************/


/**
 *  Abstract definition of a Faust Digital Signal Processor
 */
			
class dsp {
 protected:
	int fSamplingFreq;
 public:
	dsp() {}
	virtual ~dsp() {}
	
	virtual int getNumInputs() 										= 0;
	virtual int getNumOutputs() 									= 0;
	virtual void buildUserInterface(UI* interface) 					= 0;
	virtual void init(int samplingRate) 							= 0;
 	virtual void compute(int len, float** inputs, float** outputs) 	= 0;
 	virtual void conclude() 										{}
};
		
		
<<includeclass>>

						
mydsp	DSP;




/******************************************************************************
*******************************************************************************

								COMMAND LINE PARAMETERS

*******************************************************************************
*******************************************************************************/
	
///< lopt : Scan Command Line long int Arguments
long lopt (int argc, char *argv[], char* longname, char* shortname, long def) 
{
	for (int i=2; i<argc; i++) 
		if ( strcmp(argv[i-1], shortname) == 0 || strcmp(argv[i-1], longname) == 0 ) 
			return atoi(argv[i]);
	return def;
}
	
///< sopt : Scan Command Line string Arguments
char* sopt (int argc, char *argv[], char* longname, char* shortname, char* def) 
{
	for (int i=2; i<argc; i++) 
		if ( strcmp(argv[i-1], shortname) == 0 || strcmp(argv[i-1], longname) == 0 ) 
			return argv[i];
	return def;
}
	
///< fopt : Scan Command Line flag option (without argument), return true if the flag
bool fopt (int argc, char *argv[], char* longname, char* shortname) 
{
	for (int i=1; i<argc; i++) 
		if ( strcmp(argv[i], shortname) == 0 || strcmp(argv[i], longname) == 0 ) 
			return true;
	return false;
}
	


/******************************************************************************
*******************************************************************************

								GLOBAL VARIABLES

*******************************************************************************
*******************************************************************************/
	
list<UI*> 	UI::fGuiList;

UI* 		interface;

/******************************************************************************
*******************************************************************************

								AUDIO (ALSA) THREAD

*******************************************************************************
*******************************************************************************/
	
pthread_t	audiothread;

/**
 * This function will be run in a separate high priority audio thread. 
 * It reads, computes and writes audio samples in a loop until the 
 * user interface is stopped
 */
void* run_audio(void* ptr)
{
	AudioInterface*	audio = (AudioInterface*) ptr;

	bool rt = setRealtimePriority();
	printf(rt?"RT : ":"NRT: "); audio->shortinfo();
	
	audio->write();
	audio->write();
	while(!interface->stopped()) {
		audio->read();
		DSP.compute(audio->buffering(), audio->inputSoftChannels(), audio->outputSoftChannels());
		audio->write();
	} 

	pthread_exit(0);
  	return 0;
}



/******************************************************************************
*******************************************************************************

									MAIN (ALSA+QT)

*******************************************************************************
*******************************************************************************/
	
/**
 * Creates a User Interface, an Audio Interface and connect them with a Faust DSP.
 * The Audio loop is in a specific high priority audio thread. The GUI loop
 * is called in the main. The user
 */
int main(int argc, char *argv[] )
{
	interface = new QTGUI(argc, argv);
	
	// compute rcfilename to (re)store application state
	char	rcfilename[256];
	char* 	home = getenv("HOME");
	snprintf(rcfilename, 255, "%s/.%src", home, basename(argv[0]));
	
	AudioInterface	audio (
		AudioParam().cardName( sopt(argc, argv, "--device", "-d", "hw:0") ) 
					.frequency( lopt(argc, argv, "--frequency", "-f", 44100) ) 
					.buffering( lopt(argc, argv, "--buffer", "-b", 1024) )
					.inputs(DSP.getNumInputs())
					.outputs(DSP.getNumOutputs())
	);

	
	audio.open();
	
	DSP.init(audio.frequency());
	DSP.buildUserInterface(interface);
	
	interface->recallState(rcfilename);
	if (fopt(argc, argv, "--verbose", "-v")) audio.longinfo();

	pthread_create(&audiothread, NULL, run_audio, &audio);
	
	interface->run();
	interface->saveState(rcfilename);

  	return 0;
}