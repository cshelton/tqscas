#ifndef BASICSUGAR_H
#define BASICSUGAR_H

#include "basicops.hpp"
#include "sugarmacros.hpp"

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
	std::enable_if_t<isexpr_v<std::decay_t<X>>,int> = 0>
auto evalat(X &&x, Y &&y, Z &&z) {
	using XT = std::decay_t<X>;
	using YT = std::decay_t<Y>;
	using ZT = std::decay_t<Z>;
	if constexpr (isexpr_v<YT> && isexpr_v<ZT>) {
		if (!varianteq(evaltype(x),evaltype(z))) 
			return newconst<noexprT,
				  exprunion_t<expr1op_t<evalatop>,XT,YT,ZT>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(evalatop{},
				std::forward<X>(x),
				std::forward<Y>(y),
				std::forward<Z>(z));
	} else if constexpr (isexpr_v<ZT>) {
		if (!varianteq(evaltype(x),evaltype(z))) 
			return newconst<noexprT,
				  exprunion_t<expr1op_t<evalatop>,
				  			XT,expr1type_t<YT>,ZT>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(evalatop{},
				std::forward<X>(x),
				newconst<YT>(std::forward<Y>(y)),
				std::forward<Z>(z));
	} else if constexpr (isexpr_v<YT>) {
		if (!istype<ZT>(evaltype(x)))
			return newconst<noexprT,
				  exprunion_t<expr1op_t<evalatop>,
				  		XT,YT,expr1type_t<ZT>>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(evalatop{},
				std::forward<X>(x),
				std::forward<Y>(y),
				newconst<ZT>(std::forward<Z>(z)));
	} else {
		if constexpr (!std::is_same_v<YT,ZT>)
			return newconst<noexprT,expr1type_t<YT>>(noexprT{});
						// this should fail, perhaps
		else return buildexpr(evalatop{},
				std::forward<X>(x),
				newconst<YT>(std::forward<Y>(y)),
				newconst<ZT>(std::forward<Z>(z)));
	}
}

#endif
