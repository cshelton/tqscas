#ifndef POLY_H
#define POLY_H

#include "scalarrewrite.hpp"
#include <numeric>

template<typename T>
struct polyinfo : public opinfo {
	// first child: expr (must be a var) in which the polynomial is expressed (call it x)
	// next child: constant term (necessary)
	// next child: term multiplied by x
	// next child: term multiplied by x^2
	// ...
	// (perhaps need to make into a "map" if we expect many zeros in the expansion)
	polyinfo() : opinfo(0,"P",false,false,8) {}
	virtual any opeval(const std::vector<any> &x) const {
		T ret = MYany_cast<T>(x[1]);
		for(int i=2;i<x.size();i++)
			ret += pow(MYany_cast<T>(x[i]),i-1);
		return {ret};
	}
};

const op polyop = toptr<polyinfo<scalarreal>>();

// make a single term polynomial
expr makepoly(const expr &x, int degree, const expr &coef) {
	std::vector<expr> ch(degree+2,scalar(0));
	ch[0] = x;
	ch.back()=coef;
	return {polyop,std::move(ch)};
}

expr polyadd(const expr &p1, const expr &p2) {
	std::vector<expr> retch;
	const std::vector<expr> &ch1 = p1.children();
	const std::vector<expr> &ch2 = p2.children();
	retch.reserve(std::max(ch1.size(),ch2.size()));
	retch.emplace_back(ch1[0]); // assume vars match
	int i=1;
	int lastnonzero=1; // need at least the constant, even if zero
	while(i<ch1.size() && i<ch2.size()) {
		expr c = scalarruleset.rewrite(ch1[i]+ch2[i]);
		if (!isconst(c) || getconst<scalarreal>(c)!=0.0) lastnonzero = i;
		retch.emplace_back(c);
		++i;
	}
	while(i<ch1.size()) {
		lastnonzero = i;
		retch.emplace_back(ch1[i]);
		++i;
	}
	while(i<ch2.size()) {
		lastnonzero = i;
		retch.emplace_back(ch2[i]);
		++i;
	}
	retch.resize(lastnonzero+1);
	return {polyop,std::move(retch)};
}

expr polysub(const expr &p1, const expr &p2) {
	std::vector<expr> retch;
	const std::vector<expr> &ch1 = p1.children();
	const std::vector<expr> &ch2 = p2.children();
	retch.reserve(std::max(ch1.size(),ch2.size()));
	retch.emplace_back(ch1[0]); // assume vars match
	int i=1;
	int lastnonzero=1; // need at least the constant, even if zero
	while(i<ch1.size() && i<ch2.size()) {
		expr c = scalarruleset.rewrite(ch1[i]-ch2[i]);
		if (!isconst(c) || getconst<scalarreal>(c)!=0.0) lastnonzero = i;
		retch.emplace_back(c);
		++i;
	}
	while(i<ch1.size()) {
		lastnonzero = i;
		retch.emplace_back(ch1[i]);
		++i;
	}
	while(i<ch2.size()) {
		lastnonzero = i;
		retch.emplace_back(scalarruleset.rewrite(-ch2[i]));
		++i;
	}
	retch.resize(lastnonzero+1);
	return {polyop,std::move(retch)};
}


expr polymult(const expr &p1, const expr &p2) {
	std::vector<expr> retch;
	const std::vector<expr> &ch1 = p1.children();
	const std::vector<expr> &ch2 = p2.children();
	retch.reserve(ch1.size()+ch2.size()-1);
	retch.emplace_back(ch1[0]); // assume vars match
	int lastnonzero=1; // need at least the constant, even if zero
	for(int i=1;i<ch1.size();i++)
		for(int j=1;j<ch2.size();j++) {
			int k = i+j-1;
			if (k>=retch.size()) {
				expr c = scalarruleset.rewrite(ch1[i]*ch2[j]);
				if (!isconst(c) || getconst<scalarreal>(c)!=0.0) lastnonzero = k;
				retch.emplace_back(c);
			} else {
				expr c = scalarruleset.rewrite(retch[k] + ch1[i]*ch2[j]);
				if (!isconst(c) || getconst<scalarreal>(c)!=0.0) lastnonzero = k;
				retch[k] = c;
			}
		}
	retch.resize(lastnonzero+1);
	return {polyop,std::move(retch)};
}

expr polyraise(const expr &p, int i) {
	assert(i>=0);
	if (i==0) return {polyop,p.children()[0],scalar(1)};
	if (i==1) return p;
	auto phalf = polyraise(p,i/2);
	phalf = polymult(phalf,phalf);
	if (i%2==0) return phalf;
	return polymult(phalf,p);
}

struct polyrewrite : public rewriterule {
	var v;
	polyrewrite(var vv) : v(vv) { }

	// 0 = no, 1 = const, 2 = var, 3 = natively
	int ispoly(const expr &e) const {
		if (isop(e,polyop) && isvar(e.children()[0]) && getvar(e.children()[0])==v)
			return 3;
		if (isvar(e) && getvar(e)==v) return 2;
		if (isconstexpr(e,v)) return 1;
		return 0;
	}

	expr topoly(const expr &e) const {
		if (isconstexpr(e,v)) return makepoly(expr{v},0,e);
		if (isop(e,polyop) && getvar(e.children()[0])==v) return e;
		if (isvar(e) && getvar(e)==v) return makepoly(expr{v},1,scalar(1));
		assert(0);
		return e;
	}


	virtual ruleptr clone() const { return std::make_shared<rewriterule>(*this); }
	virtual std::optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		auto &ch = e.children();
		if (isop(e,powerop)) {
		    	if (ispoly(ch[0])<2) return {};
			auto ch0 = topoly(ch[0]);
			auto &pch = ch0.children();
			if (pch.size()==2) {
				if (!isconstexpr(ch[1],v)) return {};
				return std::optional<expr>{in_place,polyop,pch[0],pow(pch[1],ch[1])};
			}
		     if (!isconst(ch[1])
				|| getconstany(ch[1]).type()!=typeid(scalarreal)) return {};
			scalarreal i = getconst<scalarreal>(ch[1]);
			if (!i.isint() || i<0) return {};
			return std::optional<expr>{in_place,polyraise(ch0,i.asint())};
		}
		if (isop(e,pluschain)) {
			bool anypoly = false;
			for(auto &c : ch) {
				int isp = ispoly(c);
				if (!isp) return {};
				if (isp>=2) anypoly = true;
			}
			if (!anypoly) return {};
			if (ch.size()==1) return ch[0];
			expr ret = polyadd(topoly(ch[0]),topoly(ch[1]));
			for(int i=2;i<ch.size();i++)
				ret = polyadd(ret,topoly(ch[i]));
			return std::optional<expr>{in_place,std::move(ret)};
		}
		if (isop(e,multiplieschain)) {
			bool anypoly = false;
			for(auto &c : ch) {
				int isp = ispoly(c);
				if (!isp) return {};
				if (isp>=2) anypoly = true;
			}
			if (!anypoly) return {};
			if (ch.size()==1) return ch[0];
			expr ret = polymult(topoly(ch[0]),topoly(ch[1]));
			for(int i=2;i<ch.size();i++)
				ret = polymult(ret,topoly(ch[i]));
			return std::optional<expr>{in_place,std::move(ret)};
		}
		return {};
	}
};

bool polyiszero(const expr &e) {
	assert(isop(e.asnode(),polyop));
	auto &ch = e.children();
	return ch.size()==2 && isconst(ch[1]) && getconst<scalarreal>(ch[1])==0.0;
}

int polydegree(const expr &e) {
	assert(isop(e.asnode(),polyop));
	return e.children().size()-2;
}

expr polyvar(const expr &p) {
	return p.children().front();
}

expr polyleadingcoef(const expr &e) {
	assert(isop(e.asnode(),polyop));
	return e.children().back();
}

bool polyisunivariate(const expr &e) {
	assert(isop(e.asnode(),polyop));
	auto &ch = e.children();
	for(int i=1;i<ch.size();i++)
		if (!isconst(ch[i])) return false;
	return true;
}

bool polyisint(const expr &e) {
	assert(isop(e.asnode(),polyop));
	auto &ch = e.children();
	for(int i=1;i<ch.size();i++)
		if (!isconst(ch[i]) || !getconst<scalarreal>(ch[i]).isint()) return false;
	return true;
}

std::optional<expr> topoly(const expr &e, const expr &x) {
	assert(isvar(x));
	ruleset rs = basicscalarrules;
	auto v = getvar(x);
	rs.setvar(v);
	auto polyrr = toptr<polyrewrite>(v);
	rs.rules.emplace_back(polyrr);
	/*
	auto re = e.map([v,x](const expr &e) {
			if (isconstexpr(e,v)) return std::optional<expr>{in_place,polyop,x,e};
			if (e==x) return std::optional<expr>{in_place,polyop,x,scalar(0),scalar(1)};
			return std::optional<expr>{};
			});
	expr pe = rs.rewrite(re);
			*/
	expr pe = rs.rewrite(e);
	if (polyrr->ispoly(pe)) return std::optional<expr>{in_place,polyrr->topoly(pe)};
	return {};
}


// returns quotient (q) and remainder (r) s.t.
// q*d + r = n
// assumes n and d are polynomial expressions
std::pair<expr,expr> polydivide(const expr &n, const expr &d) {
	expr x = polyvar(n);
	expr q = makepoly(x,0,scalar(0));
	expr r = n;
	int delta;
	while (!polyiszero(r) && (delta=polydegree(r)-polydegree(d))>=0) {
		auto t = makepoly(x,delta,basicscalarrules.rewrite(polyleadingcoef(r)/polyleadingcoef(d)));
		q = polyadd(q,t);
		r = polysub(r,polymult(d,t));
	}
	return std::make_pair(q,r);
}


std::pair<expr,expr> polypseudodivide(expr n, const expr &d) {
	expr x = polyvar(n);
	expr b = polyleadingcoef(d);
	expr bk = makepoly(x,0,b);
	int N = polydegree(n) - polydegree(d) + 1;
	expr q = makepoly(x,0,scalar(0));
	int delta;
	while (!polyiszero(n) && (delta = polydegree(n)-polydegree(d))>=0) {
		//std::cout << "q = " << tostring(q) << std::endl;
		//std::cout << "r = " << tostring(n) << std::endl;
		//std::cout << "delta = " << delta << std::endl;
		expr t = makepoly(x,delta,polyleadingcoef(n));
		//std::cout << "t = " << tostring(t) << std::endl;
		//std::cout << "N = " << N << std::endl;
		--N;
		q = polyadd(polymult(bk,q),t);
		n = polysub(polymult(bk,n),polymult(t,d));
	}
	expr bN = makepoly(x,0,basicscalarrules.rewrite(pow(b,N)));
	return std::make_pair(polymult(q,bN),polymult(n,bN));
}

// from F. Winkler WS 2010/11 "Computer Algebra" "Greatest common divisors of polynomials"
expr polygcd_mod(expr a, expr b) {
	assert(polyisint(a) && polyisint(b));
	int d = std::gcd(getconst<scalarreal>(polyleadingcoef(a)).asint(),
			getconst<scalarreal>(polyleadingcoef(b)).asint());
	int M = 2*d;// TODO
}

expr polygcd(expr a, expr b) {
	while(!polyiszero(b)) {
		auto qr = polydivide(a,b);
		a = b;
		b = qr.second;
	}
	return a;
}

// Yun
std::vector<expr> sqfreedecomp(const expr &p) {
	//TODO
}

#endif
