#ifndef SCALARBASE_HPP
#define SCALARBASE_HPP

#include <functional>
#include "exprbaseops.hpp"
#include <cmath>
#include "scalartype.hpp"

template<typename E>
E scalar(double d) {
	if (std::fmod(d,1.0)==0.0) return newconst<E>(scalarreal{(int)d});
	return newconst<E>(scalarreal{d});
}

template<typename E>
E scalar(int i) {
	return newconst<E>(scalarreal{i});
}

template<typename E>
E scalar(scalarreal s) {
	return newconst<E>(scalarreal{std::move(s)});
}

/*
// shorter to write...
template<typename T, typename... Xs> auto toptr(Xs &&...xs) {
	return std::make_shared<T>(std::forward<Xs>(xs)...);
}

template<typename T> auto toptr() { return std::make_shared<T>(); }
*/

// ADD
struct scalaropadd {};

template<typename T>
T evalop(const scalaropadd &, const T &a, const T &b) { return a+b; }
std::string symbol(const scalaropadd &) { return std::string("+"); }
int precedence(const scalaropadd &) { return 6; }
std::string write(const scalaropadd &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeinplace(op,subst,true);
}

// SUB
struct scalaropsub {};

template<typename T>
T evalop(const scalaropsub &, const T &a, const T &b) { return a-b; }
std::string symbol(const scalaropsub &) { return std::string("-"); }
int precedence(const scalaropsub &) { return 6; }
std::string write(const scalaropsub &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeinplace(op,subst,true);
}

// MULT
struct scalaropmult {};

template<typename T>
T evalop(const scalaropmult &, const T &a, const T &b) { return a*b; }
std::string symbol(const scalaropmult &) { return std::string("*"); }
int precedence(const scalaropmult &) { return 4; }
std::string write(const scalaropmult &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeinplace(op,subst,true);
}

// DIV
struct scalaropdiv {};

template<typename T>
T evalop(const scalaropdiv &, const T &a, const T &b) { return a/b; }
std::string symbol(const scalaropdiv &) { return std::string("/"); }
int precedence(const scalaropdiv &) { return 4; }
std::string write(const scalaropdiv &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeinplace(op,subst,true);
}

// NEG
struct scalaropneg {};

template<typename T>
T evalop(const scalaropneg &, const T &a) { return -a; }
std::string symbol(const scalaropneg &) { return std::string("-"); }
int precedence(const scalaropneg &) { return 3; }
std::string write(const scalaropneg &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeprefixunary(op,subst);
}

// POW
struct scalaroppow {};

template<typename T>
T evalop(const scalaroppow &, const T &a, const T &b) { return pow(a,b); }
std::string symbol(const scalaroppow &) { return std::string("^"); }
int precedence(const scalaroppow &) { return 4; }
std::string write(const scalaroppow &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeinplace(op,subst,false);
}

// LOG
struct scalaroplog {};

template<typename T>
T evalop(const scalaropneg &, const T &a) { return log(a); }
std::string symbol(const scalaropneg &) { return std::string("log"); }
int precedence(const scalaropneg &) { return 5; }
std::string write(const scalaropneg &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return writeasfunc(op,subst);
}

// chains (addition and multiplication)

using scalaropaddchain = opbinarychain<scalaropadd>;
using scalaropmultchain = opbinarychain<scalaropmult>;

expr operator+(const expr &e1, const expr &e2) {
	if (!e1.isleaf() && e1.asnode()==pluschain) {
		auto ch = e1.children();
		ch.emplace_back(e2);
		return {pluschain,ch};
	} else return {pluschain,e1,e2};
}
template<typename S>
expr operator+(const S e1, const expr &e2) {
	return {pluschain,scalar(e1),e2};
}
template<typename S>
expr operator+(const expr &e1, const S &e2) {
	if (!e1.isleaf() && e1.asnode()==pluschain) {
		auto ch = e1.children();
		ch.emplace_back(scalar(e2));
		return {pluschain,ch};
	} else return {pluschain,e1,scalar(e2)};
}

expr operator-(const expr &e1, const expr &e2) {
	return {minusop,e1,e2};
}
template<typename S>
expr operator-(const S &e1, const expr &e2) {
	return {minusop,scalar(e1),e2};
}
template<typename S>
expr operator-(const expr &e1, const S &e2) {
	return {minusop,e1,scalar(e2)};
}

expr operator*(const expr &e1, const expr &e2) {
	if (!e1.isleaf() && e1.asnode()==multiplieschain) {
		auto ch = e1.children();
		ch.emplace_back(e2);
		return {multiplieschain,ch};
	} else return {multiplieschain,e1,e2};
}
template<typename S>
expr operator*(const S &e1, const expr &e2) {
	return {multiplieschain,scalar(e1),e2};
}
template<typename S>
expr operator*(const expr &e1, const S &e2) {
	if (!e1.isleaf() && e1.asnode()==multiplieschain) {
		auto ch = e1.children();
		ch.emplace_back(scalar(e2));
		return {multiplieschain,ch};
	} else return {multiplieschain,e1,scalar(e2)};
}

expr operator/(const expr &e1, const expr &e2) {
	return {dividesop,e1,e2};
}
template<typename S>
expr operator/(const S &e1, const expr &e2) {
	return {dividesop,scalar(e1),e2};
}
template<typename S>
expr operator/(const expr &e1, const S &e2) {
	return {dividesop,e1,scalar(e2)};
}

expr operator-(const expr &e) {
	return {negateop,e};
}

template<typename S>
expr pow(const expr &e1, const S &e2) {
	return {powerop,e1,scalar(e2)};
}
template<typename S>
expr pow(const S &e1, const expr &e2) {
	return {powerop,scalar(e1),e2};
}
expr pow(const expr &e1, const expr &e2) {
	return {powerop,e1,e2};
}

expr log(const expr &e) {
	return {logop,e};
}

const expr scalare = newconst(scalarreal{scalarreal::eulerconst{}});
const expr scalarpi = newconst(scalarreal{scalarreal::piconst{}});

expr exp(const expr &e) {
	return {powerop,scalare,e};
}


template<typename T>
struct condopinfo : public opinfo {
	condopinfo() : opinfo(3,"?",false,false,15) {}
	// three children: child1 < 0 ? child2 : child3
	virtual any opeval(const any &x1, const any &x2, const any &x3) const {
		return (MYany_cast<T>(x1)<0) ? MYany_cast<T>(x2) : MYany_cast<T>(x3);
	}
};

template<typename T>
struct condeqopinfo : public opinfo {
	condeqopinfo() : opinfo(3,"?=",false,false,15) {}
	// three children: child1 == 0 ? child2 : child3
	virtual any opeval(const any &x1, const any &x2, const any &x3) const {
		return (MYany_cast<T>(x1)==0) ? MYany_cast<T>(x2) : MYany_cast<T>(x3);
	}
};

/*
template<typename T>
struct switchopinfo : public opinfo {
	// children: <test> <val1> <thresh1> <val2> <thresh2>
	// 			... <valn> <threshn> <valn+1>
	// threshs should be constants (?) and in sorted order (?)
	// evals to vali iff test<threshi and test>=thresh j forall j<i
	// evals to valn+1 iff test>=threshn
	
	switchopinfo() : opinfo(0,"S",false,false,15) {}
	virtual any opeval(const std::vector<any> &x) const {
		T val = MYany_cast<T>(x[0]);
		for(int i=2;i<x.size();i+=2) {
			T theta = MYany_cast<T>(x[i]);
			if (val < theta) return MYany_cast<T>(x[i-1]);
		}
		return MYany_cast<T>(x.back());
	}
};

const op switchop = toptr<switchopinfo<scalarreal>>();
*/
const op condop = toptr<condopinfo<scalarreal>>();
const op condeqop = toptr<condeqopinfo<scalarreal>>();

template<typename T>
struct heavisideinfo : public opinfo {
	T zeroval;
	heavisideinfo(T zval=1) : opinfo(1,"H",false,false,5), zeroval(zval) {}

	virtual any opeval(const any &x1) const {
		return (MYany_cast<T>(x1)>0) ? any{T(1)} : 
			(MYany_cast<T>(x1)==0 ? any{zeroval} : any{T(0)});
	}
};

template<typename T>
struct diracinfo : public opinfo {
	diracinfo(): opinfo(1,"delta",false,false,5) {}

	virtual any opeval(const any &x1) const {
		return (MYany_cast<T>(x1)==0) ? any{std::numeric_limits<T>::infinity()} : any{T(0)};
	}
};

template<typename T>
struct absinfo : public opinfo {
	// TODO: fix notation for printing
	absinfo(): opinfo(1,"||",false,false,5) {}

	virtual any opeval(const any &x1) const {
		return (MYany_cast<T>(x1)<0) ? any{-MYany_cast<T>(x1)} : x1;
	}
};

const op heavisideop = toptr<heavisideinfo<scalarreal>>();
const op heavisideleftop = toptr<heavisideinfo<scalarreal>>(0);
const op diracop = toptr<diracinfo<scalarreal>>();
const op absop = toptr<absinfo<scalarreal>>();

template<typename T>
struct toexprimpl {
	template<typename TT>
	expr exec(TT &&v) const { return scalar(std::forward<TT>(v)); }
};

template<>
struct toexprimpl<expr> {
	expr exec(const expr &e) const { return e; }
};

template<typename T>
expr toexpr(T &&v) {
	toexprimpl<typename std::remove_cv<
		typename std::remove_reference<T>::type>::type> impl;
    return impl.exec(std::forward<T>(v));
}    

template<typename E1, typename E2, typename E3>
expr ifthenelse(E1 &&condition, E2 &&negexp, E3 &&posexp) {
	/*
	return {switchop,std::forward<E1>(condition),
		std::forward<E2>(negexp), scalar(0), std::forward<E3>(posexp)};
		*/
	/*
	return posexp*expr{heavisideop,condition}
			+ negexp*expr{heavisideleftop,-1*condition};
			*/
	return {condop,toexpr(std::forward<E1>(condition)),
		toexpr(std::forward<E2>(negexp)), toexpr(std::forward<E3>(posexp))};
}

template<typename E1, typename E2, typename E3>
expr ifeqthenelse(E1 &&condition, E2 &&negexp, E3 &&posexp) {
	return {condeqop,toexpr(std::forward<E1>(condition)),
		toexpr(std::forward<E2>(negexp)), toexpr(std::forward<E3>(posexp))};
}

expr abs(const expr &e) {
	//return cond(e,-1*e,e);
	return {absop,e};
}

/*
auto varargtovec(std::vector<expr> &ret) {
	return ret;
}

template<typename E1, typename... Es>
auto varargtovec(std::vector<expr> &ret, E1 &&e1, Es &&...es) {
	ret.emplace_back(std::forward<E1>(e1));
	return varargtovec(ret,std::forward<Es>(es)...);
}

template<typename E1, typename... Es>
expr caseexpr(E1 &&condition, Es &&...exprs) {
	std::vector<expr> es{};
	return {switchop,varargtovec(es,std::forward<Es>(exprs)...)};
}

expr caseexpr(const expr &condexp, std::vector<expr> args) {
	args.insert(args.begin(),condexp);
	return {switchop,std::move(args)};
}
*/

template<typename T>
struct derivopinfo : public scopeinfo {
	// 3 children: v, e, val
	// represents de/dx at v=val
	// (where v is local variable, as per scopeinfo)
	derivopinfo() : scopeinfo(3,"d",false,false,5) {}
};

const op derivop = toptr<derivopinfo<scalarreal>>();

expr deriv(const expr &e, const expr &x) {
	if (!isvar(x)) return noexpr;
	expr localx = newvar(getvartype(x));
	return {derivop,localx,substitute(e,x,localx),x};
}

// be careful using... localx must appear in e (and x should probably not!)
expr deriv(const expr &e, const expr &localx, const expr &x) {
	return {derivop,localx,e,x};
}

template<typename T>
struct bigopinfo : public scopeinfo {
	// 4 children: v, e, v0, v1
	// represents sum_{v=v0}^v1 e
	// (where v is local variable, as per scopeinfo)
	op bop;
	T id;
	bigopinfo(op baseop, T i, const std::string &n) : bop(baseop), id(i),
			scopeinfo(4,n,false,false,5) {}

	virtual bool caneval(const std::vector<expr> &x) const {
		return isconst(x[2]) && isconst(x[3])
			&& isvar(x[0])
			&& isconstexprexcept(x[1],getvar(x[0]))
			&& std::isfinite(getconst<T>(x[2]))
			&& std::isfinite(getconst<T>(x[3]));
	}


	virtual any eval(const std::vector<expr> &x) const {
		T v0 = MYany_cast<scalarreal>(::eval(x[2])), v1 = MYany_cast<scalarreal>(::eval(x[3]));
		any ret{id};
		for(T v = v0; v<=v1; v++)
			ret = bop->opeval(ret,
				::eval(substitute(x[1],x[0],scalar(v))));
		return ret;
	}
};

const op sumop = toptr<bigopinfo<scalarreal>>(plusop,0,"sum");
const op prodop = toptr<bigopinfo<scalarreal>>(multipliesop,1,"prod");

template<typename T>
struct integrateinfo : public scopeinfo {
	integrateinfo() : scopeinfo(4,"int",false,false,5) {}

	virtual bool caneval() const { return false; }
};

const op integrateop  = toptr<integrateinfo<scalarreal>>();

template<typename E1, typename E2, typename E3>
expr integrate(E1 &&e, const expr &x, E2 &&x0, E3 &&x1) {
	return {integrateop,x,toexpr(std::forward<E1>(e)),toexpr(std::forward<E2>(x0)),toexpr(std::forward<E3>(x1))};
}

template<typename E1,typename E2, typename E3, typename E4>
expr sum(E1 &&e, E2 &&x, E3 &&x0, E4 &&x1) {
	return {sumop,toexpr(std::forward<E1>(x)),toexpr(std::forward<E2>(e)),
		toexpr(std::forward<E3>(x0)),toexpr(std::forward<E4>(x1))};
}

template<typename E1,typename E2, typename E3, typename E4>
expr prod(E1 &&e, E2 &&x, E3 &&x0, E4 &&x1) {
	return {prodop,toexpr(std::forward<E1>(x)),toexpr(std::forward<E2>(e)),
		toexpr(std::forward<E3>(x0)),toexpr(std::forward<E4>(x1))};
}

#endif
