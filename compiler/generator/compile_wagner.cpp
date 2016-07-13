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


#include <stdio.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <math.h>
#include <set>

#include "floats.hh"

#include "compile.hh"
#include "compile_wagner.hh"
#include "recursivness.hh"
#include "simplify.hh"
#include "privatise.hh"
#include "prim2.hh"
#include "xtended.hh"
#include "wExp.cpp"

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


string ToWagner (Tree sig, int prec = 0);
wExp *ToWagnerExp (Tree sig);

Tree recids_nil = tree (symbol ("wagner_nil"));
Tree recids = recids_nil;

Tree WagnerCompiler::prepareWagner (Tree LS)
{

  Tree
    L1 = deBruijn2Sym (LS);	// convert debruijn recursion into symbolic recursion

  Tree
    L2 = simplify (L1);		// simplify by executing every computable operation
  Tree
    L3 = privatise (L2);	// Un-share tables with multiple writers
  string
    s;

  cerr << ToWagner (L1, 0)+"\n";
  for (Tree t1;
     recids != recids_nil && isTree (recids, recids->node (), t1);)
     {
     s="\n recursive signal is : \n";
     Tree n = (Tree) recids->node ().getPointer ();
     s=s+ToWagner (n, 0)+"=";

     recids = t1;
     s=s+ToWagner (n->getProperty (tree (symbol ("RECDEF"))), 0);
     cerr<< s ;        

     } 
   cerr<<"\nwith WagnerExp\n";
   cerr << ToWagnerExp (L1)->to_string () + "\n";

  exit (0);
  return L3;
}


void
WagnerCompiler::compileMultiSignal (Tree L)
{
  L = prepareWagner (L);
}

wExp *
ToWagnerExp (Tree sig)
{
  int i;
  double r;
  Tree x, y, z, u, le, id;
  xtended *p = (xtended *) getUserData (sig);

  if(p)
    {
	int v=sig->arity();
	 for(int j=0; j < v ; j++)
	return new wMathF(p->name(),ToWagnerExp(sig->branch(j)));
    }
  else if (isSigInt (sig, &i))
    {
      return new wInteger (i);
    }
  else if (isSigReal (sig, &r))
    {
      return new wDouble (r);
    }
  else if(isSigInput(sig,&i))
    {
	return new wVar1(i);
    }
  else if(isSigOutput(sig,&i,x))
    {
	return new wVar2(i,ToWagnerExp(x));
    }
  else if(isSigBinOp(sig,&i,x,y))
    {
	return new wBinaryOp(ToWagnerExp(x), ToWagnerExp(y), binopn[i]) ;
    }
  else if(isSigDelay1(sig, x))
    {
	return new wDelay1(ToWagnerExp(x));
    }
  else if(isSigPrefix(sig, x, y))
    {
	return new wPrefix(ToWagnerExp(x),ToWagnerExp(y));
    }
  else if(isSigAttach(sig, x, y))
    {
	return new wAttach(ToWagnerExp(x),ToWagnerExp(y));
    }
  else if(isSigFixDelay(sig, x, y))
    {
	return new wFixDelay(ToWagnerExp(x),ToWagnerExp(y));
    }
  else if (isList (sig) && len (sig) == 1)
    {

      return ToWagnerExp (hd (sig));

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
	while(isList(tsig));
      /*while (!isNil (tsig))
	{
	  vec.push_back (ToWagnerExp (hd (tsig)));
	  tsig = tl (tsig);
	}*/
      return new wTuple (vec);
    }
  
  else
    {
      return new wInteger (999);
    }

}

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
	s = s + ToWagner (sig->branch (j), 0);
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
  else if (isRef (sig, x))
    {
      s = tree2str (x);

      static Tree seen = NULL ;
      if( seen == NULL ) {
	seen = tree (symbol ("wagner"));
	}
      if (sig->getProperty (seen) == NULL)
	{
	  sig->setProperty (seen, seen);
	  recids = tree (Node ((void *) sig), recids);
	}
      return s;
    }


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


void
WagnerCompiler::compileSingleSignal (Tree sig)
{
}

Tree WagnerCompiler::prepare (Tree LS)
{
}

Tree WagnerCompiler::prepare2 (Tree LS)
{
}
