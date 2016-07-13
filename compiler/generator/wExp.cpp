#include <functional>
#include <string>
#include <vector>
#include <array>
#include <tuple>
#include <iostream>

using namespace std;

////////////////////////////////////////////////////////////////////////
// Abstract class for Wagner expressions                              //
////////////////////////////////////////////////////////////////////////
class wExp {

public:

  wExp() { };

  virtual std::string to_string() const = 0;

};

////////////////////////////////////////////////////////////////////////
// Integer and double constants                                                  //
////////////////////////////////////////////////////////////////////////
class wInteger : public wExp {

public:

  int value;

  wInteger(int val) : value(val) { }

  std::string to_string() const {
    return std::to_string(value);
  }

};

class wDouble : public wExp {

public:

  double value;

  wDouble(double val) : value(val) { }

  std::string to_string() const {
    return std::to_string(value);
  }

};

////////////////////////////////////////////////////////////////////////
// Variables (input and output)                                                         //
////////////////////////////////////////////////////////////////////////
class wVar1 : public wExp {

public:

  // This choice implies that variable shifting must create a copy of
  // the constructor.
  int idx;

  wVar1(int v_idx) : idx(v_idx) { }
  std::string to_string() const {
    return "IN"+std::to_string(idx);
  }

};

class wVar2 : public wExp {

public:

  // This choice implies that variable shifting must create a copy of
  // the constructor.
  int idx;
  wExp* x;

  wVar2(int v_idx,wExp* e) : idx(v_idx), x(e) { }
  std::string to_string() const {
    return "OUT"+std::to_string(idx)+x->to_string();
  }

};
////////////////////////////////////////////////////////////////////////
// Binary operators                                                   //
////////////////////////////////////////////////////////////////////////
class wBinaryOp : public wExp {

public:

  std::tuple<wExp*, wExp*> args;
  const char* op;

  wBinaryOp(wExp *e1, wExp *e2, const char* c) : args(e1, e2), op(c) {} 
  //wBinaryOp(wExp *e1, wExp *e2) : args(e1, e2) { }
  //wBinaryOp(std::tuple<wExp*, wExp*> l) : args(l) { }

  std::string to_string() const {

    return "BinOp: "
      + std::get<0>(args)->to_string()
      + op
      + std::get<1>(args)->to_string();
  }

};

////////////////////////////////////////////////////////////////////////
// Tuples                                                             //
////////////////////////////////////////////////////////////////////////
class wTuple : public wExp {

public:

  std::vector<wExp*> elems;

  wTuple(std::initializer_list<wExp*> l) : elems(l) { }
  wTuple(std::vector<wExp*> l) : elems(l) { }

  std::string to_string() const {
    return "Tuple";
  }

};
////////////////////////////////////////////////////////////////////////
// Mathematic primitive fonctions                                     //
////////////////////////////////////////////////////////////////////////
class wMathF : public wExp {

public:

  string f_name;
  wExp   *f_exp;
  //int arity;

  wMathF(string e1, wExp *e2) : f_name(e1), f_exp(e2) { }

  std::string to_string() const {
    return f_name+"("+
           f_exp->to_string() +
	   ")";
  }

};

////////////////////////////////////////////////////////////////////////
// Projections                                                        //
////////////////////////////////////////////////////////////////////////
class wProj : public wExp {

public:

  wExp *p_exp;
  int   p_idx;

  wProj(wExp *exp, int idx) : p_exp(exp), p_idx(idx) { }

  std::string to_string() const {
    return "Proj";
  }

};
////////////////////////////////////////////////////////////////////////
//                                                                    //
////////////////////////////////////////////////////////////////////////
class wDelay1 : public wExp {

public:

  wExp *d_exp;

  wDelay1(wExp *exp) : d_exp(exp) { }

  std::string to_string() const {
    return "mem("
	   +d_exp->to_string()+")";
  }

};
class wPrefix : public wExp {

public:

  std::tuple<wExp*, wExp*> args;

  wPrefix(wExp* e1, wExp* e2) : args(e1,e2) { }

  std::string to_string() const {

    return "prefix("
      + std::get<0>(args)->to_string()
      + ","
      +std::get<1>(args)->to_string()
      +")";
  }
};
class wAttach : public wExp {

public:

  std::tuple<wExp*, wExp*> args;

  wAttach(wExp* e1, wExp* e2) : args(e1,e2) { }

  std::string to_string() const {

    return "attach("
      + std::get<0>(args)->to_string()
      + ","
      +std::get<1>(args)->to_string()
      +")";
 }
};
class wFixDelay : public wExp {

public:

  std::tuple<wExp*, wExp*> args;

  wFixDelay(wExp* e1, wExp* e2) : args(e1,e2) { }

  std::string to_string() const {

    return std::get<0>(args)->to_string()
	   + "@"
	   +std::get<1>(args)->to_string();
}
};
////////////////////////////////////////////////////////////////////////
// Feedback                                                           //
// We assume deBruijn notation, but that could be changed!            //
////////////////////////////////////////////////////////////////////////
class wFeed : public wExp {

public:

  std::vector<wExp*> eqns;

  wFeed(std::initializer_list<wExp*> l) : eqns(l) { }

  std::string to_string() const {
    return "Feed";
  }
};

/*int main() {

  auto a1 = wInteger(1);
  auto a2 = wInteger(2);
  auto aa = wBinaryOp(a1, a2);
  auto at = wTuple{a1,a2,aa};

  std::cout << aa.to_string()+"\n";
  std::cout << at.to_string()+"\n";

}*/

// TODO: Add a template for a dynamic fold/visitor over types.
