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
	return {plusop,e1,e2};
}
expr operator+(const double e1, const expr &e2) {
	return {plusop,newconst(e1),e2};
}
expr operator+(const expr &e1, const double &e2) {
	return {plusop,e1,newconst(e2)};
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
	return {multipliesop,e1,e2};
}
expr operator*(const double e1, const expr &e2) {
	return {multipliesop,newconst(e1),e2};
}
expr operator*(const expr &e1, const double &e2) {
	return {multipliesop,e1,newconst(e2)};
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
	return {powerop,scalare,e};
}

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


const std::shared_ptr<opchain> pluschain = std::make_shared<opchain>(plusop);
const std::shared_ptr<opchain> multiplieschain = std::make_shared<opchain>(multipliesop);

bool isconst(const expr &e1) {
	return e1.isleaf() && e1.asleaf().type()==typeid(double);
}

bool isconstwrt(const expr &e1) {
	return true;
}

template<typename... Ts>
bool isconstwrt(const expr &e1, const expr &v, Ts &&...vs) {
	if (v==allvars) return isconst(e1);
	if (e1.isleaf()) {
		if (e1==v) return false;
		return isconstwrt(e1,std::forward<Ts>(vs)...);
	}
	for(auto &c : e1.children())
		if (!isconstwrt(c,v,std::forward<Ts>(vs)...)) return false;
	return true;
}

bool isconstwrt(const expr &e1, const std::vector<expr> &vs) {
	for(auto &v : vs) if (v==allvars) return isconst(e1);
	if (e1.isleaf()) {
		for(auto &v : vs) if (e1==v) return false;
		return true;
	}
	for(auto &c : e1.children())
		if (!isconstwrt(c,vs)) return false;
	return true;
}

#endif
