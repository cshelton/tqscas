#ifndef MATHFN_H
#define MATHFN_H

#include <utility>
#include <type_traits>
#include <cmath>

/*
template<typename DOMAIN, typename RANGE>
struct mathfn {
	typedef DOMAIN domaine;
	typedef RANGE range;

	assign operator()(range)
}
*/


template<typename ASSIGN>
struct constfn {
	typedef anytype domain;
	typedef typename ASSIGN::vartype;

	ASSIGN k;

	template<typename X>
	constexpr ASSIGN operator(const X &) const { return k; }
};

struct identityfn {
	typedef anytype domain;
	typedef anytype range;

	template<typename X>
	constexpr const assigntype<X> &operator(const assigntype<X> &x) const { return x; }
};

#endif
