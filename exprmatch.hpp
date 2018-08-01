#ifndef EXPRMATCH_HPP
#define EXPRMATCH_HPP

#include "exprbase.hpp"
#include <numeric>
#include <algorithm>
#include <map>
#include "util.hpp"
#include "exprsubst.hpp"

struct matcherbase { }; 

struct matchany : public matcherbase {
	template<typename E>
	optexprmap<E> match(const E &e) const {
		return optexprmap<E>{std::in_place};
	}
};
std::string to_string(const matchany &) { return "<E>"; }

struct matchvar : public matcherbase {
	template<typename E>
	optexprmap<E> match(const E &e) const {
		return isvar(e) ? optexprmap<E>{std::in_place} : optexprmap<E>{};
	}
};
std::string to_string(const matchvar &) { return "<V>"; }

struct matchconst : public matcherbase {
	template<typename E>
	optexprmap<E> match(const E &e) const {
		return isconst(e) ? optexprmap<E>{std::in_place} : optexprmap<E>{};
	}
};
std::string to_string(const matchconst &) { return "<C>"; }

template<typename ...VTs>
struct matchconstwrt : public matcherbase {
	optional<vset<VTs...>> vars;
	matchconstwrt() : vars{} {};
	matchconstwrt(vset<VTs...> vs) : vars{std::in_place,std::move(vs)} {}

	template<typename E>
	optexprmap<E> match(const E &e) const {
		return isconstexpr(e,vars) ? optexprmap<E>{std::in_place} : optexprmap<E>{};
	}
};
template<typename... VTs>
std::string to_string(const matchconstwrt<VTs...> &m) {
	if (!m.vars) return "<K>";
	std::string ret = "<K";
	for(auto &v : *m.vars) {
		ret += ',';
		ret += v->name;
	}
	ret += '>';
	return ret;
}

template<typename ...VTs>
struct matchnonconstwrt : public matcherbase<E> {
	optional<vset<VTs...>> vars;
	matchnonconstwrt() : vars{} {};
	matchnonconstwrt(vset vs) : vars{std::in_place,std::move(vs)} {}

	template<typename E>
	optexprmap<E> match(const E &e) const {
		return isnonconstexpr(e,vars) ? optexprmap<E>{std::in_place} : optexprmap<E>{};
	}

};
template<typename... VTs>
std::string to_string(const matchnonconstwrt<VTs...> &m) {
	if (!m.vars) return "<W>";
	std::string ret = "<W";
	for(auto &v : *m.vars) {
		ret += ',';
		ret += v->name;
	}
	ret += '>';
	return ret;
}


template<typename E1, typename E2>
bool opsmatch(const E1 &e1, const E2 &e2) {
	if (e1.isleaf() || e2.isleaf()) return false;
	auto n1 = e1.asnode();
	auto n2 = e2.asnode();

	template<typename T>
	using chaintrue = binarychainop<T,true>;
	template<typename T>
	using chainfalse = binarychainop<T,false>;

	return sametypes(n1,n2) || 
		sametypeswrap<chaintrue>(n1,n2) ||
		sametypeswrap<chaintrue>(n2,n1) ||
		sametypeswrap<chainfalse>(n1,n2) ||
		sametypeswrap<chainfalse>(n2,n1);
}

template<typename E>
bool equiv(const E1 &a, const E2 &b, vmap_t<E2,E1> &m) {
	if (!(a.isleaf()==b.isleaf())) return false;
	if (a.isleaf()) {
		if (isvar(a) && isvar(b)) {
			auto l = m.find(getvar(b));
			if (l!=m.end()) return vareq(getvar(a),l->second);
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

template<typename E1, typename E2>
bool equiv(const E1 &a, const E2 &b) {
	vmap_t<E2,E1> m;
	return equiv(a,b,m);
}

template<typename E>
bool mergemap(exprmap<E> &orig, const exprmap<E> &toadd) {
	for(auto &p : toadd) {
		auto [orig,inserted] = orig.emplace(p);
		if (!inserted && !equiv(orig->second,p.second)) return false;
	}
	return true;
}

template<typename E1, typename E2>
optexprmap<E1> match(const E1 &e, const E2 &pat) {
	if (pat.isleaf()) {
		auto &patleaf = pat.asleaf();
		if (isderivtype<matcherbase>(patleaf.asvariant()))
			return std::visit([&e](auto &&l) -> optexprmap<E1> {
					return l.match(e); }, parleaf.asvariant());
		if (e.isleaf() && e.asleaf()==pat.asleaf()) 
			return optexprmap<E1>{std::in_place};
		else return {};
	} else {
		auto &patnode = pat.asnode();
		if (isderivtype<matcherbase>(patnode.asvariant()))
			return std::visit([&e](auto &&n) -> optexprmap<E1> {
					return n.match(e,pat.children()); },
				parleaf.asvariant());
		if (e.isleaf()) return {};
		if (!(patnode==e.asnode())
			|| e.children().size() != pat.children().size())
				return {};
		exprmap<E1> retmap;
		for(int i=0;i<pat.children().size();i++) {
			auto r = match(e.children()[i],pat.children()[i]);
			if (!r || !mergemap(retmap,*r)) return {};
		}
		return {std::in_place,retmap};
	}
}

struct matchlabelop : public matcherbase {
	int lnum;

	template<typename E, typename PE>
	optexprmap<E> match(const E &e, const std::vector<PE> &matchch) const {
		auto ret = ::match(e,matchch[0]);
		if (ret) ret->emplace(lnum,e);
		return ret;
	}

	bool operator==(const matchlabelop &l) const { return l.lnum=lnum; }
};
std::string to_string(const matchlabelop &o) {
	return std::string("<L:") + to_string(o.lnum) + ">";
}

template<typename E>
auto L(int n, const E &e) {
	return buildexpr(matchlabelop{n},e);
}

template<typename OP, typename E, typename PE>
optexprmap<E> matchwithrem(const OP &o, const std::vector<E> &ech,
				const std::vector<PE> &pch) {
	if (ech.size()<matchch.size()) return {};
	exprmap<E1> retmap;
	if (ech.size()==pch.size()) { // no "remainder" to be done, really
		for(int i=0;i<pch.size();i++) {
			auto r = match(ech[i],pch[i]);
			if (!r || !mergemap(retmap,*r)) return {};
		}
		return {std::in_place,retmap};
	} else {
		for(int i=0;i<pch.size()-1;i++) {
			auto r = match(ech[i],pch[i]);
			if (!r || !mergemap(retmap,*r)) return {};
		}
		std::vector<E> lastch;
		for(int i=pch.size()-1;i<ech.size();i++)
			lastch.emplace_back(ech[i]);
		auto r = ::match(buildexprvec(o,lastch),pch.back());
		if (!r || !mergemap(retmap,*r)) return {};
		return optexprmap<E>{std::in_place,retmap};
	}
}

template<typename OP>
struct matchremainderop : public matcherbase {
	OP op;

	template<typename... Ts>
	matchremainderop(Ts &&...args) : op(std::forward<Ts>(args)...) {}

	template<typename E, typename PE>
	optexprmap<E> match(const E &e, const std::vector<PE> &matchch) const {
		if (e.isleaf() || !(op==e.asnode())) return {};
		return matchassoc(op,e.children(),matchch);
	}
};

template<typename OP, bool withremainder=false>
struct matchassocop : public matcherbase {
	OP op;

	template<typename... Ts>
	matchassocop(Ts &&...args) : op(std::forward<Ts>(args)...) {}

	template<typename E, typename PE>
	optexprmap<E> match(const E &e, const std::vector<PE> &matchch) const {
		if (e.isleaf() || !(op==e.asnode())) return {};
		auto &ech = e.children();
		if (ech.size()<matchch.size()) return {};
		if (!withremainder && ech.size()!=matchch.size()) return {};
		if (ech.size()<2) return matchwithrem(op,ech,matchch);
		pickn<E1> p(ech,matchch.size()-1);
		auto mexpr = buildexprvec(o,matchch);
		do {
			auto ret = ::matchwithrem(buildexprvec(op,p.x),mexpr);
			if (ret) return ret;
		} while(p.nextpick());
		return {};
	}
};


expr E0_ = L(0,matchany{});
expr E1_ = L(1,matchany{});
expr E2_ = L(2,matchany{});
expr E3_ = L(3,matchany{});
expr E4_ = L(4,matchany{});
expr E5_ = L(5,matchany{});
expr E6_ = L(6,matchany{});
expr E7_ = L(7,matchany{});
expr E8_ = L(8,matchany{});
expr E9_ = L(9,matchany{});

expr C0_ = L(0,matchconst{});
expr C1_ = L(1,matchconst{});
expr C2_ = L(2,matchconst{});
expr C3_ = L(3,matchconst{});
expr C4_ = L(4,matchconst{});
expr C5_ = L(5,matchconst{});
expr C6_ = L(6,matchconst{});
expr C7_ = L(7,matchconst{});
expr C8_ = L(8,matchconst{});
expr C9_ = L(9,matchconst{});

expr V0_ = L(0,matchvar{});
expr V1_ = L(1,matchvar{});
expr V2_ = L(2,matchvar{});
expr V3_ = L(3,matchvar{});
expr V4_ = L(4,matchvar{});
expr V5_ = L(5,matchvar{});
expr V6_ = L(6,matchvar{});
expr V7_ = L(7,matchvar{});
expr V8_ = L(8,matchvar{});
expr V9_ = L(9,matchvar{});

#endif
