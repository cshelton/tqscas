#ifndef EXPRSUGAR_H
#define EXPRSUGAR_H

#include "exprbasicops.hpp"

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



#endif
