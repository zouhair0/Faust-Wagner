/************************************************************************

	IMPORTANT NOTE : this file contains two clearly delimited sections :
	the ARCHITECTURE section (in two parts) and the USER section. Each section
	is governed by its own copyright and license. Please check individually
	each section for license and copyright information.
*************************************************************************/

/*******************BEGIN ARCHITECTURE SECTION (part 1/2)****************/

/************************************************************************
    FAUST Architecture File
	Copyright (C) 2003-2011 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This Architecture section is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
	as published by the Free Software Foundation; either version 3 of
	the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
	along with this program; If not, see <http://www.gnu.org/licenses/>.

	EXCEPTION : As a special exception, you may create a larger work
	that contains this FAUST architecture section and distribute
	that work under terms of your choice, so long as this FAUST
	architecture section is not modified.


 ************************************************************************
 ************************************************************************/

#ifndef __netjack_dsp__
#define __netjack_dsp__

#include "audio.h"
#include "dsp.h"
#include <jack/net.h>

class netjackaudio : public audio {


        dsp* fDsp;
        jack_net_slave_t* fNet;

        static void net_shutdown(void *)
        {
            exit(1);
        }

        static int net_process(jack_nframes_t buffer_size,
                        int,
                        float** audio_input_buffer,
                        int,
                        void**,
                        int,
                        float** audio_output_buffer,
                        int,
                        void**,
                        void* data)
        {
            AVOIDDENORMALS;
            netjackaudio* obj = (netjackaudio*)data;
            obj->fDsp->compute(buffer_size, audio_input_buffer, audio_output_buffer);
            return 0;
        }

    public:

        bool init(const char* name, dsp* DSP)
        {
            fDsp = DSP;
            jack_slave_t request = { DSP->getNumInputs(), DSP->getNumOutputs(), 0, 0, DEFAULT_MTU, -1, JackFloatEncoder, 0, JackSlowMode };
            jack_master_t result;

            if ((fNet = jack_net_slave_open(DEFAULT_MULTICAST_IP, DEFAULT_PORT, name, &request, &result)) == 0) {
                printf("jack remote server not running ?\n");
                return false;
            }

            jack_set_net_slave_process_callback(fNet, net_process, this);
            jack_set_net_slave_shutdown_callback(fNet, net_shutdown, 0);

            fDsp->init(result.sample_rate);
            return true;
        }

        bool start()
        {
            if (jack_net_slave_activate(fNet)) {
                printf("cannot activate net");
                return false;
            }
            return true;
        }

        void stop()
        {
            jack_net_slave_deactivate(fNet);
            jack_net_slave_close(fNet);
        }

};


#endif


/********************END ARCHITECTURE SECTION (part 2/2)****************/
