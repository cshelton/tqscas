#ifndef EXPRSUGARMACROS_H
#define EXPRSUGARMACROS_H

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

#endif
