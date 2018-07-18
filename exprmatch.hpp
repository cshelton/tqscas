#ifndef EXPRMATCH_HPP
#define EXPRMATCH_HPP

#include "exprbase.hpp"
#include <numeric>
#include <algorithm>
#include <map>
#include "util.hpp"

template<typename E>
struct matcherbase {
	virtual optexprmap<E> match(const E &e,
			const std::vector<E> &matchch) const {
		return {};
	}
	virtual std::string name() const {
		return "";
	}
};

template<typename E>
using matcher = std::shared_ptr<matcherbase<E>>;

template<typename E>
using exprmerge = exprunion_t<E,
	 expr<std::tuple<matcher<E>>,std::tuple<matcher<E>>>>;

template<typename E>
struct matchany : public matcherbase<E> {
	virtual optexprmap<E> match(const E &e,
			const std::vector<E> ) const {
		return optexprmap<E>{std::in_place};
	}
	virtual std::string name() const {
		return "[E]";
	}
};

template<typename E>
struct matchvar : public matchbase<E> {
	virtual optexprmap<E> match(const E &e,
			const std::vector<E> ) const {
		if (isvar(e)) return optexprmap<E>{std::in_place};
		else return {};
	}
	virtual std::string name() const {
		return "[V]";
	}
};

template<typename E>
struct matchconst : public matcherbase<E> {
	virtual optexprmap<E> match(const E &e,
			const std::vector<E> ) const {
		if (isconst(e)) return optexprmap<E>{std::in_place};
		else return {};
	}
	virtual std::string name() const {
		return "[C]";
	}
};

template<typename E>
struct matchconstwrt : public matcherbase<E> {
	optional<vset> vars;
	matchconstwrt() : vars{} {};
	matchconstwrt(vset vs) : vars{std::in_place,std::move(vs)} {}

	virtual optexprmap<E> match(const E &e,
			const std::vector<E> ) const {
		return isconstexpr(e,vars) ? optexprmap<E>{std::in_place} : optexprmap<E>{};
	}
	virtual std::string name() const {
		return "[K]";
	}
};

template<typename E>
struct matchnonconstwrt : public matcherbase<E> {
	optional<vset> vars;
	matchnonconstwrt() : vars{} {};
	matchnonconstwrt(vset vs) : vars{std::in_place,std::move(vs)} {}

	virtual optexprmap<E> match(const E &e,
			const std::vector<E> ) const {
		return isnonconstexpr(e,vars) ? optexprmap<E>{std::in_place} : optexprmap<E>{};
	}

	virtual std::string name() const {
		return "[W]";
	}
};


template<typename T, typename... As>
matchleaf makematchleaf(As &&...args) {
	return std::dynamic_pointer_cast<matcherbase<tmplparam_t<E>>>
					(std::make_shared<T>(std::forward<As>(args)...));
}


template<typename E>
bool opsmatch(const E &e1, const E &e2) {
	if (e1.isleaf() || e2.isleaf()) return false;
	auto n1 = e1.asnode();
	auto n2 = e2.asnode();

	template<typename T>
	using chaintrue = opbinarychain<T,true>;
	template<typename T>
	using chainfalse = opbinarychain<T,false>;

	return sametype(n1,n2) || 
		sametypewrap<chaintrue>(n1,n2) ||
		sametypewrap<chaintrue>(n2,n1) ||
		sametypewrap<chainfalse>(n1,n2) ||
		sametypewrap<chainfalse>(n2,n1);
}

template<typename E>
bool equiv(const E &a, const E &b, vmap &m) {
	if (!(a.isleaf()==b.isleaf())) return false;
	if (a.isleaf()) {
		if (isvar(a) && isvar(b)) {
			auto l = m.find(getvar(b));
			if (l!=m.end()) return getvar(a)==l->second;
		}
		return a.asleaf()==b.asleaf();
	}
	if (a.asnode()!=b.asnode()) return false;
	auto &ach = a.children();
	auto &bch = b.children();
	if (ach.size()!=bch.size()) return false;
	if (isderivtype<scopeop>(a.asnode()))
		m.emplace(getvar(bch[0]), getvar(ach[0]));
	for(int i=0;i<ach.size();i++)
		if (!equiv(ach[i],bch[i],m)) return false;
	if (isderivtype<scopeop>(a.asnode()))
		m.erase(getvar(bch[0]));
	return true;
}

template<typename E>
bool equiv(const E &a, const E &b) {
	vmap m;
	return equiv(a,b,m);
}

template<typename E>
bool mergemap(exprmap<E> &orig, const exprmap<E> &toadd) {
	for(auto &p : toadd) {
		auto loc = orig.emplace(p);
		if (!loc.second && !equiv(loc.first->second,p.second)) return false;
	}
	return true;
}

template<typename E>
optexprmap<E> match(const E &e, const E &pat) {
	if (pat.isleaf()) {
		auto patleaf = pat.asleaf();
		if (istype<matchleaf>
		if (patleaf.type()==typeid(matchleaf))
			return MYany_cast<matchleaf>(patleaf)->match(e);
		if (e.sameas(pat)) return optexprmap<E>{std::in_place};
		return {};
	} else {
		std::shared_ptr<matchop> mop
			= std::dynamic_pointer_cast<matchop>(pat.asnode());
		if (mop) return mop->match(e,pat.children());
		if (!opsmatch(e,pat)
			|| e.children().size() != pat.children().size())
				return {};
		exprmap<E> retmap;
		for(int i=0;i<pat.children().size();i++) {
			auto r = match(e.children()[i],pat.children()[i]);
			if (!r || !mergemap(retmap,*r)) return {};
		}
		return optexprmap<E>{std::in_place,retmap};
	}
}

struct matchlabelop : public matchop {
	int lnum;

	matchlabelop(int num) : matchop(1,std::string("#")+std::to_string(num),
				false,false,10), lnum(num) {}

	virtual optexprmap<E> match(const expr &e,
			const std::vector<expr> &matchch) const {
		auto ret = ::match(e,matchch[0]);
		if (ret) ret->emplace(lnum,e);
		return ret;
	}

	virtual any opeval(const any &x1) const { return x1; };

};

expr L(int n, const expr &e) {
	return {std::make_shared<matchlabelop>(n),e};
}

struct matchmodop : public matchop {
	op o;
	matchmodop(op orig, const std::string &namemod)
			: matchop(orig->narg,namemod+orig->name,
			namemod+orig->treename,orig->infix,orig->leftassoc,orig->prec),
			o(orig) {}

	virtual any opeval() const { return o->opeval(); }
	virtual any opeval(const any &x1) const { return o->opeval(x1); }
	virtual any opeval(const any &x1, const any &x2) const
			{ return o->opeval(x1,x2); }
	virtual any opeval(const any &x1, const any &x2, const any &x3) const
			{ return o->opeval(x1,x2,x3); }
	virtual any opeval(const std::vector<any> &x) const
			{ return o->opeval(x); }

};

struct matchremainderop : public matchmodop {
	matchremainderop(op orig) : matchmodop(orig,"R") {}

	virtual optexprmap<E> match(const expr &e,
			const std::vector<expr> &matchch) const {
		expr me{o,matchch};
		if (!opsmatch(e,me)) return {};
		auto ech = e.children();
		if (ech.size()<matchch.size()) return {};
		if (ech.size()==matchch.size()) return ::match(e,me);
		exprmap<E> retmap;
		for(int i=0;i<matchch.size()-1;i++) {
			auto r = ::match(ech[i],matchch[i]);
			if (!r || !mergemap(retmap,*r)) return {};
		}
		std::vector<expr> lastch;
		for(int i=matchch.size()-1;i<ech.size();i++)
			lastch.emplace_back(ech[i]);
		auto r = ::match(expr{e.asnode(),lastch},matchch.back());
		if (!r || !mergemap(retmap,*r)) return {};
		return optexprmap<E>{std::in_place,retmap};
	}
};

struct matchassocop : public matchmodop {
	matchassocop(op orig) : matchmodop(orig,"A") {}

	virtual optexprmap<E> match(const expr &e,
			const std::vector<expr> &matchch) const {
		if (e.isleaf() || e.children().size()<2)
			return ::match(e,expr{o,matchch});
		auto ech = e.children();
		auto enode = e.asnode();
		bool isrem = std::dynamic_pointer_cast<matchremainderop>(o) != nullptr;
		if (!isrem && ech.size()!=matchch.size()) return {};
		if (isrem || ech.size()<=matchch.size()) {
			/* below works perfectly well in all cases 
			 * (and doesn't need to know about matchremainderop),
			 * except if ech.size() gets too large
			 */
			/*
			expr mexpr{o,matchch};
			std::vector<int> ii(ech.size());
			std::iota(ii.begin(),ii.end(),0);
			do {
				std::vector<expr> newch;
				newch.reserve(ii.size());
				for(int i=0;i<ii.size();i++)
					newch.emplace_back(ech[ii[i]]);
				auto ret = ::match(expr{enode,newch},mexpr);
				if (ret) return ret;
			} while(std::next_permutation(ii.begin(),ii.end()));
			return {};
			*/
			pickn<expr> p(e.children(),matchch.size()-1);
			expr mexpr{o,matchch};
			do {
				auto ret = ::match(expr{enode,p.x},mexpr);
				if (ret) return ret;
			} while(p.nextpick());
			return {};
		} else {
			pickn<expr> p(matchch,matchch.size()-1);
			do {
				auto ret = ::match(e,expr{o,p.x});
				if (ret) return ret;
			} while(p.nextpick());
			return {};
		}
	}
};


expr E_{makematchleaf<matchany>()};
expr V_{makematchleaf<matchvar>()};
expr C_{makematchleaf<matchconst>()};
expr K_{makematchleaf<matchconstwrt>()};
expr W_{makematchleaf<matchnonconstwrt>()};

expr E0_ = L(0,E_);
expr E1_ = L(1,E_);
expr E2_ = L(2,E_);
expr E3_ = L(3,E_);
expr E4_ = L(4,E_);
expr E5_ = L(5,E_);
expr E6_ = L(6,E_);
expr E7_ = L(7,E_);
expr E8_ = L(8,E_);
expr E9_ = L(9,E_);

expr C0_ = L(0,C_);
expr C1_ = L(1,C_);
expr C2_ = L(2,C_);
expr C3_ = L(3,C_);
expr C4_ = L(4,C_);
expr C5_ = L(5,C_);
expr C6_ = L(6,C_);
expr C7_ = L(7,C_);
expr C8_ = L(8,C_);
expr C9_ = L(9,C_);

expr K0_ = L(0,K_);
expr K1_ = L(1,K_);
expr K2_ = L(2,K_);
expr K3_ = L(3,K_);
expr K4_ = L(4,K_);
expr K5_ = L(5,K_);
expr K6_ = L(6,K_);
expr K7_ = L(7,K_);
expr K8_ = L(8,K_);
expr K9_ = L(9,K_);

expr V0_ = L(0,V_);
expr V1_ = L(1,V_);
expr V2_ = L(2,V_);
expr V3_ = L(3,V_);
expr V4_ = L(4,V_);
expr V5_ = L(5,V_);
expr V6_ = L(6,V_);
expr V7_ = L(7,V_);
expr V8_ = L(8,V_);
expr V9_ = L(9,V_);

expr W0_ = L(0,W_);
expr W1_ = L(1,W_);
expr W2_ = L(2,W_);
expr W3_ = L(3,W_);
expr W4_ = L(4,W_);
expr W5_ = L(5,W_);
expr W6_ = L(6,W_);
expr W7_ = L(7,W_);
expr W8_ = L(8,W_);
expr W9_ = L(9,W_);

#endif
