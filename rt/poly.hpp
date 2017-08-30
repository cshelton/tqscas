#ifndef POLY_H
#define POLY_H

#include "scalarrewrite.hpp"

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
		if (isconstexpr(e,v)) return {polyop,expr{v},e};
		if (isop(e,polyop) && getvar(e.children()[0])==v) return e;
		if (isvar(e) && getvar(e)==v)
			return {polyop,expr{v},scalar(0),scalar(1)};
		assert(0);
		return e;
	}


	virtual ruleptr clone() const { return std::make_shared<rewriterule>(*this); }
	virtual optional<expr> apply(const expr &e) const {
		//if (isconstexpr(e,getvar(v))) return optional<expr>{in_place,polyop,v,e};
		//if (v==e) return optional<expr>{in_place,polyop,v,scalar(0),scalar(1)};
		if (e.isleaf()) return {};
		auto &ch = e.children();
		if (isop(e,powerop)) {
		    	if (ispoly(ch[0])<2) return {};
			auto ch0 = topoly(ch[0]);
			auto &pch = ch0.children();
			if (pch.size()==2) {
				if (!isconstexpr(ch[1],v)) return {};
				return optional<expr>{in_place,polyop,pch[0],pow(pch[1],ch[1])};
			}
		     if (!isconst(ch[1])
				|| getconstany(ch[1]).type()!=typeid(scalarreal)) return {};
			scalarreal i = getconst<scalarreal>(ch[1]);
			if (!i.isint() || i<0) return {};
			return optional<expr>{in_place,polyraise(ch0,i.asint())};
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
			return optional<expr>{in_place,std::move(ret)};
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
			return optional<expr>{in_place,std::move(ret)};
		}
		return {};
	}
};

optional<expr> topoly(const expr &e, const expr &x) {
	assert(isvar(x));
	ruleset rs = basicscalarrules;
	auto v = getvar(x);
	rs.setvar(v);
	auto polyrr = toptr<polyrewrite>(v);
	rs.rules.emplace_back(polyrr);
	/*
	auto re = e.map([v,x](const expr &e) {
			if (isconstexpr(e,v)) return optional<expr>{in_place,polyop,x,e};
			if (e==x) return optional<expr>{in_place,polyop,x,scalar(0),scalar(1)};
			return optional<expr>{};
			});
	expr pe = rs.rewrite(re);
			*/
	expr pe = rs.rewrite(e);
	if (polyrr->ispoly(pe)) return optional<expr>{in_place,polyrr->topoly(pe)};
	return {};
}

#endif
