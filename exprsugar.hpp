#ifndef EXPRSUGAR_H
#define EXPRSUGAR_H

#include "exprbasicops.hpp"
#include "exprsubst.hpp"

// boo.. hiss.. a macro! (or two)
// These macros quickly register an overload for a function (or operator)
// to use used to build expressions
// opname is the type representing the expr op
// fnname is the C++ function to be overloaded
// ALLOWEXPRFROM2 is for two-argument functions
#define ALLOWEXPRFROM2(opname,fnname) \
	template<typename X, typename Y, \
     std::enable_if_t<isexpr_v<std::decay_t<X>> \
                         || isexpr_v<std::decay_t<Y>>,int> = 0> \
auto fnname(X &&x, Y &&y) { \
     using XT = std::decay_t<X>; \
     using YT = std::decay_t<Y>; \
     if constexpr (isexpr_v<XT> && isexpr_v<YT>) \
          return buildexpr(opname{},std::forward<X>(x),std::forward<Y>(y)); \
     else if constexpr (isexpr_v<XT>) { \
          if constexpr (ismem_v<YT,exprvalue_t<XT>>) \
               return buildexpr(opname{},std::forward<X>(x), \
                         newconst<YT,XT>(std::forward<Y>(y))); \
          else \
               return buildexpr(opname{},std::forward<X>(x), \
                         newconst<YT>(std::forward<Y>(y))); \
     } else { \
          if constexpr (ismem_v<XT,exprvalue_t<YT>>) \
               return buildexpr(opname{}, \
                         newconst<XT,YT>(std::forward<X>(x)), \
                         std::forward<Y>(y)); \
          else \
               return buildexpr(opname{}, \
                         newconst<XT>(std::forward<X>(x)), \
                         std::forward<Y>(y)); \
     }  \
}

// for one-argument functions
#define ALLOWEXPRFROM1(opname,fnname) \
	template<typename X, \
		std::enable_if_t<isexpr_v<std::decay_t<X>>, int> = 0> \
auto fnname(X &&x) { \
	return buildexpr(opname{},std::forward<X>(x)); \
}

// add overloads to allow natural syntax for expr building:

ALLOWEXPRFROM2(addop,operator+)
ALLOWEXPRFROM2(subop,operator-)
ALLOWEXPRFROM2(mulop,operator*)
ALLOWEXPRFROM2(divop,operator/)
ALLOWEXPRFROM1(negop,operator-)
ALLOWEXPRFROM1(logop,log)
ALLOWEXPRFROM2(powop,pow)


// builds the expression y|_{x=z}  (yes, the ordering is weird... maybe change?)
template<typename X, typename Y, typename Z,
	std::enable_if_t<isexpr_v<std::decay<X>>,int> = 0>
auto evalat(X &&x, Y &&y, Z &&z) {
	using YT = std::decay_t<Y>;
	using ZT = std::decay_t<Z>;
	if constexpr (isexpr_v<YT> && isexpr_v<ZT>)
		return buildexpr(evalatop{},std::forward<X>(x),
				std::forward<Y>(y),std::forward<Z>(z));
	else if constexpr (isexpr_v<ZT>)
		return buildexpr(evalatop{},std::forward<X>(x),
					newconst<YT>(std::forward<Y>(y)),
					std::forward<Z>(z));
	else if constexpr (isexpr_v<YT>)
		return buildexpr(evalatop{},std::forward<X>(x),
					std::forward<Y>(y),
					newconst<YT>(std::forward<Z>(z)));
	else return buildexpr(evalatop{},std::forward<X>(x),
					newconst<YT>(std::forward<Y>(y)),
					newconst<YT>(std::forward<Z>(z)));
}

//----------

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
