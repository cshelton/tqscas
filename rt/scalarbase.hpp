#ifndef SCALARBASE_HPP
#define SCALARBASE_HPP

#include <functional>
#include "expr.hpp"
#include <cmath>

// shorter to write...
template<typename T, typename... Xs> auto toptr(Xs &&...xs) {
	return std::make_shared<T>(std::forward<Xs>(xs)...);
}

template<typename T> auto toptr() { return std::make_shared<T>(); }

const op plusop = toptr<binopinfo<std::plus<double>,double,double>>
			(std::string("+"),true,true,6);
const op minusop = toptr<binopinfo<std::minus<double>,double,double>>
			(std::string("-"),true,true,6);
const op negateop = toptr<uniopinfo<std::negate<double>,double>>
			(std::string("-"),false,false,3);
const op multipliesop = toptr<binopinfo<std::multiplies<double>,double,double>>
			(std::string("*"),true,true,4);
const op dividesop = toptr<binopinfo<std::divides<double>,double,double>>
			(std::string("/"),true,true,4);

const std::shared_ptr<opchain> pluschain = std::make_shared<opchain>(plusop);
const std::shared_ptr<opchain> multiplieschain = std::make_shared<opchain>(multipliesop);

template<typename T>
struct powerinfo {
	constexpr T operator()(const T &b, const T &e) const {
		return std::pow(b,e);
	}
};

const op powerop = toptr<binopinfo<powerinfo<double>,double,double>>
			(std::string("^"),true,false,2);

template<typename T>
struct natloginfo {
	constexpr T operator()(const T &e) const {
		return std::log(e);
	}
};

const op logop = toptr<uniopinfo<natloginfo<double>,double>>
			(std::string("log"),false,false,5);


expr operator+(const expr &e1, const expr &e2) {
	if (!e1.isleaf() && e1.asnode()==pluschain) {
		auto ch = e1.children();
		ch.emplace_back(e2);
		return {pluschain,ch};
	} else return {pluschain,e1,e2};
}
expr operator+(const double e1, const expr &e2) {
	return {pluschain,newconst(e1),e2};
}
expr operator+(const expr &e1, const double &e2) {
	if (!e1.isleaf() && e1.asnode()==pluschain) {
		auto ch = e1.children();
		ch.emplace_back(newconst(e2));
		return {pluschain,ch};
	} else return {pluschain,e1,newconst(e2)};
}

expr operator-(const expr &e1, const expr &e2) {
	return {minusop,e1,e2};
}
expr operator-(const double e1, const expr &e2) {
	return {minusop,newconst(e1),e2};
}
expr operator-(const expr &e1, const double &e2) {
	return {minusop,e1,newconst(e2)};
}

expr operator*(const expr &e1, const expr &e2) {
	if (!e1.isleaf() && e1.asnode()==multiplieschain) {
		auto ch = e1.children();
		ch.emplace_back(e2);
		return {multiplieschain,ch};
	} else return {multiplieschain,e1,e2};
}
expr operator*(const double e1, const expr &e2) {
	return {multiplieschain,newconst(e1),e2};
}
expr operator*(const expr &e1, const double &e2) {
	if (!e1.isleaf() && e1.asnode()==multiplieschain) {
		auto ch = e1.children();
		ch.emplace_back(e2);
		return {multiplieschain,ch};
	} else return {multiplieschain,e1,newconst(e2)};
}

expr operator/(const expr &e1, const expr &e2) {
	return {dividesop,e1,e2};
}
expr operator/(const double e1, const expr &e2) {
	return {dividesop,newconst(e1),e2};
}
expr operator/(const expr &e1, const double &e2) {
	return {dividesop,e1,newconst(e2)};
}

expr operator-(const expr &e) {
	return {negateop,e};
}

expr pow(const expr &e1, const double &e2) {
	return {powerop,e1,newconst(e2)};
}
expr pow(const double &e1, const expr &e2) {
	return {powerop,newconst(e1),e2};
}
expr pow(const expr &e1, const expr &e2) {
	return {powerop,e1,e2};
}

expr log(const expr &e) {
	return {logop,e};
}

constexpr double scalare = exp(1.0);

expr exp(const expr &e) {
	return {powerop,newconst(scalare),e};
}


/*
template<typename T>
struct condopinfo : public opinfo {
	condopinfo() : opinfo(3,"?",false,false,15) {}
	// three children: child1 < 0 ? child2 : child3
	virtual any opeval(const any &x1, const any &x2, const any &x3) const {
		return (MYany_cast<T>(x1)<0) ? MYany_cast<T>(x2) : MYany_cast<T>(x3);
	}
};
*/

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

//const op condop = toptr<condopinfo<double>>();
const op switchop = toptr<switchopinfo<double>>();
*/

template<typename T>
struct heavisideinfo : public opinfo {
	T zeroval;
	heavisideinfo(T zval=1.0) : opinfo(1,"H",false,false,5), zeroval(zval) {}

	virtual any opeval(const any &x1) const {
		return (MYany_cast<T>(x1)>0.0) ? any{T(1.0)} : 
			(MYany_cast<T>(x1)==0.0 ? any{zeroval} : any{T(0.0)});
	}
};

template<typename T>
struct diracinfo : public opinfo {
	diracinfo(): opinfo(1,"delta",false,false,5) {}

	virtual any opeval(const any &x1) const {
		return (MYany_cast<T>(x1)==0) ? any{std::numeric_limits<T>::infinity()} : any{T(0.0)};
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

const op heavisideop = toptr<heavisideinfo<double>>();
const op heavisideleftop = toptr<heavisideinfo<double>>(0.0);
const op diracop = toptr<diracinfo<double>>();
const op absop = toptr<absinfo<double>>();

template<typename E1, typename E2, typename E3>
expr cond(E1 &&condition, E2 &&negexp, E3 &&posexp) {
	/*
	return {switchop,std::forward<E1>(condition),
		std::forward<E2>(negexp), newconst(0.0), std::forward<E3>(posexp)};
		*/
	return posexp*expr{heavisideop,condition}
			+ negexp*expr{heavisideleftop,-1.0*condition};
}

expr abs(const expr &e) {
	//return cond(e,-1.0*e,e);
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

const op derivop = toptr<derivopinfo<double>>();

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
		T v0 = MYany_cast<double>(::eval(x[2])), v1 = MYany_cast<double>(::eval(x[3]));
		any ret{id};
		for(T v = v0; v<=v1; v++)
			ret = bop->opeval(ret,
				::eval(substitute(x[1],x[0],newconst(v))));
		return ret;
	}
};

const op sumop = toptr<bigopinfo<double>>(plusop,0.0,"sum");
const op prodop = toptr<bigopinfo<double>>(multipliesop,1.0,"prod");

expr sum(const expr &e, const expr &x, const expr &x0, const expr &x1) {
	return {sumop,x,e,x0,x1};
}
template<typename T>
expr sum(const expr &e, const expr &x, T x0, const expr &x1) {
	return {sumop,x,e,newconst(x0),x1};
}
template<typename T>
expr sum(const expr &e, const expr &x, const expr &x0, T x1) {
	return {sumop,x,e,x0,newconst<T>(x1)};
}
template<typename T0, typename T1>
expr sum(const expr &e, const expr &x, T0 x0, T1 x1) {
	return {sumop,x,e,newconst<T0>(x0),newconst<T1>(x1)};
}

expr prod(const expr &e, const expr &x, const expr &x0, const expr &x1) {
	return {prodop,x,e,x0,x1};
}
template<typename T>
expr prod(const expr &e, const expr &x, T x0, const expr &x1) {
	return {prodop,x,e,newconst(x0),x1};
}
template<typename T>
expr prod(const expr &e, const expr &x, const expr &x0, T x1) {
	return {prodop,x,e,x0,newconst<T>(x1)};
}
template<typename T0, typename T1>
expr prod(const expr &e, const expr &x, T0 x0, T1 x1) {
	return {prodop,x,e,newconst<T0>(x0),newconst<T1>(x1)};
}

#endif
