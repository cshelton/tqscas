#ifndef SCALARREWRITE_HPP
#define SCALARREWRITE_HPP

#include "exprscalarops.hpp"
#include "exprrewrite.hpp"
#include "exprmatch.hpp"
#include <algorithm>
//#include <boost/logiclogic/tribool>
//#include "scalarrange.hpp"

template<typename E>
bool ismatchexp(const E &e) {
	if (e.isleaf()) return ismatcher(e);
	else return ismatcher(e) && e.children().size()==1
			&& ismatchexp(e.children()[0]);
}

template<typename E>
E chainpatternmod(const E &ex) {
	return ex.map([](const E &e) {
			if (ischainop(e)) {
				return std::visit([](auto &&o) {
						using O = std::decay_t<decltype(o)>;
						using BOP = typename O::baseopT;
						auto ch = e.children();
						return optional<E>
						return std::optional<E>{std::in_place,
							matchcommop<BOP,ismatcher(ch.back()),ch};
					},e.asnode().asvariant());
			} else return std::optional<E>{};
		});
}

struct matchscalarvar : public matcherbase {
	template<typename E>
	optexprmap<E> match(const E &e) const {
		return isvar(e) && 

/*
template<typename E>
scalarset<scalarreal> rangeprop(const E &e) {
	if (isconst(e)) {
		return {getconst<scalarreal>(e)};
	}
	if (isop(e,pluschain)) {
		auto &ch = e.children();
		if (ch.empty()) return {-std::numeric_limits<scalarreal>::infinity(),
					std::numeric_limits<scalarreal>::infinity()};
		auto ret = rangeprop(ch[0]);
		for(int i=1;i<ch.size();i++)
			ret = ret.combine(rangeprop(ch[i]),
					[](const range<scalarreal> &a, const range<scalarreal> &b) {
						return range<scalarreal>{a.first+b.first,
									a.second+b.second}; });
		return ret;
	}
	if (isop(e,multiplieschain)) {
		auto &ch = e.children();
		if (ch.empty()) return {-std::numeric_limits<scalarreal>::infinity(),
					std::numeric_limits<scalarreal>::infinity()};
		auto ret = rangeprop(ch[0]);
		for(int i=1;i<ch.size();i++)
			ret = ret.combine(rangeprop(ch[i]),
					[](const range<scalarreal> &a, const range<scalarreal> &b) {
						auto v1=a.first*b.first, v2=a.second*b.second,
							v3 = a.first*b.second, v4=a.second*b.first;
						return range<scalarreal>{
							std::min(std::min(v1,v2),std::min(v3,v4)),
							std::max(std::max(v1,v2),std::max(v3,v4))};
							});
		return ret;
	}
	if (isop(e,absop)) {
		return rangeprop(e.children()[0]).modify(
				[](const range<scalarreal> &a) {
					if (a.first>=0) return a;
					if (a.second<=0) return range<scalarreal>{-a.second,-a.first};
					return range<scalarreal>{scalarreal{0},std::max(-a.first,a.second)};
					});
	}
	if (isop(e,powerop)) {
		return rangeprop(e.children()[0]).combinemult(
				rangeprop(e.children()[1]),
				[](const range<scalarreal> &b, const range<scalarreal> &p) {
					scalarset<scalarreal> ret;
					limpt<scalarreal> zero(scalarreal{0});
					if (b.first>zero) {
						auto v1=pow(b.first,p.first),
							v2=pow(b.second,p.second),
							v3=pow(b.first,p.second),
							v4=pow(b.second,p.first);
						ret.x.emplace(
							std::min(std::min(v1,v2),std::min(v3,v4)),
							std::max(std::max(v1,v2),std::max(v3,v4)));
						return ret;
					}
					if (b.second>zero) {
						auto v1=pow(zero,p.first),
							v2=pow(b.second,p.second),
							v3=pow(zero,p.second),
							v4=pow(b.second,p.first);
						v1.closed = v3.closed = false;
						ret.x.emplace(
							std::min(std::min(v1,v2),std::min(v3,v4)),
							std::max(std::max(v1,v2),std::max(v3,v4)));
					}
					// what to do with negative base???
					if (p.first.pt==p.second.pt) { // single exponent
						if (p.first.pt.isint()) {
							auto v1 = pow(zero,p.first);
							auto v2 = pow(b.first,p.first);
							ret.x.emplace(v1,v2);
						}
					} else {
						limpt<scalarreal> b2 = b.second.pt>=zero ? zero :
								(b.second.pt.isint() ? b.second :
									(limpt<scalarreal>{
										b.first.pt.floor()}));
						limpt<scalarreal> b1 = b2-limpt<scalarreal>{scalarreal{1}};
						if (b1>b.first) {
							auto v1 = pow(b2,p.first);
							auto v2 = pow(b1,p.first);
							auto v3 = pow(b2,p.second);
							auto v4 = pow(b1,p.second);
							ret.x.emplace(
								std::min(std::min(v1,v2),
									std::min(v3,v4)),
								std::max(std::max(v1,v2),
									std::max(v3,v4)));
						} else {
							auto v1 = pow(b2,p.first);
							auto v3 = pow(b2,p.second);
							ret.x.emplace(v1,v3);
						}
					}
					// not sure why this was the code... seems very wrong
					// p not even mentioned!
				});
	}
	if (isop(e,logop)) {
		return rangeprop(e.children()[0]).modify(
				[](range<scalarreal> a) {
					a.first.pt = log(a.first.pt);
					a.second.pt = log(a.second.pt);
					return a;
					});
	}
	if (isop(e,heavisideop)) {
		return rangeprop(e.children()[0]).modify(
				[](const range<scalarreal> &a) {
					return range<scalarreal>{a.first < 0 ? 0 : 1,
								a.second < 0 ? 0 : 1};
					return a;
				});
	}
	if (isop(e,diracop)) {
		return rangeprop(e.children()[0]).modify(
				[](const range<scalarreal> &a) {
					return range<scalarreal>{scalarreal{0},
						a.overlap(range<scalarreal>(0,0)) ?
							std::numeric_limits<scalarreal>::infinity()
							: scalarreal{0}};
				});
	}
	return {-std::numeric_limits<scalarreal>::infinity(),
					std::numeric_limits<scalarreal>::infinity()};
}


// perhaps these should do their own simplification first...

bool ispos(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first<=0) return false;
	return true;
}
bool isneg(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.second>=0) return false;
	return true;
}
bool isnonneg(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first<0) return false;
	return true;
}
bool isnonpos(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.second>0) return false;
	return true;
}
bool isconst(const expr &e, scalarreal k) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first!=k || r.second!=k) return false;
	return true;
}
bool iseven(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first!=r.second || !r.first.pt.iseven()) return false;
	return true;
}
bool isodd(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first!=r.second || !r.first.pt.isodd()) return false;
	return true;
}

struct simpcond : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<simpcond>(*this); }
	virtual optional<expr> apply(const expr &e) const {
		if (!isop(e,condop) && !isop(e,condeqop)) return {};
		auto rngset = rangeprop(e.children()[0]);
		if (isop(e,condop)) {
			bool hasless=false, hasgreq=false;
			for(auto &rng : rngset.x) {
				if (rng.first<0) {
					hasless=true;
					if (hasgreq) break;
				}
				if (rng.second>=0) {
					hasgreq=true;
					if (hasless) break;
				}
			}
			if (!hasgreq) return optional<expr>{in_place,e.children()[1]};
			if (!hasless) return optional<expr>{in_place,e.children()[2]};
			return {};
		} else {
			bool hasnotzero=false, haszero=false;
			for(auto &rng : rngset.x) {
				if (rng.first!=0 || rng.second!=0) {
					hasnotzero=true;
					if (haszero) break;
				}
				if (rng.first<=0 && rng.second>=0) {
					haszero=true;
					if (hasnotzero) break;
				}
			}
			if (!hasnotzero) return optional<expr>{in_place,e.children()[1]};
			if (!haszero) return optional<expr>{in_place,e.children()[2]};
			return {};
		}
	}
};

*/

/*
struct normswitch : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<normswitch>(*this); }
	virtual optional<expr> apply(const expr &e) const {
		if (!isop(e,switchop)) return {};
		auto &c0 = e.children()[0];
		expr tosub = scalar(0);
		if (c0.isleaf()) {
			if (!isconst(c0) || getconst<scalarreal>(c0)==0)
				return {};
			tosub = c0;
		} else {
			if (!isop(c0,pluschain) || c0.children().empty()
					|| !isconst(c0.children()[0])
					|| getconst<scalarreal>(c0.children()[0])==0)
				return {};
			tosub = c0.children()[0];
		}
		std::vector<expr> newch(e.children());
		for (int i=0;i<newch.size();i+=2)
			newch[i] = newch[i] - tosub;
		return optional<expr>{in_place,e.asnode(),newch};
	}
};

struct liftswitch : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<liftswitch>(*this); }
	static expr replacech(const expr &e, int chi, const expr &ch) {
		auto &ech = e.children();
		std::vector<expr> newch;
		newch.reserve(ech.size());
		for(int i=0;i<ech.size();i++)
			if (i==chi) newch.emplace_back(ch);
			else newch.emplace_back(ech[i]);
		return {e.asnode(),newch};
	}

	virtual optional<expr> apply(const expr &e) const {
		if (e.isleaf() || e.asnode()==switchop) return {};
		auto &ch = e.children();
		for(int i=0;i<ch.size();i++)
			if (isop(ch[i],switchop)) {
				auto &sch = ch[i].children();
				std::vector<expr> newch;
				for(int j=0;j<sch.size();j+=2) {
					newch.emplace_back(sch[j]);
					newch.emplace_back(replacech(e,i,sch[j+1]));
				}
				return optional<expr>{in_place,switchop,newch};
			}
		return {};
	}
};

struct squeezeswitch : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<squeezeswitch>(*this); }
	virtual optional<expr> apply(expr &e) const {
		if (!isop(e,switchop)) return {};
		auto &ch = e.children();
		if (ch.size()<5) {
			if (ch[1]==ch[3]) // technically, this might expand
				// the domain (if ch[0] is undefined in places)
				return optional<expr>{in_place,ch[1]};
			return {};
		}
		for(int i=2;i<ch.size()-1;i+=2)
			if (ch[i-1]==ch[i+1]) {
				std::vector<expr> newch;
				newch.reserve(ch.size()-2);
				for(int j=0;j<i;j++) newch.emplace_back(ch[j]);
				for(int j=i+2;j<ch.size();j++) newch.emplace_back(ch[j]);
				return optional<expr>{in_place,e.asnode(),newch};
			}
		return {};
	}
};


struct mergeswitch : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<mergeswitch>(*this); }
	static bool ismergeable(const expr &e, const expr &te, bool checkte=true) {
		if (!isop(e,switchop)) return false;
		auto &ch = e.children();
		if (checkte && !(ch[0]==te)) return false;
		scalarreal lastk = 0;
		for(int i=2;i<ch.size();i+=2) {
			if (!isconst(ch[i])) return false;
			if (i>2) {
				scalarreal newk = getconst<scalarreal>(ch[i]);
				if (newk<lastk) return false;
				lastk = newk;
			}
		}
		return true;
	}

	virtual optional<expr> apply(expr &e) const {
		if (!ismergeable(e,e,false)) return {};
		auto &ch = e.children();
		int mchi = -1;
		scalarreal preth = -std::numeric_limits<scalarreal>::infinity();
		scalarreal postth = std::numeric_limits<scalarreal>::infinity();
		for(int i=1;i<ch.size();i+=2)
			if (ismergeable(ch[i],ch[0])) {
				mchi = i;
				if (i>1) preth = getconst<scalarreal>(ch[i-1]);
				if (i+1 < ch.size()) postth = getconst<scalarreal>(ch[i+1]);
				break;
			}
		if (mchi==-1) return {};

		auto &mch = ch[mchi].children();
		std::vector<expr> newch;
		for(int i=0;i<mchi;i++)
			newch.emplace_back(ch[i]);
		newch.reserve(ch.size()+mch.size()-2);
		for(int i=1;i<mch.size();i+=2) {
			scalarreal th = (i+1<mch.size() ? getconst<scalarreal>(mch[i+1])
							: std::numeric_limits<scalarreal>::infinity());
			if (th > preth) {
				newch.emplace_back(mch[i]);
				if (th < postth && i+1<mch.size()) newch.emplace_back(mch[i+1]);
				else break;
			}
		}
		for(int i=mchi+1;i<ch.size();i++)
			newch.emplace_back(ch[i]);
		return optional<expr>{in_place,e.asnode(),newch};
	}
};
*/

template<typename E>
ruleptr SRR(const E &s, const E &p) {
	return SR(chainpatternmod(s),p);
}

template<typename E, typename F>
ruleptr SRR(const E &s, const E &p, F &&f) {
	return SR(chainpatternmod(s),p,std::forward<F>(f));
}

/*
template<typename T>
struct bigopexpand : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<bigopexpand>(*this); }
	int nterms;
	op bigop, chainop;
	T id;

	bigopexpand(int n, op bop, op cop, T i) : nterms(n), bigop(bop), chainop(cop), id(i) {}

	virtual optional<expr> apply(const expr &e) const {
		if (!isop(e,bigop)) return {};
		auto &ch = e.children();
		if (!isconst(ch[2]) || !isconst(ch[3])) return {};
		scalarreal x0 = getconst<scalarreal>(ch[2]);
		scalarreal x1 = getconst<scalarreal>(ch[3]);
		if (x1 < x0) return optional<expr>{in_place,scalar(id)};
		if (x1 >= x0+nterms) return {};
		std::vector<expr> terms;
		for(scalarreal x=x0;x<=x1;x+=1)
			terms.emplace_back(substitute(ch[1],ch[0],scalar(x)));
		return optional<expr>{in_place,chainop,terms};
	}
};
*/

/*
expr x_ = newvar<scalarreal>("x_");
expr notx_{makematchleaf<matchconstwrt>(vset{{getvar(x_)}})};
expr k1_ = L(1,notx_);
expr k2_ = L(2,notx_);
expr k3_ = L(3,notx_);
expr k4_ = L(4,notx_);
expr k5_ = L(5,notx_);
expr k6_ = L(6,notx_);
expr k7_ = L(7,notx_);
expr k8_ = L(8,notx_);
expr k9_ = L(9,notx_);
*/

/*
struct tableintegrate : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<tableintegrate>(*this); }
	std::vector<std::pair<expr,expr>> antiderivs;

	tableintegrate(std::vector<std::pair<expr,expr>> ader)
			: antiderivs(std::move(ader)) {}

	virtual optional<expr> apply(const expr &e) const {
		if (!isop(e,integrateop)) return {};
		auto &ch = e.children();
		expr newint = substitute(ch[1],ch[0],x_);
		for(auto &ad : antiderivs) {
			auto res = match(newint,ad.first);
			if (res) {
				expr ader = substitute(ad.second,*res);
				expr ret = substitute(ader,x_,ch[3])
					- substitute(ader,x_,ch[2]);
				return optional<expr>{in_place,std::move(ret)};
			}
		}
		return {};
	}
};
*/

/*
template<typename E1, typename E2>
std::pair<expr,expr> ADR(E1 &&e, E2 &&ad) {
	return std::make_pair(chainpatternmod(std::forward<E1>(e)),
			std::forward<E2>(ad));
}

std::vector<std::pair<expr,expr>> stdantiderivs
	{{
		 ADR(  k1_,              
	            P1_*x_   ),

		 ADR(  x_,
		       x_*x_/2   ),

		 // TODO:
		 // when k1_<0 this doesn't work if the integral goes over the pole (@ x_=0)
		 // similar problem with the other rational function antiderivatives below!
		 ADR(  pow(x_,k1_),
			  ifeqthenelse(P1_+1,log(abs(x_)),pow(x_,P1_+1)/(P1_+1))   ),

		 ADR(  pow(x_ + k2_,k3_),
			  ifeqthenelse(P3_+1, log(abs(x_+P2_)),
							pow(x_+P2_,P3_+1)/(P3_+1))),

		 ADR(  pow(k1_*x_ + k2_,k3_),
			  ifeqthenelse(P1_,pow(P2_,P3_)*x_,
				  ifeqthenelse(P3_+1, log(abs(P1_*x_+P2_))/P1_,
					  			pow(P1_*x_+P2_,P3_+1)/(P3_+1)/P1_))),

		 // added to make example in proposal work... needs to be
		 // more general
		 ADR(  pow(k1_,k2_*x_),
				 ifeqthenelse(P2_,x_,
					 pow(P1_,P2_*x_)/(P2_*log(P1_)))),

	 }};
	 */

/*
// all of the next few "helpers" should perhaps be moved elsewhere
// they may also need to be made more efficient and perhaps given
// their own algebraic symbols to be reasonable about directly
// (for instance, we probably don't want to be evaluating "factorial"
//  or "choose" directly)
expr factorial(const expr &n) {
	expr i = newvar<scalarreal>(); 
	return prod(i,i,scalar(1),n);
}

expr nchoosek(const expr &n, const expr k) {
	return factorial(n)/(factorial(k)*factorial(n-k));
}

expr B0(const expr &m) {
	expr k=newvar<scalarreal>(), v=newvar<scalarreal>();

	// pos series
	//return sum(sum(pow(scalar(-1),v)*nchoosek(k,v)*pow(v+1,m)/(k+1),v,scalar(0),k),k,scalar(0),m);
	// neg series
	return sum(sum(pow(scalar(-1),v)*nchoosek(k,v)*pow(v,m)/(k+1),v,scalar(0),k),k,scalar(0),m);
}

// Bernoulli polynomial
expr B(const expr &n, const expr &m) {
	expr k = newvar<scalarreal>();
	return sum(nchoosek(n,k)*B0(n-k)*pow(m,k),k,scalar(0),n);
}

expr psum(const expr &p, const expr &n) {
	expr k = newvar<scalarreal>();
	return sum(nchoosek(p,k)*B0(p-k)*pow(-1,p-k)/(k+1)*pow(n,k+1),k,scalar(0),p);
}
*/

// TODO:  will need to be separated out into general and specific to scalars
//std::vector<ruleptr>

template<typename E>
ruleset basicscalarrules
	{{
  SRR(E1_ - E2_                          ,  P1_ + -1*P2_                   ),
  SRR(-E1_                               ,  -1*P1_                         ),

  SRR(E1_ / E2_                          ,  P1_ * pow(P2_,-1)              ),

  // some of these are only true "almost everywhere"
  // and might need to be removed for some applications
  // (or, we need a "domain" to be propagated with the expr)
  //
  SRR( 0 + E1_                          ,  P1_                             ),
  SRR( -0 + E1_                          ,  P1_                             ),
  SRR( 1 * E1_                          ,  P1_                             ),
  SRR( 0 * E1_                          ,  scalar(0)                   ),
  SRR( -0 * E1_                          ,  scalar(0)                   ),
  SRR( pow(E1_,1)                       ,  P1_                             ),
  SRR( pow(1,E1_)                       ,  scalar(1)                   ),
  SRR( pow(0,E1_)                       ,  ifthenelse(P1_,
				scalar(std::numeric_limits<scalarreal>::infinity()),
				scalar(0))                                           ),
  SRR( pow(E1_,0)                       ,  scalar(1)                   ),

  SRR( E1_ + E1_                          ,  2*P1_                           ),
  SRR( E1_ + E1_ + E2_                    ,  2*P1_ + P2_                     ),

  SRR( E1_ * E1_                          ,  pow(P1_,2)                    ),
  SRR( E1_ * E1_ * E2_                    ,  pow(P1_,2)*P2_                ),

  SRR( log(pow(E1_,E2_))                  ,  P2_*log(P1_)                    ),
  SRR( log(E1_*E2_)                       ,  log(P1_) + log(P2_)             ),

  SRR( (W2_ + W3_)*E1_                    ,  P1_*P2_ + P1_*P3_               ),
  SRR( (K2_ + W3_)*E1_                    ,  P1_*P2_ + P1_*P3_               ),
  SRR( (W2_ + K3_)*E1_                    ,  P1_*P2_ + P1_*P3_               ),

  SRR( K1_*E2_ + K3_*E2_                  ,  (P1_+P3_) * P2_                 ),
  SRR( K1_*E2_ + K3_*E2_ + E4_            ,  (P1_+P3_) * P2_ + P4_           ),
  SRR( E2_ + K3_*E2_                      ,  (scalar(1)+P3_) * P2_       ),
  SRR( E2_ + K3_*E2_ + E4_                ,  (scalar(1)+P3_) * P2_ + P4_ ),

  SRR( pow(E1_,E2_) * pow(E1_,E3_)        ,  pow(P1_,P2_+P3_)                ),
  SRR( pow(E1_,E2_) * pow(E1_,E3_) * E4_  ,  pow(P1_,P2_+P3_)*P4_            ),
  SRR( E1_ * pow(E1_,E3_)                 ,  pow(P1_,scalar(1)+P3_)      ),
  SRR( E1_ * pow(E1_,E3_) * E4_           ,  pow(P1_,scalar(1)+P3_)*P4_  ),

  SRR( pow(pow(W1_,E2_),E3_)              ,  pow(P1_,P2_*P3_)                ),
  SRR( pow(pow(K1_,W2_),W3_)              ,  pow(P1_,P2_*P3_)                ),
  SRR( pow(pow(K1_,W2_),K3_)              ,  pow(pow(P1_,P3_),P2_)           ),
  SRR( pow(K1_,K2_*E3_)                   ,  pow(pow(P1_,P2_),P3_)           ),

  SRR( pow(E1_*W2_,E3_)                   ,  pow(P1_,P3_)*pow(P2_,P3_)       ),
  SRR( pow(W1_*E2_,E3_)                   ,  pow(P1_,P3_)*pow(P2_,P3_)       ),
  SRR( pow(K1_,E3_)*pow(K2_,E3_)          ,  pow(P1_*P2_,P3_)                ),


  SRR( log(ifthenelse(E1_,E2_,E3_))    ,  ifthenelse(P1_,log(P2_),log(P3_))  ),

  SRR( abs(E1_)                           ,  P1_ ,
   [](const exprmap &m) { return isnonneg(m.at(1)); } ),

  SRR( abs(E1_)                           ,  -P1_ ,
   [](const exprmap &m) { return isnonpos(m.at(1)); } ),
	 }};

ruleset derivscalarrules{{
  SRR( deriv(E1_,V2_,V3_)                 ,  scalar(0),
      [](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(2))); }),
  SRR( deriv(V1_,V1_,V3_)                 ,  scalar(1)                   ),

  SRR( deriv(E1_+E2_,V3_,V4_)             ,
		                           deriv(P1_,P3_,P4_) + deriv(P2_,P3_,P4_) ),

  SRR( deriv(E1_*E2_,V3_,V4_)             ,
		     deriv(P1_,P3_,P4_)*evalat(P2_,P3_,P4_)
		  + evalat(P1_,P3_,P4_)* deriv(P2_,P3_,P4_)                        ),

  SRR( deriv(pow(E1_,E2_),V3_,V4_)        ,
	  evalat(pow(P1_,P2_),P3_,P4_)*evalat(log(P1_),P3_,P4_)*deriv(P2_,P3_,P4_)
   + evalat(P2_,P3_,P4_)*evalat(pow(P1_,P2_-1),P3_,P4_)*deriv(P1_,P3_,P4_) ),

  SRR( deriv(log(E1_),V2_,V3_)            ,
		                          deriv(P1_,P2_,P3_) / evalat(P1_,P2_,P3_) ),

  SRR( deriv(expr{heavisideop,E1_},V2_,V3_),
				 evalat(expr{diracop,P1_},P2_,P3_)*deriv(P1_,P2_,P3_) ),

  // not sure this is right with diracop
  SRR( deriv(ifthenelse(E1_,E2_,E3_),V4_,V5_)   ,
		  evalat(expr{diracop,P1_},P4_,P5_)*deriv(P1_,P4_,P5_)*(E2_-E3_)
		  + ifthenelse(P1_,deriv(P2_,P4_,P5_),deriv(P3_,P4_,P5_))           ),

  SRR( deriv(ifeqthenelse(E1_,E2_,E3_),V4_,V5_)  ,
		  ifeqthenelse(evalat(P1_,P4_,P5_),
			  deriv(P2_,P4_,P5_),deriv(P3_,P4_,P5_))                       ),

  SRR( deriv(integrate(E1_,E2_,E3_,E4_),E5_,E6_)  ,
		  integrate(deriv(P1_,P5_,P6_),P2_,
					evalat(P3_,P5_,P6_),evalat(P4_,P5_,P6_))
		  + deriv(P4_,P5_,P6_)*evalat(evalat(P1_,P2_,P4_),P5_,P6_)
		  - deriv(P3_,P5_,P6_)*evalat(evalat(P1_,P2_,P3_),P5_,P6_)          ),
}};

ruleset integralscalarrules {{
  SRR( integrate(E1_+E2_,E3_,E4_,E5_), integrate(P1_,P3_,P4_,P5_) + integrate(P2_,P3_,P4_,P5_) ),

  SRR( integrate(E1_*E2_,E3_,E4_,E5_), P1_ * integrate(P2_,P3_,P4_,P5_) ,
  		[](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(3))); } ),

  SRR( integrate(E1_*E2_,E3_,E4_,E5_), integrate(P1_,P3_,P4_,P5_) * P2_ ,
  		[](const exprmap &m) { return isconstexpr(m.at(2),getvar(m.at(3))); } ),

  std::make_shared<tableintegrate>(stdantiderivs),

  SRR( sum(E1_,V2_,E3_,E4_)               , scalar(0),
		  [](const exprmap &m) { return isneg(E4_-E3_); } ),
  SRR( prod(E1_,V2_,E3_,E4_)               , scalar(1),
		  [](const exprmap &m) { return isneg(E4_-E3_); } ),

  // perhaps need "max(0,P4_-P3_+1)"??
  SRR( sum(E1_,V2_,E3_,E4_)               , P1_*(P4_-P3_+1),
      [](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(2))); }   ),

  SRR( sum(E1_*E2_,V3_,E4_,E5_)           , P1_*sum(P2_,P3_,P4_,P5_),
	 [](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(3))); }   ),
  SRR( sum(E1_*E2_,V3_,E4_,E5_)           , sum(P1_,P3_,P4_,P5_)*P2_,
	 [](const exprmap &m) { return isconstexpr(m.at(2),getvar(m.at(3))); }   ),
}};

ruleset sumprodscalarrules {{
  std::make_shared<bigopexpand<scalarreal>>(3,sumop,pluschain,0),
  std::make_shared<bigopexpand<scalarreal>>(3,prodop,multiplieschain,1),

  SRR( sum(V2_,V2_,E3_,E4_)      , ifthenelse(P4_-P3_,scalar(0),(P4_+1-P3_)*(P4_+P3_)/2)                  ),
  SRR( sum((V2_+E1_),V2_,E3_,E4_) , ifthenelse(P4_-P3_,scalar(0),(P4_+1-P3_)*(P4_+P3_+2*P1_)/2)              ,
	  [](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(2))); } ),

  SRR( sum(pow(V2_,2),V2_,E3_,E4_)  
	  , ifthenelse(P4_-P3_,scalar(0),scalarreal{1,6}*(P4_-P3_+1) + scalarreal{1,2}*(pow(P4_,2)-pow(P3_-1,2))
	  		+ scalarreal{1,3}*(pow(P4_,3)-pow(P3_-1,3)))                   ),
  SRR( sum(pow(V2_+E1_,2),V2_,E3_,E4_)  
	  , ifthenelse(P4_-P3_,scalar(0),scalarreal{1,6}*(P4_-P3_+1) + scalarreal{1,2}*(pow(P4_,2)-pow(P3_-1,2))
	  		+ scalarreal{1,3}*(pow(P4_,3)-pow(P3_-1,3))
			+ pow(P1_,2)*(P4_-P3_) + P1_*(pow(P4_,2)-pow(P3_,2))          ),
	  [](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(2))); } ),

  // Faulhaber's formula
  SRR( sum(pow(V1_,E2_),V1_,E3_,E4_)      ,
	//	 			(B(P2_+1,P4_+1) - B(P2_+1,P3_+1))/(P2_+1) ,
		ifthenelse(P4_-P3_,scalar(0),psum(P2_,P4_) - psum(P2_,P3_-1)),
  	[](const exprmap &m) { return isconstexpr(m.at(2),getvar(m.at(1))); }   ),
  SRR( sum(pow((V1_+E5_),E2_),V1_,E3_,E4_)      ,
	//	 			(B(P2_+1,P4_+1) - B(P2_+1,P3_+1))/(P2_+1) ,
		ifthenelse(P4_-P3_,scalar(0),psum(P2_,P4_+P5_) - psum(P2_,P3_+P5_-1)),
  	[](const exprmap &m) { return isconstexpr(m.at(2),getvar(m.at(1)))
				&& isconstexpr(m.at(5),getvar(m.at(1))); }   ),
}};

ruleset numericevalrules {{
		  std::make_shared<consteval>(),
	 }};

ruleset scalarruleset = basicscalarrules
			+ derivscalarrules
			+ integralscalarrules
			+ sumprodscalarrules
			+ numericevalrules;
#endif
