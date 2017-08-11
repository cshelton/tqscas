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
struct power {
	constexpr T operator()(const T &b, const T &e) const {
		return std::pow(b,e);
	}
};

const op powerop = toptr<binopinfo<power<double>,double,double>>
			(std::string("^"),true,false,2);

template<typename T>
struct natlog {
	constexpr T operator()(const T &e) const {
		return std::log(e);
	}
};

const op logop = toptr<uniopinfo<natlog<double>,double>>
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

template<typename T>
struct switchopinfo : public opinfo {
	// children: <test> <val1> <thresh1> <val2> <thresh2>
	// 			... <valn> <threshn> <valn+1>
	// threshs should be constants and in sorted order! (?)
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

template<typename E1, typename E2, typename E3>
expr cond(E1 &&condition, E2 &&negexp, E3 &&posexp) {
	return {switchop,std::forward<E1>(condition),
		std::forward<E2>(negexp), newconst(0.0), std::forward<E3>(posexp)};
}

expr abs(const expr &e) {
	return cond(e,-e,e);
}

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

#endif
