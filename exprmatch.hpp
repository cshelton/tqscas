#ifndef EXPRMATCH_HPP
#define EXPRMATCH_HPP

#include "exprbase.hpp"
#include "exprcoreops.hpp"
#include <numeric>
#include <algorithm>
#include <map>
#include "util.hpp"
#include "exprsubst.hpp"
#include <string>

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
	std::optional<vset<VTs...>> vars;

	template<typename... Ts>
	matchconstwrt(Ts &&...x) : vars{std::in_place,std::forward<Ts>(x)...} {}
	matchconstwrt(const matchconstwrt &) = default;
	matchconstwrt(matchconstwrt &&) = default;

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
struct matchnonconstwrt : public matcherbase {
	std::optional<vset<VTs...>> vars;

	template<typename... Ts>
	matchnonconstwrt(Ts &&...x) : vars{std::in_place,std::forward<Ts>(x)...} {}
	matchnonconstwrt(const matchnonconstwrt &) = default;
	matchnonconstwrt(matchnonconstwrt &&) = default;

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

template<typename T>
using chaintrue = binarychainop<T,true>;
template<typename T>
using chainfalse = binarychainop<T,false>;

template<typename E1, typename E2>
bool opsmatch(const E1 &e1, const E2 &e2) {
	if (e1.isleaf() || e2.isleaf()) return false;
	auto n1 = e1.asnode();
	auto n2 = e2.asnode();


	return sametypes(n1,n2) || 
		sametypeswrap<chaintrue>(n1,n2) ||
		sametypeswrap<chaintrue>(n2,n1) ||
		sametypeswrap<chainfalse>(n1,n2) ||
		sametypeswrap<chainfalse>(n2,n1);
}

template<typename E1, typename E2>
bool equiv(const E1 &a, const E2 &b, vmap_t<E2,E1> &m) {
	if (!(a.isleaf()==b.isleaf())) return false;
	if (a.isleaf()) {
		if (isvar(a) && isvar(b)) {
			auto l = m.find(getvar(b));
			if (l!=m.end()) return varianteq(getvar(a),l->second);
		}
		return a.asleaf()==b.asleaf();
	}
	if (!(a.asnode()==b.asnode())) return false;
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
		auto [oloc,inserted] = orig.emplace(p);
		if (!inserted && !equiv(oloc->second,p.second)) return false;
	}
	return true;
}

template<typename E1, typename E2>
optexprmap<E1> match(const E1 &e, const E2 &pat) {
	if (pat.isleaf()) {
		return std::visit([&e](auto &&l) -> optexprmap<E1> {
				using Ltype = std::decay_t<decltype(l)>;
				if constexpr (istmpl_v<constval,Ltype>) {
					using Vtype = std::decay_t<decltype(l.v)>;
					if constexpr (std::is_base_of_v<matcherbase,Vtype>)
						return l.v.match(e);
				}
				if (e.isleaf() && e.asleaf()==l)
					return optexprmap<E1>{std::in_place};
				else return {};	
			}, pat.asleaf().asvariant());
	} else {
		return std::visit([&e,&pat](auto &&n) -> optexprmap<E1> {
				if constexpr (std::is_base_of_v<matcherbase,
						std::decay_t<decltype(n)>>)
					return n.match(e,pat.children());
				else if (e.isleaf()) return {};
				else if (!(n==e.asnode())
						|| e.children().size()!=pat.children().size())
					return {};
				else {
					exprmap<E1> retmap;
					for(int i=0;i<pat.children().size();i++) {
						auto r = match(e.children()[i],pat.children()[i]);
						if (!r || !mergemap(retmap,*r)) return {};
					}
					return optexprmap<E1>{std::in_place,retmap};
				}
				}, pat.asnode().asvariant());
	}
}

struct matchlabelop : public matcherbase {
	int lnum;

	matchlabelop(int l) : lnum(l), matcherbase() {}

	template<typename E, typename PE>
	optexprmap<E> match(const E &e, const std::vector<PE> &matchch) const {
		auto ret = ::match(e,matchch[0]);
		if (ret) ret->emplace(lnum,e);
		return ret;
	}

	bool operator==(const matchlabelop &l) const { return l.lnum==lnum; }
};
std::string symbol(const matchlabelop &o) {
	return std::string("<L") + std::to_string(o.lnum) + ">";
}
std::string write(const matchlabelop &o,
			const std::vector<std::pair<std::string,int>> &subst) {
	return putinparen(subst[0].first,subst[0].second>precedence(o)) + "\\" + std::to_string(o.lnum);
}
int precedence(matchlabelop) { return 2; }


template<typename E>
auto L(int n, const E &e) {
	return buildexpr(matchlabelop{n},e);
}

template<typename OP, typename E, typename PE>
optexprmap<E> matchwithrem(const OP &o, const std::vector<E> &ech,
				const std::vector<PE> &pch) {
	if (ech.size()<pch.size()) return {};
	exprmap<E> retmap;
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
template<typename OP>
constexpr int precedence(const matchremainderop<OP> &o) {
	return precedence(o.op);
}
template<typename OP>
std::string symbol(const matchremainderop<OP> &o) {
	return symbol(o.op)+"(r)";
}
template<typename OP>
std::string write(const matchremainderop<OP> &o,
			std::vector<std::pair<std::string,int>> subst) {
	subst.emplace_back("rem",precedence(o.op));
	return write(o.op,std::move(subst));
}

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
		pickn<E> p(ech,matchch.size()-1);
		auto mexpr = buildexprvec(op,matchch);
		do {
			auto ret = ::matchwithrem(buildexprvec(op,p.x),mexpr);
			if (ret) return ret;
		} while(p.nextpick());
		return {};
	}
};
template<typename OP, bool WR>
constexpr int precedence(const matchassocop<OP,WR> &o) {
	return precedence(o.op);
}
template<typename OP, bool WR>
std::string symbol(const matchassocop<OP,WR> &o) {
	if constexpr (WR) return symbol(o.op)+"(ar)";
	else return symbol(o.op)+"(a)";
}
template<typename OP>
std::string write(const matchassocop<OP,true> &o,
			std::vector<std::pair<std::string,int>> subst) {
	subst.emplace_back("rem",precedence(o.op));
	return std::string("{")+write(o.op,std::move(subst))+"}";
}
template<typename OP>
std::string write(const matchassocop<OP,false> &o,
			const std::vector<std::pair<std::string,int>> &subst) {
	return std::string("{")+write(o.op,std::move(subst))+"}";
}

auto E0_ = L(0,newconst(matchany{}));
auto E1_ = L(1,newconst(matchany{}));
auto E2_ = L(2,newconst(matchany{}));
auto E3_ = L(3,newconst(matchany{}));
auto E4_ = L(4,newconst(matchany{}));
auto E5_ = L(5,newconst(matchany{}));
auto E6_ = L(6,newconst(matchany{}));
auto E7_ = L(7,newconst(matchany{}));
auto E8_ = L(8,newconst(matchany{}));
auto E9_ = L(9,newconst(matchany{}));

auto C0_ = L(0,newconst(matchconst{}));
auto C1_ = L(1,newconst(matchconst{}));
auto C2_ = L(2,newconst(matchconst{}));
auto C3_ = L(3,newconst(matchconst{}));
auto C4_ = L(4,newconst(matchconst{}));
auto C5_ = L(5,newconst(matchconst{}));
auto C6_ = L(6,newconst(matchconst{}));
auto C7_ = L(7,newconst(matchconst{}));
auto C8_ = L(8,newconst(matchconst{}));
auto C9_ = L(9,newconst(matchconst{}));

auto V0_ = L(0,newconst(matchvar{}));
auto V1_ = L(1,newconst(matchvar{}));
auto V2_ = L(2,newconst(matchvar{}));
auto V3_ = L(3,newconst(matchvar{}));
auto V4_ = L(4,newconst(matchvar{}));
auto V5_ = L(5,newconst(matchvar{}));
auto V6_ = L(6,newconst(matchvar{}));
auto V7_ = L(7,newconst(matchvar{}));
auto V8_ = L(8,newconst(matchvar{}));
auto V9_ = L(9,newconst(matchvar{}));

#endif
