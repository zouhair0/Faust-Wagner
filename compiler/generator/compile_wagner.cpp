/************************************************************************
 ************************************************************************
    FAUST compiler
	Copyright (C) 2003-2004 GRAME, Centre National de Creation Musicale
    ---------------------------------------------------------------------
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 ************************************************************************
 ************************************************************************/


#include "sigtyperules.hh"
#include "signals.hh"
#include "sigtype.hh"
#include "xtended.hh"
#include "tree.hh"
#include "node.hh"

//#include "binop.hh"
#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <set>
#include <deque>
#include <unordered_map>
#include <string>

#include "floats.hh"

#include "compile.hh"
#include "compile_wagner.hh"
#include "recursivness.hh"
#include "simplify.hh"
#include "privatise.hh"
#include "prim2.hh"
#include "xtended.hh"
#include "wExp.hh"

#include "compatibility.hh"
#include "ppsig.hh"
#include "sigToGraph.hh"
#include "Text.hh"
#include "ppbox.hh"


const char *binopn[] = {
  "+", "-", "*", "/", "%",
  "<<", ">>",
  ">", "<", ">=", "<=", "==", "!=",
  "&", "|", "^"
};

int binopp[] = {
  2, 2, 3, 3, 3,
  1, 1,
  1, 1, 1, 1, 1, 1,
  1, 1, 1
};

// Count of number of expressions
std::unordered_map < Tree, unsigned int >
ht (65543);

typedef std::unordered_map < Tree , wExp * > local_map;

// Cache of translation
std::deque < local_map * >hw;


wExp *
ToWagnerExp (Tree sig)
{
  int
    i;
  double
    r;
  Tree x, y, z, u, le, id, sel, c, label, ff, largs, type, name, file;
  xtended *
    p = (xtended *) getUserData (sig);

  ht[sig]++;

  wExp *ret;			// Return value

  //cerr << "Number of elements in the stack: " << hw.size () << endl;

  /*for(auto it = hw.begin(); it != hw.end(); ++it)
     {
     auto iter = it->find(sig);
     if(iter != it->end())
     {
     return(*iter).second;
     }
     *it++;
     } 

  std::deque<local_map*>::iterator it = hw.begin();
        local_map* it = *hw.begin();    
     while(it != *hw.end())
     {    
     auto iter = it->find(sig);
     if(iter != it->end())
     {
     return(*iter).second;
     }
     *it++;
     }*/


  //browse all the container

for (local_map * elem:hw)
    {
      auto iter = elem->find (sig);

      if (iter != elem->end ())
		{
		  //return (*iter).second;
	      return new wHash(sig,(*iter).second);
		  //break;
		} 
    }

// If there is no cash of translation we build a new value.
     
		if (isList (sig) && len (sig) == 1)
	    {
	      ret = ToWagnerExp (hd (sig));
	    }
	     else if (isList (sig) && len (sig) > 1)
	    {
	      std::vector < wExp * >vec;
	      Tree tsig = sig;
	      do
		{
		  vec.push_back (ToWagnerExp (hd (tsig)));
		  tsig = tl (tsig);
		}
	      while (isList (tsig));

	      ret = new wTuple (vec, len (sig));
	    }
		 else if (isProj (sig, &i, x))
	    {
	      ret = new wProj (ToWagnerExp (x), i);
	    }
	  else if (isRec (sig, x, le))
	    {
	      std::cerr << "Non Debruijn expression found!\n";
	      ret = new wFeed1 (ToWagnerExp (le), (string) tree2str (x));
	    }
	  //debruijn notation
	  else if (isRec (sig, le))
	    {
	      local_map *
		  H = new local_map (999);
	      hw.push_front (H);
	      wExp *e = ToWagnerExp (le);
	      hw.pop_front ();
	      ret = new wFeed (e, H);
	    }
	  else if (isRef (sig, i))
	    {
	      ret = new wRef (i);
	    }
	  else if (getUserData (sig))
	    {
	      int
		  a = sig->arity ();
	      std::vector < wExp * >vec;

	      for (int j = 0; j < a; j++)
		{
		  vec.push_back (ToWagnerExp (sig->branch (j)));
		}

	      ret = new wFun (p->name (), vec);
	    }
	  else if (isSigInt (sig, &i))
	    {
	      ret = new wInteger (i);
	    }
	  else if (isSigReal (sig, &r))
	    {
	      ret = new wDouble (r);
	    }
	  else if (isSigWaveform (sig))
	    {
	      ret = new wWaveform ("waveform{...}");
	    }
	  else if (isSigInput (sig, &i))
	    {
	      ret = new wVar1 (i);
	    }
	  else if (isSigOutput (sig, &i, x))
	    {
	      ret = new wVar2 (i, ToWagnerExp (x));
	    }
	  else if (isSigDelay1 (sig, x))
	    {
	      ret = new wDelay1 (ToWagnerExp (x));
	    }
	  else if (isSigFixDelay (sig, x, y))
	    {
	      ret = new wFixDelay (ToWagnerExp (x), ToWagnerExp (y));
	    }
	  else if (isSigPrefix (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("prefix", vec);
	    }
	  else if (isSigIota (sig, x))
	    {
	      std::vector < wExp * >vec (1);
	      vec[0] = ToWagnerExp (x);
	      ret = new wFun ("iota", vec);
	    }
	  else if (isSigFFun (sig, ff, largs))
	    {
	      std::vector < wExp * >vec (1);
	      vec[0] = ToWagnerExp (largs);

	      ret = new wFun (ffname (ff), vec);
	    }
	  else if (isSigBinOp (sig, &i, x, y))
	    {
	      ret =
		new wBinaryOp (ToWagnerExp (x), ToWagnerExp (y), binopn[i]);
	    }

	  else if (isSigFConst (sig, type, name, file))
	    {
	      ret = new wWaveform (tree2str (name));
	    }

	  else if (isSigFVar (sig, type, name, file))
	    {
	      ret = new wWaveform (tree2str (name));
	    }

	  else if (isSigTable (sig, id, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("table", vec);
	    }
	  else if (isSigWRTbl (sig, id, x, y, z))
	    {
	      std::vector < wExp * >vec (3);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      vec[2] = ToWagnerExp (z);
	      ret = new wFun ("write", vec);
	    }
	  else if (isSigRDTbl (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("read", vec);
	    }
	  else if (isSigGen (sig, x))
	    {
	      ret = ToWagnerExp (x);
	    }
	  else if (isSigDocConstantTbl (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("DocConstantTbl", vec);
	    }
	  else if (isSigDocWriteTbl (sig, x, y, z, u))
	    {
	      std::vector < wExp * >vec (4);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      vec[2] = ToWagnerExp (z);
	      vec[3] = ToWagnerExp (u);
	      ret = new wFun ("DocwriteTbl", vec);
	    }
	  else if (isSigDocAccessTbl (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("DocaccessTbl", vec);
	    }
	  else if (isSigSelect2 (sig, sel, x, y))
	    {
	      std::vector < wExp * >vec (3);
	      vec[0] = ToWagnerExp (sel);
	      vec[1] = ToWagnerExp (x);
	      vec[2] = ToWagnerExp (y);
	      ret = new wFun ("select2", vec);
	    }

	  else if (isSigSelect3 (sig, sel, x, y, z))
	    {
	      std::vector < wExp * >vec (4);
	      vec[0] = ToWagnerExp (sel);
	      vec[1] = ToWagnerExp (x);
	      vec[2] = ToWagnerExp (y);
	      vec[3] = ToWagnerExp (z);
	      ret = new wFun ("select3", vec);
	    }
	  else if (isSigIntCast (sig, x))
	    {
	      std::vector < wExp * >vec (1);
	      vec[0] = ToWagnerExp (x);
	      ret = new wFun ("int", vec);
	    }
	  else if (isSigFloatCast (sig, x))
	    {
	      std::vector < wExp * >vec (1);
	      vec[0] = ToWagnerExp (x);
	      ret = new wFun ("float", vec);
	    }
	  else if (isSigButton (sig, label))
	    {
	      ret = new wUi ("button", (string) tree2str (label));
	    }
	  else if (isSigCheckbox (sig, label))
	    {
	      ret = new wUi ("checkbox", (string) tree2str (label));
	    }
	  else if (isSigVSlider (sig, label, c, x, y, z))
	    {
	      std::vector < wExp * >vec (4);
	      vec[0] = ToWagnerExp (c);
	      vec[1] = ToWagnerExp (x);
	      vec[2] = ToWagnerExp (y);
	      vec[3] = ToWagnerExp (z);

	      ret = new wUi ("vslider", (string) tree2str (label), vec);
	    }
	  else if (isSigHSlider (sig, label, c, x, y, z))
	    {
	      std::vector < wExp * >vec (4);
	      vec[0] = ToWagnerExp (c);
	      vec[1] = ToWagnerExp (x);
	      vec[2] = ToWagnerExp (y);
	      vec[3] = ToWagnerExp (z);
	      ret = new wUi ("hslider", (string) tree2str (label), vec);
	    }
	  else if (isSigNumEntry (sig, label, c, x, y, z))
	    {
	      std::vector < wExp * >vec (4);
	      vec[0] = ToWagnerExp (c);
	      vec[1] = ToWagnerExp (x);
	      vec[2] = ToWagnerExp (y);
	      vec[3] = ToWagnerExp (z);
	      ret = new wUi ("nentry", (string) tree2str (label), vec);
	    }
	  else if (isSigVBargraph (sig, label, x, y, z))
	    {
	      std::vector < wExp * >vec (3);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      vec[2] = ToWagnerExp (z);
	      ret = new wUi ("vbargraph", (string) tree2str (label), vec);
	    }
	  else if (isSigHBargraph (sig, label, x, y, z))
	    {
	      std::vector < wExp * >vec (3);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      vec[2] = ToWagnerExp (z);
	      ret = new wUi ("hbargraph", (string) tree2str (label), vec);
	    }
	  else if (isSigAttach (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("attach", vec);
	    }
	  else if (isSigVectorize (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("vectorize", vec);
	    }
	  else if (isSigSerialize (sig, x))
	    {
	      std::vector < wExp * >vec (1);
	      vec[0] = ToWagnerExp (x);
	      ret = new wFun ("serialize", vec);
	    }
	  else if (isSigVectorAt (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("vectorAt", vec);
	    }
	  else if (isSigConcat (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);
	      ret = new wFun ("concat", vec);
	    }
	  else if (isSigUpSample (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);

	      ret = new wFun ("up", vec);
	    }
	  else if (isSigDownSample (sig, x, y))
	    {
	      std::vector < wExp * >vec (2);
	      vec[0] = ToWagnerExp (x);
	      vec[1] = ToWagnerExp (y);

	      ret = new wFun ("down", vec);
	    }
	  else
	    {
	      ret = new wError ((string) tree2str (sig));
	    }
		//add the value into the table
		std::pair < Tree, wExp * >pr (sig, ret);
	    hw.front ()->insert (pr);

	  return new wHash (sig, ret);

}
/*
void PrintsortedTable(local_map* hs)
{
	
}*/


void
WagnerCompiler::compileMultiSignal (Tree L)
{
  bool printw = true;
  local_map *initial_map = new local_map (988);
  hw.push_front (initial_map);
  std::map<Tree,wExp*> temp;

  wExp *wexp = ToWagnerExp (L);

  cerr <<
    "************************************************************************"
    << endl;

  cerr << "Number of elements in the deque: " << hw.size () << endl;

  for (auto it = hw.rbegin (); it != hw.rend (); ++it)
    {
      cerr << "Number of elements in the top map inside the deque : " <<
	hw.front ()->size () <<endl;

    for (std::pair<Tree,wExp*> elem : **it)
	{
	  temp.insert (elem);
	}
	for (std::pair<Tree,wExp*> e : temp)
	{
		cerr<< "["<<e.first<<"]"<<":"<< e.second<<endl;   
	}
	/*std::map<Tree,wExp*>::reverse_iterator rit;
	 for (rit = temp.rbegin (); rit != temp.rend (); ++rit)
	{
		cerr<< rit->first <<":"<< rit->second <<endl;
	}*/
	

    }
  cerr <<
    "************************************************************************"
    << endl;

  cerr << "Program is: " << endl << endl;

  if (printw)
    {
      cerr << wexp << endl;
    }

  exit (0);
}

void
WagnerCompiler::compileSingleSignal (Tree sig)
{
}

Tree
WagnerCompiler::prepare (Tree LS)
{
}

Tree
WagnerCompiler::prepare2 (Tree LS)
{
}
