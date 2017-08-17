#ifndef SCALARREWRITE_HPP
#define SCALARREWRITE_HPP

#include "scalarbase.hpp"
#include "exprrewrite.hpp"
#include "exprmatch.hpp"
#include <algorithm>
//#include <boost/logiclogic/tribool>
#include "scalarrange.hpp"

bool isgenexp(const expr &e) {
	if (e.isleaf()) {
		if (e.asleaf().type()==typeid(matchleaf)) {
			matchleaf ml = MYany_cast<matchleaf>(e.asleaf());
			if (std::dynamic_pointer_cast<matchany>(ml)
			    || std::dynamic_pointer_cast<matchvar>(ml)
			    || std::dynamic_pointer_cast<matchconstwrt>(ml)
			    || std::dynamic_pointer_cast<matchnonconstwrt>(ml))
				return true;
		}
		return false;
	}
	auto n = std::dynamic_pointer_cast<matchlabelop>(e.asnode());
	if (!n) return false;
	if (e.children().size()!=1) return false;
	return isgenexp(e.children()[0]);
}


expr chainpatternmod(const expr &e) {
	return e.map([](const expr &e) {
			if (e.isleaf()) return optional<expr>{};
			auto n = e.asnode();
			if (n==pluschain || n==multiplieschain) {
				auto bop = std::dynamic_pointer_cast<opchain>(n)->baseop;
				auto ch = e.children();
				auto last = ch.back();
				if (isgenexp(last))
					return optional<expr>{in_place,std::make_shared<matchassocop>(std::make_shared<matchremainderop>(bop)),
							ch};
				else return optional<expr>{in_place,std::make_shared<matchassocop>(bop),
							ch};
			}
			return optional<expr>{};
		});
}

struct sortchildren : public rewriterule {
	optional<vset> vars;
	std::vector<op> cops;

	sortchildren(std::vector<op> comops)
		: cops(comops), vars{} { }
	sortchildren(std::vector<op> comops, vset v)
		: cops(comops), vars(in_place,std::move(v)) {}

	int typeorder(const expr &e) const {
		// TODO: use map/unordered_map
		if (isconst(e)) return 0;
		int add = (isconstexpr(e,vars) ? 0 : 9);
		if (e.isleaf()) return 1+add;
		auto &op = e.asnode();
		if (op==plusop || op==pluschain) return 2+add;
		if (op==multipliesop || op==multiplieschain) return 3+add;
		if (op==powerop) return 4+add;
		if (op==logop) return 5+add;
		//if (op==switchop) return 6+add;
		if (op==absop) return 6+add;
		if (op==derivop) return 7+add;
		if (op==evalatop) return 8+add;
		return 9+add;
	}

	int secondordering(const expr &e1, const expr &e2) const {
		if (isconst(e1)) {
			double v1 = getconst<double>(e1);
			double v2 = getconst<double>(e2);
			if (v1<v2) return -1;
			if (v2>v1) return +1;
			return 0;
		}
		if (isvar(e1))
			return MYany_cast<var>(e1.asleaf())->name
					.compare(MYany_cast<var>(e2.asleaf())->name);
		if (e1.isleaf()) return 0;
		auto &ch1 = e1.children();
		auto &ch2 = e2.children();
		for(int i=std::min(ch1.size(),ch2.size())-1;i>=0;i--) {
			int res = exprcmp(ch1[i],ch2[i]);
			if (res!=0) return res;
		}
		return ch1.size()-ch2.size();
	}

	int exprcmp(const expr &e1, const expr &e2) const {
		int o1 = typeorder(e1), o2 = typeorder(e2);
		if (o1!=o2) return o1-o2;
		return secondordering(e1,e2);
	}

	virtual optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		if (std::find(cops.begin(),cops.end(),e.asnode())==cops.end())
			return {};
		auto &ch = e.children();
		if (ch.size()<2) return {};
		for(int i=1;i<ch.size();i++)
			if (exprcmp(ch[i-1],ch[i])>0) {
				std::vector<expr> che = ch;
				std::sort(che.begin(),che.end(),
						[this](const expr &e1, const expr &e2) {
							return exprcmp(e1,e2)<0;
						}
				);
				return optional<expr>{in_place,e.asnode(),che};
			}
		return {};
	}
};

scalarset<double> rangeprop(const expr &e) {
	if (isconst(e)) {
		return {getconst<double>(e)};
	}
	if (isop(e,pluschain)) {
		auto &ch = e.children();
		if (ch.empty()) return {-std::numeric_limits<double>::infinity(),
					std::numeric_limits<double>::infinity()};
		auto ret = rangeprop(ch[0]);
		for(int i=1;i<ch.size();i++)
			ret = ret.combine(rangeprop(ch[i]),
					[](const range<double> &a, const range<double> &b) {
						return range<double>{a.first+b.first,
									a.second+b.second}; });
		return ret;
	}
	if (isop(e,multiplieschain)) {
		auto &ch = e.children();
		if (ch.empty()) return {-std::numeric_limits<double>::infinity(),
					std::numeric_limits<double>::infinity()};
		auto ret = rangeprop(ch[0]);
		for(int i=1;i<ch.size();i++)
			ret = ret.combine(rangeprop(ch[i]),
					[](const range<double> &a, const range<double> &b) {
						auto v1=a.first*b.first, v2=a.second*b.second,
							v3 = a.first*b.second, v4=a.second*b.first;
						return range<double>{
							std::min(std::min(v1,v2),std::min(v3,v4)),
							std::max(std::max(v1,v2),std::max(v3,v4))};
							});
		return ret;
	}
	if (isop(e,absop)) {
		return rangeprop(e.children()[0]).modify(
				[](const range<double> &a) {
					if (a.first>=0.0) return a;
					if (a.second<=0.0) return range<double>{-a.second,-a.first};
					return range<double>{0.0,std::max(-a.first,a.second)};
					});
	}
	if (isop(e,powerop)) {
		return rangeprop(e.children()[0]).combinemult(
				rangeprop(e.children()[1]),
				[](const range<double> &b, const range<double> &p) {
					scalarset<double> ret;
					if (b.first>0.0) {
						auto v1=std::pow(b.first,p.first),
							v2=std::pow(b.second,p.second),
							v3=std::pow(b.first,p.second),
							v4=std::pow(b.second,p.first);
						ret.x.emplace(
							std::min(std::min(v1,v2),std::min(v3,v4)),
							std::max(std::max(v1,v2),std::max(v3,v4)));
						return ret;
					}
					if (b.second>0.0) {
						auto v1=std::pow(0.0,p.first),
							v2=std::pow(b.second,p.second),
							v3=std::pow(0.0,p.second),
							v4=std::pow(b.second,p.first);
						ret.x.emplace(
							std::min(std::min(v1,v2),std::min(v3,v4)),
							std::max(std::max(v1,v2),std::max(v3,v4)));
					}
					for(int ip = b.first.closed
								|| std::fmod(b.first.pt,2.0)!=0.0 ?
							(int)std::ceil(b.first.pt/2.0)*2
							: (int)std::ceil(b.first.pt/2.0+1.0)*2;
							ip<0 && ip<b.second;ip+=2)
						ret.x.emplace(
							b.first<=0.0 && b.second>=0.0 ?
								0.0 : std::pow(
									std::min(std::abs(b.first),
										std::abs(b.second)),ip),
							std::pow(std::max(std::abs(b.first),
									std::abs(b.second)),ip));
				});
	}
	if (isop(e,logop)) {
		return rangeprop(e.children()[0]).modify(
				[](range<double> a) {
					a.first.pt = ::log(a.first.pt);
					a.second.pt = ::log(a.second.pt);
					return a;
					});
	}
	if (isop(e,heavisideop)) {
		return rangeprop(e.children()[0]).modify(
				[](const range<double> &a) {
					return range<double>{a.first < 0.0 ? 0.0 : 1.0,
								a.second < 0.0 ? 0.0 : 1.0};
					return a;
				});
	}
	if (isop(e,diracop)) {
		return rangeprop(e.children()[0]).modify(
				[](const range<double> &a) {
					return range<double>{0.0,
						a.overlap(range<double>(0.0,0.0)) ?
							std::numeric_limits<double>::infinity()
							: 0.0};
				});
	}
	return {-std::numeric_limits<double>::infinity(),
					std::numeric_limits<double>::infinity()};
}

bool ispos(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first<=0.0) return false;
	return true;
}
bool isneg(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.second>=0.0) return false;
	return true;
}
bool isnonneg(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first<0.0) return false;
	return true;
}
bool isnonpos(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.second>0.0) return false;
	return true;
}
bool isconst(const expr &e, double k) {
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
		if (r.first!=r.second || std::fmod(r.first,2.0)!=0.0) return false;
	return true;
}
bool isodd(const expr &e) {
	auto rs = rangeprop(e);
	if (rs.x.empty()) return false; //???
	for(auto &r : rs.x)
		if (r.first!=r.second || std::fmod(r.first,2.0)!=1.0) return false;
	return true;
}

/*
struct normswitch : public rewriterule {
	virtual optional<expr> apply(const expr &e) const {
		if (!isop(e,switchop)) return {};
		auto &c0 = e.children()[0];
		expr tosub = newconst(0.0);
		if (c0.isleaf()) {
			if (!isconst(c0) || getconst<double>(c0)==0.0)
				return {};
			tosub = c0;
		} else {
			if (!isop(c0,pluschain) || c0.children().empty()
					|| !isconst(c0.children()[0])
					|| getconst<double>(c0.children()[0])==0.0)
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
	static bool ismergeable(const expr &e, const expr &te, bool checkte=true) {
		if (!isop(e,switchop)) return false;
		auto &ch = e.children();
		if (checkte && !(ch[0]==te)) return false;
		double lastk = 0;
		for(int i=2;i<ch.size();i+=2) {
			if (!isconst(ch[i])) return false;
			if (i>2) {
				double newk = getconst<double>(ch[i]);
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
		double preth = -std::numeric_limits<double>::infinity();
		double postth = std::numeric_limits<double>::infinity();
		for(int i=1;i<ch.size();i+=2)
			if (ismergeable(ch[i],ch[0])) {
				mchi = i;
				if (i>1) preth = getconst<double>(ch[i-1]);
				if (i+1 < ch.size()) postth = getconst<double>(ch[i+1]);
				break;
			}
		if (mchi==-1) return {};

		auto &mch = ch[mchi].children();
		std::vector<expr> newch;
		for(int i=0;i<mchi;i++)
			newch.emplace_back(ch[i]);
		newch.reserve(ch.size()+mch.size()-2);
		for(int i=1;i<mch.size();i+=2) {
			double th = (i+1<mch.size() ? getconst<double>(mch[i+1])
							: std::numeric_limits<double>::infinity());
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

ruleptr SRR(const expr &s, const expr &p) {
	return SR(chainpatternmod(s),p);
}

template<typename F>
ruleptr SRR(const expr &s, const expr &p, F &&f) {
	return SR(chainpatternmod(s),p,std::forward<F>(f));
}

// TODO:  will need to be separated out into general and specific to scalars
std::vector<ruleptr> scalarruleset 
	{{toptr<consteval>(),
	  toptr<scopeeval>(),
	  toptr<sortchildren>(std::vector<op>{pluschain,multiplieschain}),

  SRR(E1_ - E2_                          ,  P1_ + -1*P2_                     ),
  SRR(-E1_                               ,  -1*P1_                           ),

  SRR(E1_ / E2_                          ,  P1_ * pow(P2_,-1.0)              ),

  /*
  SRR(E1_ + (E2_ + E3_)                  ,  P1_ + P2_ + P3_                  ),
  SRR( E1_ + (E2_ + E3_) + E4_           ,  P1_ + P2_ + P3_ + P4_            ),

  SRR(E1_ * (E2_ * E3_)                  ,  P1_ * P2_ * P3_                  ),
  SRR( E1_ * (E2_ * E3_) * E4_           ,  P1_ * P2_ * P3_ * P4_            ),
  */
  toptr<collapsechain>(pluschain,true),
  toptr<collapsechain>(multiplieschain,true),
  /*
  toptr<normswitch>(),
  toptr<mergeswitch>(),
  toptr<squeezeswitch>(),
  toptr<liftswitch>(),
  */

  // some of these are only true "almost everywhere"
  // and might need to be removed for some applications
  // (or, we need a "domain" to be propagated with the expr)
  SRR( 0.0 + E1_                          ,  P1_                             ),
  SRR( 1.0 * E1_                          ,  P1_                             ),
  SRR( 0.0 * E1_                          ,  newconst(0.0)                   ),
  SRR( pow(E1_,1.0)                       ,  P1_                             ),
  SRR( pow(E1_,0.0)                       ,  newconst(1.0)                   ),
  SRR( pow(1.0,E1_)                       ,  newconst(1.0)                   ),

  SRR( E1_ + E1_                          ,  2*P1_                           ),
  SRR( E1_ + E1_ + E2_                    ,  2*P1_ + P2_                     ),

  SRR( E1_ * E1_                          ,  pow(P1_,2.0)                    ),
  SRR( E1_ * E1_ * E2_                    ,  pow(P1_,2.0)*P2_                ),

  SRR( K1_*(W2_ + W3_)                    ,  P1_*P2_ + P1_*P3_               ),

  SRR( K1_*E2_ + K3_*E2_                  ,  (P1_+P3_) * P2_                 ),
  SRR( K1_*E2_ + K3_*E2_ + E4_            ,  (P1_+P3_) * P2_ + P4_           ),
  SRR( E2_ + K3_*E2_                      ,  (newconst(1.0)+P3_) * P2_       ),
  SRR( E2_ + K3_*E2_ + E4_                ,  (newconst(1.0)+P3_) * P2_ + P4_ ),

  SRR( pow(E1_,E2_) * pow(E1_,E3_)        ,  pow(P1_,P2_+P3_)                ),
  SRR( pow(E1_,E2_) * pow(E1_,E3_) * E4_  ,  pow(P1_,P2_+P3_)*P4_            ),
  SRR( E1_ * pow(E1_,E3_)                 ,  pow(P1_,newconst(1.0)+P3_)      ),
  SRR( E1_ * pow(E1_,E3_) * E4_           ,  pow(P1_,newconst(1.0)+P3_)*P4_  ),

  SRR( pow(pow(W1_,E2_),E3_)              ,  pow(P1_,P2_*P3_)                ),
  SRR( pow(pow(K1_,W2_),W3_)              ,  pow(P1_,P2_*P3_)                ),
  SRR( pow(pow(K1_,W2_),K3_)              ,  pow(pow(P1_,P3_),P2_)           ),
  SRR( pow(K1_,K2_*E3_)                   ,  pow(pow(P1_,P2_),P3_)           ),

  SRR( pow(E1_*W2_,E3_)                   ,  pow(P1_,P3_)*pow(P2_,P3_)       ),
  SRR( pow(W1_*E2_,E3_)                   ,  pow(P1_,P3_)*pow(P2_,P3_)       ),
  SRR( pow(K1_,E3_)*pow(K2_,E3_)          ,  pow(P1_*P2_,P3_)                ),

  SRR( log(pow(E1_,E2_))                  ,  P2_*log(P1_)                    ),
  SRR( log(E1_*E2_)                       ,  log(P1_) + log(P2_)             ),

  SRR( log(cond(E1_,E2_,E3_))             ,  cond(P1_,log(P2_),log(P3_))     ),

  SRR( abs(E1_)                           ,  P1_ ,
   [](const exprmap &m) { return isnonneg(m.at(1)); } ),

  SRR( abs(E1_)                           ,  -P1_ ,
   [](const exprmap &m) { return isnonpos(m.at(1)); } ),

  SRR( deriv(E1_,V2_,V3_)                 ,  newconst(0.0),
      [](const exprmap &m) { return isconstexpr(m.at(1),getvar(m.at(2))); }),
  SRR( deriv(V1_,V1_,V3_)                 ,  newconst(1.0)                   ),

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
				 evalat(expr{diracop,E1_},V2_,V3_)*deriv(E1_,V2_,V3_) ),


	 }};


#endif
