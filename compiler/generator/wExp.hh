#include <functional>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <iostream>
#include "tree.hh"

// using namespace std;


////////////////////////////////////////////////////////////////////////
// Abstract class for Wagner expressions                              //
////////////////////////////////////////////////////////////////////////


class wExp
{

public:

  wExp ()
  {
  };
//  virtual std::set<Tree> Depend();
  virtual ostream & print (ostream & fout) const = 0;

};

inline ostream & operator << (ostream & file, wExp * e) {return e->print (file);}

////////////////////////////////////////////////////////////////////////
// Reference to hash table
////////////////////////////////////////////////////////////////////////
class wHash:public wExp
{

public:

  Tree tref;
  wExp *eref;

    wHash (Tree tre,wExp* e):tref (tre),eref(e)
  {
  }

  ostream & print (ostream & out) const
  {
	return out<<"P["<< tref <<"]" ;
  }
};

////////////////////////////////////////////////////////////////////////
// Integer and double constants                                       //
////////////////////////////////////////////////////////////////////////
class wInteger:public wExp
{

public:

  int value;

    wInteger (int val):value (val)
  {
  }
  ostream & print (ostream & out) const
  {
    out << value;
    return out;
  }
  
};

class wDouble:public wExp
{

public:

  double value;

    wDouble (double val):value (val)
  {
  }

  ostream & print (ostream & out) const
  {
    out << value;
    return out;
  }
  
};

////////////////////////////////////////////////////////////////////////
// Variables (input and output)                                       //
////////////////////////////////////////////////////////////////////////
class wVar1:public wExp
{

public:

  // This choice implies that variable shifting must create a copy of
  // the constructor.
  int idx;

    wVar1 (int v_idx):idx (v_idx)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "IN" << idx;
    return out;
  }
  
};
class wVar2:public wExp
{

public:

  int idx;
  wExp *x;

    wVar2 (int v_idx, wExp * e):idx (v_idx), x (e)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "OUT" << idx << x;
    return out;
  }

};
//this class for FConst, FVar and waveforme signals
class wWaveform:public wExp
{

public:

  string str;

  wWaveform (string mess):str (mess)
  {
  }
  ostream & print (ostream & out) const
  {
    out << str;
    return out;
  }
};
////////////////////////////////////////////////////////////////////////
// Binary operators                                                   //
////////////////////////////////////////////////////////////////////////
class wBinaryOp:public wExp
{

public:

  std::tuple < wExp *, wExp * >args;
  const char *op;

    wBinaryOp (wExp * e1, wExp * e2, const char *c):args (e1, e2), op (c)
  {
  }

  ostream & print (ostream & out) const
  {
    out << "(" << std::get < 0 > (args) << op << std::get < 1 > (args) << ")";
    return out;
  }

};

////////////////////////////////////////////////////////////////////////
// Projections                                                        //
////////////////////////////////////////////////////////////////////////
class wProj:public wExp
{

public:

  wExp * p_exp;
  int p_idx;

    wProj (wExp * exp, int idx):p_exp (exp), p_idx (idx)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "Proj" << p_idx << " " << p_exp;
    return out;
  }

};
////////////////////////////////////////////////////////////////////////
//                                                                    //
////////////////////////////////////////////////////////////////////////
class wDelay1:public wExp
{

public:

  wExp * d_exp;

  wDelay1 (wExp * exp):d_exp (exp)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "mem(" << d_exp << ")";
    return out;
  }

};
class wFixDelay:public wExp
{

public:

  std::tuple < wExp *, wExp * >args;

  wFixDelay (wExp * e1, wExp * e2):args (e1, e2)
  {
  }

  ostream & print (ostream & out) const
  {
    out << std::get < 0 > (args) << "@" << std::get < 1 > (args);
    return out;
  }  
};
////////////////////////////////////////////////////////////////////////
// Feedback                                                           //
// We assume deBruijn notation, but that could be changed!            //
////////////////////////////////////////////////////////////////////////
class wFeed:public wExp
{

public:

  wExp * exp;
  std::unordered_map<Tree,wExp*>* env;

  wFeed (wExp * e, std::unordered_map<Tree,wExp*>* ev) : exp (e),env(ev)
  {
  }
  ostream & print (ostream & out) const
  {
	std::map<Tree,wExp*> temp;

    out << " Feed = "<< exp ;
	out<<"\nFeed Table\n" ;
	 for(auto & elem : *env)
		{
	        temp.insert (elem);
        }	
	for(std::pair<Tree,wExp*> e : temp)
		{ 	
      		out << "\n[" << e.first << "]]: " << e.second << endl;
        }	
 	
    return out;
  }
};

class wFeed1:public wExp
{

public:

  wExp * exp;
  string x;
    wFeed1 (wExp * ex, string s):exp (ex), x (s)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "Feed1 =" << x << exp;
    return out;
  }
};
class wRef:public wExp
{

public:

  int ref;

    wRef (int val):ref (val)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "REF[" << ref << "]";
    return out;
  }

};
////////////////////////////////////////////////////////////////////////
// Tuples                                                             //
////////////////////////////////////////////////////////////////////////
class wTuple:public wExp
{

public:

  std::vector < wExp * >elems;
  int n;

    wTuple (std::initializer_list < wExp * >l):elems (l)
  {
  }
  wTuple (std::vector < wExp * >l, int k):elems (l), n (k)
  {
  }
  ostream & print (ostream & out) const
  {
	out<<"(";
    for (int j = 0; j < n; j++)
      {
	out << elems[j];
	if (j < n - 1)
	  out << ",";
      }
	out<<")";
    return out;
  }

};

////////////////////////////////////////////////////////////////////////
// Function and Interface                                            //
////////////////////////////////////////////////////////////////////////
class wFun:public wExp
{

public:

  string f_name;
  std::vector < wExp * >elems;
  
    wFun (string s, std::vector < wExp * >l):f_name (s), elems (l)
  {
  }
  ostream & print (ostream & out) const
  {
    int n = elems.size ();
      out << f_name + "(";
    for (int j = 0; j < n; j++)
      {
	out << elems[j];
	if (j < n - 1)
	  out << ",";
      }
    out << ")";
    return out;
  }

};

class wUi:public wExp
{

public:

  string name;
  string label;
  std::vector < wExp * >elems;
  
    wUi (string s, string lb, std::vector < wExp * >l):name (s), label (lb),
    elems (l)
  {
  }
  wUi (string s, string lb):name (s), label (lb)
  {
  }
  ostream & print (ostream & out) const
  {
    out << name + "(" + label;
    int n = elems.size ();

    if (n != 0)
        out << ",";
    for (int j = 0; j < n; j++)
      {
	out << elems[j];
	if (j < n - 1)
	  out << ",";
      }
    out << ")";

    return out;
  }

};

class wError:public wExp
{

public:

  //Tree sig;
  string s;

  wError (string err):s (err)
  {
  }
  ostream & print (ostream & out) const
  {
    out << "\"" << s << "is NOT A SIAGNAL \"";
    return out;
  }
};
