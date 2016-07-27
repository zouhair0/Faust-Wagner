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


//using namespace std;

//extern bool gWagnerSwitch;

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

// Cache of translation
std::unordered_map < Tree, wExp * >hw (65543);

wExp *ToWagnerExp (Tree sig);

void
WagnerCompiler::compileMultiSignal (Tree L)
{
  bool printw = true;

  wExp *wexp = ToWagnerExp (L);

  cerr <<
    "************************************************************************"
    << endl;

  cerr << "Number of elements in the hash: " << ht.size () << endl << endl;

for (auto & elem:hw)
    {

      cerr << "[" << elem.first << "]: " << elem.second << endl;

    }

  cerr <<
    "************************************************************************"
    << endl;
  cerr << "Program is: " << endl << endl;

  if (printw)
    {
      //cerr << wexp->to_string () << endl;
      cerr << wexp << endl;
    }

  exit (0);
}

wExp *
ToWagnerExp (Tree sig)
{
  int i;
  double r;
  Tree x, y, z, u, le, id, sel, c, label, ff, largs, type, name, file;
  xtended *p = (xtended *) getUserData (sig);

  // Return value
  wExp *ret;

  ht[sig]++;

  // If sig is in hw, then return the cache value.
  auto iter = hw.find (sig);

  if (iter != hw.end ())
    {
      return new wHash ((void *) sig);
    }

  // Else we build a new value.
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
      //while (!isNil (tsig))
      //{
      //vec.push_back (ToWagnerExp (hd (tsig)));
      //tsig = tl (tsig);
      //}
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
      ret = new wFeed (ToWagnerExp (le));
    }
  else if (isRef (sig, i))
    {
      ret = new wRef (i);
    }
  else if (getUserData (sig))
    {
      int a = sig->arity ();
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
      ret = new wBinaryOp (ToWagnerExp (x), ToWagnerExp (y), binopn[i]);
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

  // Adjouter la valeur:
  hw[sig] = ret;

  return ret;
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


/* Ancien to Wagner

string
ToWagner (Tree sig, int prec)
{
  int i;
  double r;
  string s;
  ostringstream oss;
  Tree x, y, z, u, le, id;
  xtended *p = (xtended *) getUserData (sig);
  if (p)
    {
      s = p->name ();
      s = s + "(";
      int v = sig->arity ();
      for (int j = 0; j < v; j++)
	{
	  s = s + ToWagner (sig->branch (j), 0);
	  if (j != v - 1)
	    s = s + ",";
	}
      s = s + ")";
      return s;
    }
  if (isSigInt (sig, &i))
    {
      oss << i;
      s = oss.str ();
      return s;
    }

  else if (isSigReal (sig, &r))
    {
      oss << r;
      s = oss.str ();
      return s;
    }
  else if (isSigInput (sig, &i))
    {
      oss << i;
      s = oss.str ();
      return "IN" + s;
    }
  else if (isSigOutput (sig, &i, x))
    {
      oss << i;
      s = oss.str ();
      s = "OUT" + s + ":=";
      s = s + ToWagner (x, 0);
      return s;
    }
  else if (isSigBinOp (sig, &i, x, y))
    {
      if (prec > binopp[i])
	s = "(";
      s = s + ToWagner (x, binopp[i]);
      s = s + binopn[i];
      s = s + ToWagner (y, binopp[i]);
      if (prec > binopp[i])
	s = s + ")";
      return s;
    }
  else if (isSigDelay1 (sig, x))
    {
      s = "mem(";
      s = s + ToWagner (x, 0);
      s = s + ")";
      return s;
    }
  else if (isSigPrefix (sig, x, y))
    {
      s = "prefix(";
      s = s + ToWagner (x, 0);
      s = s + ",";
      s = s + ToWagner (y, 0);
      s = s + ")";
      return s;
    }
  else if (isSigAttach (sig, x, y))
    {
      s = "attach(";
      s = s + ToWagner (x, 0);
      s = s + ",";
      s = s + ToWagner (y, 0);
      s = s + ")";
      return s;
    }
  else if (isSigFixDelay (sig, x, y))
    {
      if (prec > 4)
	s = "(";
      s = s + ToWagner (x, 4);
      if (isSigInt (y, &i) && i == 0)
	{
	}
      else
	{
	  s = s + "@";
	  s = s + ToWagner (y, 4);
	}
      if (prec > 4)
	s = s + ")";
      return s;
    }
  else if (isProj (sig, &i, x))
    {
      s = "P";
      oss << i;
      s = s + oss.str () + " ";
      s = s + ToWagner (x, prec);
      return s;
    }
  else if (isRef (sig, i))
    {
      s = "$";
      oss << i;
      s = s + oss.str ();
      return s;
    }
  // This has to be fixed like in ppsig using an enviroment.
  // else if (isRef (sig, x))
  //   {
  //     s = tree2str (x);
  //     static Tree seen = NULL;
  //     if (seen == NULL)
  //       {
  //         seen = tree (symbol ("wagner"));
  //       }
  //     if (sig->getProperty (seen) == NULL)
  //       {
  //         sig->setProperty (seen, seen);
  //         recids = tree (Node ((void *) sig), recids);
  //       }
  //     return s;
  //   }


  else if (isSigTable (sig, id, x, y))
    {
      s = "table(";
      s = s + ToWagner (x, 0);
      s = s + ",";
      s = s + ToWagner (y, 0);
      s = s + ")";
      return s;
    }
  else if (isSigWRTbl (sig, id, x, y, z))
    {
      s = ToWagner (x, 0);
      s = s + "[";
      s = s + ToWagner (y, 0);
      s = s + "] := (";
      s = s + ToWagner (z, 0);
      s = s + ")";
      return s;
    }
  else if (isList (sig))
    {
      int k = 0;
      int l = len (sig);
      string sep = "(";
      do
	{
	  k++;
	  s = s + sep;
	  s = s + ToWagner (hd (sig), 0);
	  sig = tl (sig);
	  if (k == l - 1)
	    {
	      sep = ",";
	    }
	  else
	    sep = ",(";
	}
      while (isList (sig));
      if (k == 1)
	{
	  s = s + ")";
	}
      else
	{
	  for (int j = 0; j < k - 1; j++)
	    {
	      s = s + ")";
	    }
	}
      return s;
    }
  else
    return "--" + (string) tree2str (sig) + "||";
}

*/
