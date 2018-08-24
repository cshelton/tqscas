#ifndef EXPRSUBSTSUGAR_H
#define EXPRSUBSTSUGAR_H

#include "exprsubst.hpp"
#include "exprsugarmacros.hpp"

template<typename E1, typename E2, typename E3,
     std::enable_if_t<isexpr_v<E1>,int> = 0>
auto operator|(E1 e, std::vector<subst<E2,E3>> st) {
     return substitute(std::move(e),std::move(st));
}

template<typename E1, typename E2,
     std::enable_if_t<isexpr_v<E1> && isexpr_v<E2>,int> = 0>
std::vector<subst<E1,E2>> operator<<(E1 xx, E2 vv) {
     return {1,subst{std::move(xx),std::move(vv)}};
}

template<typename E1, typename E2,
     std::enable_if_t<isexpr_v<E1> && !isexpr_v<std::decay_t<E2>>,int> = 0>
std::vector<subst<E1,
	decltype(newconst(std::declval<std::decay_t<E2>>()))>>
operator<<(E1 xx, E2 &&vv) {
     return {1,subst{std::move(xx),
			newconst(std::forward<E2>(vv))}};
}

template<typename E1, typename E2, typename E3, typename E4>
auto operator&(std::vector<subst<E1,E2>> v1, std::vector<subst<E3,E4>> v2) {
	if constexpr (std::is_same_v<E1,E3> && std::is_same_v<E2,E4>) {
		v1.insert(v1.end(),std::make_move_iterator(v2.begin()),
				std::make_move_iterator(v2.end()));
		return v1;
	} else {
		using E13 = exprunion_t<E1,E3>;
		using E24 = exprunion_t<E2,E4>;
		std::vector<subst<E13,E24>> ret;
		for(auto &s: v1)
			ret.emplace_back(upgradeexpr<E13>(s.x),upgradeexpr<E24>(s.v));
		for(auto &s: v2)
			ret.emplace_back(upgradeexpr<E13>(s.x),upgradeexpr<E24>(s.v));
		return ret;
	}
}

#endif
