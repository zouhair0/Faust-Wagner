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
// Variables (input and output)                                       //
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

    return "("
      + std::get<0>(args)->to_string()
      + op
      + std::get<1>(args)->to_string()
      + ")";
  }

};

////////////////////////////////////////////////////////////////////////
// Mathematic primitive fonctions                                     //
////////////////////////////////////////////////////////////////////////
class wMathF : public wExp {

public:

  string f_name;
  std::vector<wExp*> elems;
  

  wMathF(string e1,std::vector<wExp*> vec) : f_name(e1),elems(vec) { }

  std::string to_string() const {
	string s;
    s = f_name+"(";
		int a=elems.size();   
	       for(int j=0; j < a ; j++)
           {s  += elems[j]->to_string();
			if(j<a-1)
			s=s+",";
		   }
		   s= s+")";
	return s;
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
    return "Proj"
		   +std::to_string(p_idx)+" "
	       +p_exp->to_string();
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
/*class wPrefix : public wExp {

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
};*/
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

  //std::vector<wExp*> eqns;
  //wFeed(std::initializer_list<wExp*> l) : eqns(l) { }
   wExp* exp;
   wFeed(wExp* ex) :exp(ex) {}

  std::string to_string() const {
    return "Feed "+exp->to_string();
  }
};
class wFeed1 : public wExp {

public:

  //std::vector<wExp*> eqns;
  //wFeed(std::initializer_list<wExp*> l) : eqns(l) { }
   wExp* exp;
   string x;
   wFeed1(wExp* ex, string s) :exp(ex),x(s) {}

  std::string to_string() const {
    return "Feed "+
		   x+
		   exp->to_string();
  }
};
class wRef : public wExp {

public:

  int ref;

  wRef(int val) : ref(val) { }

  std::string to_string() const {
    return "REF["+std::to_string(ref)+"]";
  }

};
////////////////////////////////////////////////////////////////////////
//                                                                    //
////////////////////////////////////////////////////////////////////////
class wTable : public wExp {

public:

  std::tuple<wExp*, wExp*> args;

  wTable(wExp* e1, wExp* e2) : args(e1,e2) { }

  std::string to_string() const {

    return "table("
      + std::get<0>(args)->to_string()
      + ","
      +std::get<1>(args)->to_string()
      +")";
 }
};
class wWRTbl : public wExp {

public:

  std::tuple<wExp*, wExp*, wExp*> args;

  wWRTbl(wExp* e1, wExp* e2, wExp* e3) : args(e1,e2,e3) { }

  std::string to_string() const {

    return std::get<0>(args)->to_string() 
      +"["+std::get<1>(args)->to_string()+"]:=("
      +std::get<2>(args)->to_string()
      +")";
 }
};
////////////////////////////////////////////////////////////////////////
// Tuples                                                             //
////////////////////////////////////////////////////////////////////////
class wTuple : public wExp {

public:

  std::vector<wExp*> elems;
  int n;

  wTuple(std::initializer_list<wExp*> l) : elems(l) { }
  wTuple(std::vector<wExp*> l, int k) : elems(l), n(k) { }

  std::string to_string() const {
	string s;
	 
	       for(int j=0; j < n ; j++)
           {s  = s+ elems[j]->to_string();
			if(j < n-1)
				s=s+","; 
				
		   }
    return s;
  }

};
class wError : public wExp {

public:

  //Tree sig;
  string s;
  
	wError(string err) : s(err) { }

  std::string to_string() const {
    return s+" NOT A SIGNAL ";
  }

};
class wFun : public wExp {

public:

  std::vector<wExp*> elems;
  string name;

  wFun(string s , std::vector<wExp*> l) : elems(l) { }
  
  std::string to_string() const {
	string s;
	int n=elems.size();
	       s=name;
	       for(int j=0; j < n ; j++)
           {s  = s+ elems[j]->to_string();
			if(j < n-1)
				s=s+","; 
		   }
			s=s+")";
    return s;
  }
};
class wUi : public wExp {

public:

  std::vector<wExp*> elems;
  string name;
  string label;

  wUi(string n, string lb, std::vector<wExp*> l) : name(n), label(lb), elems(l) { }
  wUi(string n, string lb) : name(n), label(lb) { }
 
  std::string to_string() const {
	string s=name+label;
	int n=elems.size();
	if(n!=1)
	s=s+","; 
	       for(int j=0; j < n ; j++)
           {s  = s+ elems[j]->to_string();
			if(j < n-1)
				s=s+","; 
		   }
		   s=s+")";
    return s;
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
