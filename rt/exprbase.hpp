#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include <experimental/any>
#include <experimental/optional>
#include <unordered_map>
#include <typeinfo>
#include <typeindex>

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

struct constval {
	any v;
	template<typename T>
	constval(T val) : v{std::move(val)} {}
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
		return {};
	}

	any eval(const std::vector<any> &x) const {
		switch(x.size()) {
			case 0: return opeval();
			case 1: return opeval(x[0]);
			case 2: return opeval(x[0],x[1]);
			case 3: return opeval(x[0],x[1],x[2]);
			default: return opeval(x);
		}
	}
};

typedef std::shared_ptr<opinfo> op;

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

typedef any leaf;

struct printableleafT {
	virtual std::string name() { return ""; }
};

/*
struct placeholder {
	int num;
};

placeholder P(int i) { return placeholder{i}; }
*/

bool operator==(const leaf &a, const leaf &b) {
     std::unordered_map<std::type_index,
		std::function<bool(const any &, const any &)>>
          eqcmplookupconst =
           {
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

using expr = gentree<leaf,op>;

const expr noexpr {noexprT{}};


template<typename T>
expr newvar(const std::string &name) {
	return var{varinfo(name,typeid(T))};
}

expr newvar(const std::string &name, const std::type_info &ti) {
	return var{varinfo(name,ti)};
}

template<typename T>
expr newconst(const T &v) {
	return {constval{v}};
}

template<typename T>
T getconst(const expr &e) {
	return MYany_cast<T>(MYany_cast<constval>(e.asleaf()).v);
}

bool isconst(const expr &e) {
	return e.isleaf() && e.asleaf().type()==typeid(constval);
}

bool isvar(const expr &e) {
	return e.isleaf() && e.asleaf().type()==typeid(var);
}

bool isconstexpr(const expr &e,const optional<std::vector<expr>> &vars) {
	if (e.isleaf()) {
		if (isconst(e)) return true;
		if (isvar(e)) {
			if (vars) {
				for(auto &v : *vars)
					if (e.asleaf()==v.asleaf()) return false;
				return true;
			} else return false;
		} else return false; // not var or const
	} else {
		for(auto &c : e.children())
			if (isconstexpr(c,vars)) return false;
		return true;
	}
}

bool isconstexpr(const expr &e) {
	return isconstexpr(e,optional<std::vector<expr>>{});
}


bool isnonconstexpr(const expr &e,const optional<std::vector<expr>> &vars) {
	if (e.isleaf()) {
		if (isconst(e)) return {};
		if (isvar(e)) {
			if (vars) {
				for(auto &v : *vars)
					if (e.asleaf()==v.asleaf()) return true;
				return false;
			} else return true;
		} else return false; // not var or const
	} else {
		for(auto &c : e.children())
			if (isnonconstexpr(c,vars)) return true;
		return false;
	}
}

bool isnonconstexpr(const expr &e) {
	return isnonconstexpr(e,optional<std::vector<expr>>{});
}

#endif
