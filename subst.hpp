#ifndef SUBST_HPP
#define SUBST_HPP

#include "base.hpp"
#include <map>
#include <iterator>
#include <optional>

// Assumes that the type of the expression remains unchanged
// replaces x in e with v
template<typename E1, typename E2, typename E3>
auto substitute(const E1 &e, const E2 &x, const E3 &v) {
	if constexpr (std::is_same_v<E1,E3>)
		return e.map([x,v](const E1 &ex) {
				if (exprsame(ex,x)) return std::optional<E1>{v};
					return std::optional<E1>{};
				});
	else {
		using retT = exprunion_t<E1,E3>;
		using ltype = exprleaf_t<retT>;
		retT newv = upgradeexpr<retT>(v);
		if constexpr (std::is_same_v<retT,E1>) 
			return e.map([x,&newv](const E1 &ex) {
				if (exprsame(ex,x)) return std::optional<retT>{newv};
					return std::optional<retT>{};
				});
		else return e.fold([x,&newv](const auto &l) -> retT {
				return (x.isleaf() && x.asleaf()==l)
						? newv : 
					     retT{upgradevariant<ltype>(l.asvariant())};
				},
				[x,&newv](const auto &n, auto &&ch) -> retT {
					retT tempe{n,std::move(ch)};
					return exprsame(tempe,x) ? newv : tempe;
				});
	}
}
template<typename E1, typename E2, typename T>
auto substituteconst(const E1 &e, const E2 &x, T &&v) {
	using retT = exprunion_t<E1,expr1type_t<T>>;
	retT newv = newconst<T,retT>(std::forward<T>(v));
	return substitute(e,x,newv);
}

// x <- v (x is replaced by v)
template<typename X, typename V>
struct subst {
	subst(X xx, V vv) : x(std::move(xx)), v(std::move(vv)) {}
	X x;
	V v;
};

template<typename E1, typename E2, typename E3>
auto substitute(const E1 &e, const subst<E2,E3> &st) {
	return substitute(e,st.x,st.v);
}

// note that this does not try subsequent substitutions on the result
// of the first... it applies them all "in parallel"
template<typename E1, typename E2, typename E3>
auto substitute(const E1 &e, const std::vector<subst<E2,E3>> &st) {
	if constexpr (std::is_same_v<E1,E3>)
		return e.map([&st](const E1 &ex) {
				for(auto &s : st)
					if (exprsame(ex,s.x)) return std::optional<E1>{s.v};
				return std::optional<E1>{};
				});
	else {
		using retT = exprunion_t<E1,E3>;
		std::vector<subst<E2,retT>> newst;
		for(auto &s : st)
			newst.emplace_back(s.x,upgradeexpr<retT>(s.v));
		if constexpr (std::is_same_v<retT,E1>) 
			return e.map([&newst](const E1 &ex) {
				for(auto &s : newst)
					if (exprsame(ex,s.x)) return std::optional<E1>{s.v};
				return std::optional<E1>{};
				});
		else return e.fold([&newst](const auto &l) -> retT {
						for(auto &s : newst)
							if (exprsame(l,s.x)) return s.v;
						return upgradeexpr<retT>(l);
					},
					[&newst](const auto &n, auto &&ch) {
						retT tempe{n,std::move(ch)};
						for(auto &s : newst)
							if (exprsame(tempe,s.x)) return s.v;
						return tempe;
					});
	}
};

//------------------

struct placeholder {
	int num;
	bool operator==(const placeholder &p) const { return p.num==num; }
};

std::string to_string(const placeholder &p) {
	return std::string("P")+std::to_string(p.num);
}

auto exprplaceholder(int i) { return newconst(placeholder{i}); }


inline auto P0_ = exprplaceholder(0);
inline auto P1_ = exprplaceholder(1);
inline auto P2_ = exprplaceholder(2);
inline auto P3_ = exprplaceholder(3);
inline auto P4_ = exprplaceholder(4);
inline auto P5_ = exprplaceholder(5);
inline auto P6_ = exprplaceholder(6);
inline auto P7_ = exprplaceholder(7);
inline auto P8_ = exprplaceholder(8);
inline auto P9_ = exprplaceholder(9);

template<typename E> // map from placeholder to appropriate replacement expr
using exprmap = std::map<int,E>;
template<typename E> // a map as above, or nothing
using optexprmap = std::optional<exprmap<E>>;

template<typename E,
	std::enable_if_t<isexpr_v<E>,int> = 0>
bool isplaceholder(const E &e) {
	return (isconst(e) && std::holds_alternative<constval<placeholder>>(e.asleaf()));
}

template<typename... Ts>
bool isplaceholder(const exprleaf<Ts...> &l) {
	return std::holds_alternative<constval<placeholder>>(l.asvariant());
}


template<typename E1, typename E2>
auto substitute(const E1 &e, const exprmap<E2> &st) {
	using retT = exprunion_t<E1,E2>;
	if (st.empty()) return upgradeexpr<retT>(e);
	if constexpr (std::is_same_v<E1,retT>)
		return e.map([&st](const E1 &ex) {
				if (isplaceholder(ex)) {
					int n = std::get<constval<placeholder>>(ex.asleaf()).v.num;
					auto l = st.find(n);
					if (l!=st.end()) return std::optional<retT>
								{upgradeexpr<retT>(l->second)};
				}
				return std::optional<retT>{};
			});
	else {
		using ltype = exprleaf_t<retT>;
		using ntype = exprnode_t<retT>;
		return e.fold([&st](const auto &l) -> retT {
					if (isplaceholder(l)) {
						int n = std::get<constval<placeholder>>(l).v.num;
						auto loc = st.find(n);
						if (loc!=st.end())
							return retT{upgradeexpr<retT>(loc->second)};
					}
					return retT{upgradevariant<ltype>(l.asvariant())};
				},
				[](auto &&n, auto &&ch) {
					return retT{upgradevariant<ntype>
                                   (std::forward<decltype(n)>(n).asvariant()),
                              std::forward<decltype(ch)>(ch)};

				});
	}
}

//--------------------

template<typename E>
E replacelocal(const E &e) {
	return e.map([](const E &ex) {
			if (!isscope(ex) || isplaceholder(ex.children()[0]))
				return std::optional<E>{};
			else return std::optional<E>{substitute(ex,
						ex.children()[0],
						newvarsametype(ex.children()[0])
					)};
		});
}

#endif
