#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include <experimental/any>
#include <experimental/optional>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cmath>
#include <typeinfo>
#include <typeindex>
#include "scalartype.hpp"

using any = std::experimental::any;

template<typename T>
auto MYany_cast(const any &a) {
	     return std::experimental::any_cast<T>(a);
}
template<typename T>
auto MYany_cast(any &a) {
	     return std::experimental::any_cast<T>(a);
}

struct noexprT {};

typedef any leaf;
struct opinfo;

typedef std::shared_ptr<opinfo> op;
using expr = gentree<leaf,op>;

any eval(const expr &e);

struct constval {
	any v;
	constval(any val) : v{std::move(val)} {
		/*
		double vv = MYany_cast<double>(v);
		if (vv!=0.0 && std::abs(vv)<1e-6)
				std::cout << "RIGHT HERE" << std::endl;
				*/
	}
	template<typename T>
	constval(T val) : v{std::move(val)} {
		/*
		if (val!=0.0 && std::abs(val)<1e-6)
				std::cout << "RIGHT HERE" << std::endl;
				*/
	}
};

struct opinfo {
	// perhaps narg isn't needed (narg=0 => variable)
	std::size_t narg;

	// below is for conversion to human-read string
	std::string name,treename;
	bool infix;
	bool leftassoc;
	int prec;

	opinfo(std::size_t na, const std::string &n, bool inf, bool la, int pr)
		: narg(na), name(n), treename(n),
		  infix(inf), leftassoc(la), prec(pr) {}

	opinfo(std::size_t na, const std::string &n, const std::string &tn,
				bool inf, bool la, int pr)
		: narg(na), name(n), treename(tn),
		  infix(inf), leftassoc(la), prec(pr) {}


	virtual any opeval() const { return {}; };
	virtual any opeval(const any &x1) const { return {}; };
	virtual any opeval(const any &x1, const any &x2) const { return {}; };
	virtual any opeval(const any &x1, const any &x2,
			const any &x3) const { return {}; };
	virtual any opeval(const std::vector<any> &x) const {
		switch(x.size()) {
			case 0: return opeval();
			case 1: return opeval(x[0]);
			case 2: return opeval(x[0],x[1]);
			case 3: return opeval(x[0],x[1],x[2]);
		}
	}

	virtual any eval(const std::vector<expr> &x) const {
		std::vector<any> v;
		v.reserve(x.size());
		for(auto &e : x)
			v.emplace_back(::eval(e));
		return opeval(v);
	}

	virtual bool caneval() const { return true; }
};

struct varinfo {
	std::string name;
	const std::type_info &t;

	varinfo(const std::string &n, const std::type_info &ti) : name(n), t(ti) {}
};

struct var : public std::shared_ptr<varinfo> {
	var(const varinfo &vi) :
		std::shared_ptr<varinfo>(std::make_shared<varinfo>(vi)) {}
	var(varinfo &&vi) :
		std::shared_ptr<varinfo>(std::make_shared<varinfo>(std::move(vi))) {}
};

namespace std { 
	template<> struct hash<var> {
		typedef var argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return std::hash<std::shared_ptr<varinfo>>{}(s);
		}
	};
	template<> struct hash<expr> {
		typedef expr argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return s.ptrhash();
		}
	};

	bool operator==(const expr &e1, const expr &e2) {
		return e1.sameptr(e2);
	}
	bool operator!=(const expr &e1, const expr &e2) {
		return !(e1==e2);
	}

}


expr newvar(const std::string &name, const std::type_info &ti) {
	return expr{var{varinfo(name,ti)}};
}

template<typename T>
expr newvar(const std::string &name) {
	return expr{var{varinfo(name,typeid(T))}};
}

std::size_t globalvarnum__ = 0;

expr newvar(const std::type_info &ti) {
	return newvar(std::string("__")+std::to_string(++globalvarnum__),ti);
}

template<typename T>
expr newvar() {
	return newvar(typeid(T));
}

template<typename T>
expr newconst(const T &v) {
	constval VV{v};
	return expr{VV};
	//return expr{constval{v}};
}

any getconstany(const expr &e) {
	return MYany_cast<constval>(e.asleaf()).v;
}

template<typename T>
T getconst(const expr &e) {
	return MYany_cast<T>(getconstany(e));
}


var getvar(const expr &e) {
	return MYany_cast<var>(e.asleaf());
}


const std::type_info &getvartype(const expr &e) {
    return (MYany_cast<var>(e.asleaf()))->t;
}

template<typename T>
bool isop(const expr &e) {
	if (e.isleaf()) return false;
	auto p = std::dynamic_pointer_cast<T>(e.asnode());
	return (bool)p;
}

bool isop(const expr &e, op o) {
	return !e.isleaf() && e.asnode()==o;
}

bool isconst(const expr &e) {
	return e.isleaf() && e.asleaf().type()==typeid(constval);
}

bool isvar(const expr &e) {
	return e.isleaf() && e.asleaf().type()==typeid(var);
}

using vset = std::unordered_set<var>;
using vmap = std::unordered_map<var,var>;
//using vkmap = std::unordered_map<var,any>;

struct scopeinfo;// : public opinfo;

// do not call!
bool isconstexpr1(const expr &e,
		const optional<vset> &vars,
		vset &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return true;
		if (!isvar(e)) return false;
		var v = getvar(e);
		return (vars && vars->find(v)==vars->end())
				|| exceptvars.find(v)!=exceptvars.end();
	} else {
		auto n = std::dynamic_pointer_cast<scopeinfo>(e.asnode());
		if (n) {
			auto v = getvar(e.children()[0]);
			exceptvars.emplace(v);
		}
		for(auto &c : e.children())
			// no need to undo changes to exceptvars, because 
			// we will return false all the way back to the top
			if (!isconstexpr1(c,vars,exceptvars)) return false;
		if (n) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return true;
	}
}

// vars == empty => "all vars"
bool isconstexpr(const expr &e, const optional<vset> &vars = {}) {
	vset ex;
	return isconstexpr1(e,vars,ex);
}

bool isconstexpr(const expr &e, const var &v) {
    vset ex;
    optional<vset> vars{in_place,{v}};
    return isconstexpr1(e,vars,ex);
}

bool isconstexprexcept(const expr &e, const var &v) {
    vset ex{v};
    optional<vset> vars{};
    return isconstexpr1(e,vars,ex);
}
	    

// do not call!
bool isnonconstexpr1(const expr &e,
		const optional<vset> &vars,
		vset &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return false;
		if (!isvar(e)) return false; // (not sure what to place here...
						// ... only reason this isn't !isconstexpr
		var v = getvar(e);
		return (!vars || vars->find(v)!=vars->end())
				&& exceptvars.find(v)==exceptvars.end();
	} else {
		auto n = std::dynamic_pointer_cast<scopeinfo>(e.asnode());
		if (n) {
			auto v = getvar(e.children()[0]);
			exceptvars.emplace(v);
		}
		for(auto &c : e.children())
			// no need to undo changes to exceptvars, because 
			// we will return true all the way back to the top
			if (isnonconstexpr1(c,vars,exceptvars)) return true;
		if (n) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return false;
	}
}

// vars == empty => "all vars"
bool isnonconstexpr(const expr &e,
		const optional<vset> &vars = {}) {
	vset ex;
	return isnonconstexpr1(e,vars,ex);
}



bool operator==(const leaf &a, const leaf &b) {
     std::unordered_map<std::type_index,
		std::function<bool(const any &, const any &)>>
          eqcmplookupconst =
           {
             {typeid(scalarreal),[](const any &a, const any &b)
		    	{ return MYany_cast<scalarreal>(a) == MYany_cast<scalarreal>(b); }},
             {typeid(double),[](const any &a, const any &b)
		    	{ return MYany_cast<double>(a) == MYany_cast<double>(b); }},
             {typeid(long double),[](const any &a, const any &b)
		    	{ return MYany_cast<long double>(a) == MYany_cast<long double>(b); }},
             {typeid(float),[](const any &a, const any &b)
		    	{ return MYany_cast<float>(a) == MYany_cast<float>(b); }},
             {typeid(int),[](const any &a, const any &b)
		    	{ return MYany_cast<int>(a) == MYany_cast<int>(b); }},
             {typeid(unsigned int),[](const any &a, const any &b)
		    	{ return MYany_cast<unsigned int>(a) == MYany_cast<unsigned int>(b); }},
             {typeid(unsigned long),[](const any &a, const any &b)
		    	{ return MYany_cast<unsigned long>(a) == MYany_cast<unsigned long>(b); }},
             {typeid(unsigned long long),[](const any &a, const any &b)
		    	{ return MYany_cast<unsigned long long>(a) == MYany_cast<unsigned long long>(b); }},
		};

     std::unordered_map<std::type_index,std::function<bool(const any &, const any &)>>
          eqcmplookupleaf =
           {
			 {typeid(constval),[eqcmplookupconst](const any &a, const any &b)
				 { constval aa = MYany_cast<constval>(a);
					 constval bb = MYany_cast<constval>(b);
					 return aa.v.type()==bb.v.type()
						 && eqcmplookupconst.at(aa.v.type())(aa.v,bb.v);
				 }},
		   {typeid(var),[](const any &a, const any &b) 
		     { return MYany_cast<var>(a) == MYany_cast<var>(b); }},
		   {typeid(noexprT),[](const any &a, const any &b) 
		     { return true; }},
		   /*
		   {typeid(placeholder),[](const any &a, const any &b) 
		     { return MYany_cast<placeholder>(a).num
				== MYany_cast<placeholder>(b).num; }},
				*/
		   {typeid(op),[](const any &a, const any &b) 
		     { return MYany_cast<op>(a) == MYany_cast<op>(b); }},
            };

     return a.type()==b.type() && eqcmplookupleaf.at(a.type())(a,b);
}


template<typename OP, typename T1, typename T2>
struct binopinfo : public opinfo {
	OP op;
	binopinfo(const std::string &opname, bool inf, bool la, int pr)
		: opinfo(2,opname,inf,la,pr), op{} {}

	virtual any opeval(const any &x1, const any &x2) const {
		return {op(std::experimental::any_cast<T1>(x1),std::experimental::any_cast<T2>(x2))};
	}
};

template<typename OP, typename T1>
struct uniopinfo : public opinfo {
	OP op;
	uniopinfo(const std::string &opname, bool inf, bool la, int pr)
		: opinfo(1,opname,inf,la,pr), op{} {}

	virtual any opeval(const any &x1) const {
		return {op(std::experimental::any_cast<T1>(x1))};
	}
};

struct opchain : public opinfo {
	op baseop;
	opchain(op bop) : baseop(bop),
	opinfo(0,bop->name,bop->name+"C",bop->infix,bop->leftassoc,bop->prec) {}

	virtual any opeval(const any &x1) const {
		return x1;
	}
	virtual any opeval(const any &x1, const any &x2) const {
		return baseop->opeval(x1,x2);
	}
	virtual any opeval(const any &x1, const any &x2, const any &x3) const {
		if (leftassoc) return baseop->opeval(baseop->opeval(x1,x2),x3);
		else baseop->opeval(x1,baseop->opeval(x2,x3));
	}
	virtual any opeval(const std::vector<any> &x) const {
		if (leftassoc) return lefteval(*(x.begin()),x.begin()+1,x.end());
		else return righteval(x.begin(),x.end());
	}

	template<typename I>
		any lefteval(const any x, const I &b, const I &e) const {
			if (b==e) return x;
			return lefteval(baseop->opeval(x,*b),b+1,e);
		}

	template<typename I>
		any righteval(const I &b, const I &e) const {
			if (b+1==e) return *b;
			return baseop->opeval(*b,righteval(b+1,e));
		}
};

// abstract, for operations (like sum, deriv, integral)
//  that introduce a local variable
//  This just flags that the first child is
//    the new local variable
struct scopeinfo : public opinfo {
	using opinfo::opinfo;

	virtual bool caneval(const std::vector<expr> &x) const { return false; }
	virtual any eval(const std::vector<expr> &x) const { return {}; }

};

any eval(const expr &e) {
	if (e.isleaf()) {
		if (isconst(e)) return MYany_cast<constval>(e.asleaf()).v;
		return {};
	} else return e.asnode()->eval(e.children());
}

expr substitute(const expr &e, const expr &x, const expr &v);

struct evalatinfo : public scopeinfo {
	// 3 children: v, e, val
	// represents evaluating e where variable v is replaced by val
	evalatinfo(): scopeinfo(3,"eval",false,false,5) {}

	virtual bool caneval(const std::vector<expr> &x) const {
		return isconst(x[1]) || (isconst(x[2]) && isvar(x[0])
			&& isconstexprexcept(x[1],getvar(x[0])));
	}

	virtual any eval(const std::vector<expr> &x) const {
		return ::eval(substitute(x[1],x[0],x[2]));
	}
};

const op evalatop  = std::make_shared<evalatinfo>();

const expr noexpr {noexprT{}};

expr evalat(const expr &e, const expr &localx, const expr &x) {
	return {evalatop,localx,e,x};
}

#endif
