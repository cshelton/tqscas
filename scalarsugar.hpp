#ifndef SCALARSUGAR_H
#define SCALARSUGAR_H

#include "scalarops.hpp"
#include "sugarmacros.hpp"

// add overloads to allow natural syntax for expr building:
ALLOWEXPRFROM1(absop,abs)

template<typename X, typename Y,
	std::enable_if_t<isexpr_v<std::decay_t<X>>,int> = 0>
auto deriv(const X &x, Y &&y) {
	return deriv(x,std::forward<Y>(y),x);
}

// builds the expression (dy/dx)_{x=z}  (yes, the ordering is weird... maybe change?)
// (taken directly from evalat, perhaps should be combined?)
template<typename X, typename Y, typename Z,
	std::enable_if_t<isexpr_v<std::decay_t<X>>,int> = 0>
auto deriv(X &&x, Y &&y, Z &&z) {
	using XT = std::decay_t<X>;
	using YT = std::decay_t<Y>;
	using ZT = std::decay_t<Z>;
	if constexpr (isexpr_v<YT> && isexpr_v<ZT>) {
		if (!varianteq(evaltype(x),evaltype(z))) 
			return newconst<noexprT,
				  exprunion_t<expr1op_t<derivop>,XT,YT,ZT>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(derivop{},
				std::forward<X>(x),
				std::forward<Y>(y),
				std::forward<Z>(z));
	} else if constexpr (isexpr_v<ZT>) {
		if (!varianteq(evaltype(x),evaltype(z))) 
			return newconst<noexprT,
				  exprunion_t<expr1op_t<derivop>,
				  			XT,expr1type_t<YT>,ZT>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(derivop{},
				std::forward<X>(x),
				newconst<YT>(std::forward<Y>(y)),
				std::forward<Z>(z));
	} else if constexpr (isexpr_v<YT>) {
		if (!istype<ZT>(evaltype(x)))
			return newconst<noexprT,
				  exprunion_t<expr1op_t<derivop>,
				  		XT,YT,expr1type_t<ZT>>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(derivop{},
				std::forward<X>(x),
				std::forward<Y>(y),
				newconst<ZT>(std::forward<Z>(z)));
	} else {
		if constexpr (!std::is_same_v<YT,ZT>)
			return newconst<noexprT,expr1type_t<YT>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(derivop{},
				std::forward<X>(x),
				newconst<YT>(std::forward<Y>(y)),
				newconst<ZT>(std::forward<Z>(z)));
	}
}

#endif
