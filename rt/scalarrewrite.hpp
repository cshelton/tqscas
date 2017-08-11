#ifndef SCALARREWRITE_HPP
#define SCALARREWRITE_HPP

#include "scalarbase.hpp"
#include "exprrewrite.hpp"
#include "exprmatch.hpp"
#include <algorithm>

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
	optional<std::vector<expr>> vars;
	std::vector<op> cops;

	sortchildren(std::vector<op> comops)
		: cops(comops), vars{} { }
	sortchildren(std::vector<op> comops, std::vector<expr> v)
		: cops(comops), vars(in_place,std::move(v)) {}

	int typeorder(const expr &e) const {
		if (isconst(e)) return 0;
		int add = (isconstexpr(e,vars) ? 2 : 9);
		if (e.isleaf()) return -1+add;
		auto &op = e.asnode();
		if (op==plusop || op==pluschain) return 0+add;
		if (op==multipliesop || op==multiplieschain) return 1+add;
		if (op==powerop) return 2+add;
		if (op==logop) return 3+add;
		//if (op==condop || op==switchop) return 4+add;
		if (op==switchop) return 4+add;
		return 5+add;
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
				auto node = e.asnode();
				return optional<expr>{in_place,node,che};
			}
		return {};
	}
};

ruleptr SRR(const expr &s, const expr &p) {
	return SR(chainpatternmod(s),p);
}

std::vector<ruleptr> scalarruleset 
	{{std::dynamic_pointer_cast<rewriterule>(std::make_shared<consteval>()),
	  std::dynamic_pointer_cast<rewriterule>(std::make_shared<sortchildren>(
				  std::vector<op>{pluschain,multiplieschain})),

  SRR(E1_ - E2_                          ,  P1_ + -1*P2_                     ),
  SRR(-E1_                               ,  -1*P1_                           ),

  SRR(E1_ / E2_                          ,  P1_ * pow(P2_,-1.0)              ),

  /*
  SRR(E1_ + (E2_ + E3_)                  ,  P1_ + P2_ + P3_                  ),
  SRR( E1_ + (E2_ + E3_) + E4_           ,  P1_ + P2_ + P3_ + P4_            ),

  SRR(E1_ * (E2_ * E3_)                  ,  P1_ * P2_ * P3_                  ),
  SRR( E1_ * (E2_ * E3_) * E4_           ,  P1_ * P2_ * P3_ * P4_            ),
  */
  std::dynamic_pointer_cast<rewriterule>(std::make_shared<collapsechain>(pluschain,true)),
  std::dynamic_pointer_cast<rewriterule>(std::make_shared<collapsechain>(multiplieschain,true)),

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

  SRR( K1_*(W2_ + W3_)                    ,  P1_*P2_ + P1_*P3_               ),

	 }};


#endif
