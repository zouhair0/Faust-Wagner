#include <functional>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <iostream>
#include "tree.hh"

using namespace std;


////////////////////////////////////////////////////////////////////////
// Abstract class for Wagner expressions                              //
////////////////////////////////////////////////////////////////////////


class wExp
{

public:

  wExp ()
  {
  };

  virtual ostream & print (ostream & fout) const = 0;
  virtual std::string to_string () const = 0;

};

inline ostream & operator << (ostream & file, wExp * e) {return e->print (file);}

////////////////////////////////////////////////////////////////////////
// Reference to hash table
////////////////////////////////////////////////////////////////////////
class wHash:public wExp
{

public:

  void *ref;

    wHash (void *ref):ref (ref)
  {
  }

  ostream & print (ostream & out) const
  {
	ostringstream oss;
	oss<<ref;
	out<<"P["<< oss.str () <<"]";
    return out;
  }

  std::string to_string () const
  {
    ostringstream oss;
    oss << ref;
      return "P[" + oss.str () + "]";
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
  std::string to_string () const
  {
    return std::to_string (value);
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
  std::string to_string () const
  {
    return std::to_string (value);
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
  std::string to_string () const
  {
    return "IN" + std::to_string (idx);
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

  std::string to_string () const
  {
    return "OUT" + std::to_string (idx) + x->to_string ();
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

  std::string to_string () const
  {
    return str;
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

  std::string to_string () const
  {

    return "("
      + std::get < 0 > (args)->to_string ()
      + op + std::get < 1 > (args)->to_string () + ")";
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
    out << "Proj" << std::to_string (p_idx) << " " << p_exp;
    return out;
  }

  std::string to_string () const
  {
    return "Proj" + std::to_string (p_idx) + " " + p_exp->to_string ();
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

  std::string to_string () const
  {
    return "mem(" + d_exp->to_string () + ")";
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
  std::string to_string () const
  {

    return std::get < 0 > (args)->to_string ()
      + "@" + std::get < 1 > (args)->to_string ();
  }
};
////////////////////////////////////////////////////////////////////////
// Feedback                                                           //
// We assume deBruijn notation, but that could be changed!            //
////////////////////////////////////////////////////////////////////////
class wFeed:public wExp
{

public:

  //wFeed(std::initializer_list<wExp*> l) : eqns(l) { }
  wExp * exp;
  wFeed (wExp * ex):exp (ex)
  {
  }
  ostream & print (ostream & out) const
  {
    out << " Feed = " << exp;
    return out;
  }

  std::string to_string () const
  {
    return "\n Feed =" + exp->to_string ();
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
    out << "Fees1 =" << x << exp;
    return out;
  }

  std::string to_string () const
  {
    return "\n Feed1 " + x + exp->to_string ();
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

  std::string to_string () const
  {
    return "REF[" + std::to_string (ref) + "]";
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
    for (int j = 0; j < n; j++)
      {
	out << elems[j];
	if (j < n - 1)
	  out << ",";
      }
    return out;
  }

  std::string to_string ()const
  {
    string s;

    for (int j = 0; j < n; j++)
      {
	s = s + elems[j]->to_string ();
	if (j < n - 1)
	  s = s + ",";
      }
    return s;
  }

};

////////////////////////////////////////////////////////////////////////
// Function and Interface                                            //
////////////////////////////////////////////////////////////////////////
class wFun:public wExp
{

public:

  std::vector < wExp * >elems;
  string f_name;

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

  std::string to_string ()const
  {
    string s;
    int n = elems.size ();
      s = f_name + "(";
    for (int j = 0; j < n; j++)
      {
	s = s + elems[j]->to_string ();
	if (j < n - 1)
	  s = s + ",";
      }
    s = s + ")";
    return s;
  }
};

class wUi:public wExp
{

public:

  std::vector < wExp * >elems;
  string name;
  string label;

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

  std::string to_string ()const
  {

    string s = name + "(" + label;

    int n = elems.size ();

    if (n != 0)
        s = s + ",";
    for (int j = 0; j < n; j++)
      {
	s = s + elems[j]->to_string ();
	if (j < n - 1)
	  s = s + ",";
      }
    s = s + ")";

    return s;
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

  std::string to_string () const
  {

    return "\"" + s + " is NOT A SIGNAL \"";

  }

};
