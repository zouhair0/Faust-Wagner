
#ifndef _COMPILE_WAGNER_
#define _COMPILE_WAGNER_

#include <utility>
#include <set>
#include "compile.hh"
#include "sigtyperules.hh"
#include "sigraterules.hh"
// #include "occurences.hh"
// #include "property.hh"
// #include "xtended.hh"





////////////////////////////////////////////////////////////////////////
/**
 * Compile a list of FAUST signals into a Wagner Code
 */
///////////////////////////////////////////////////////////////////////




class WagnerCompiler:public Compiler
{

protected:RateInferrer * fRates;
  bool fHasIota;

public:

    WagnerCompiler (const string & name, const string & super, int numInputs,
		    int numOutputs):Compiler (name, super, numInputs,
					      numOutputs, false),
    fHasIota (false), fRates (0)
  {
  }

  virtual void compileMultiSignal (Tree lsig);
  virtual void compileSingleSignal (Tree lsig);

protected:

  Tree prepareWagner (Tree L0);
  Tree prepare (Tree L0);

  Tree prepare2 (Tree L0);


};

#endif
