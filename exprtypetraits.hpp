#ifndef EXPRTYPETRAITS_HPP
#define EXPRTYPETRAITS_HPP

#include <tuple>
#include "typestuff.hpp"

// an object (to be included in the C++ expression type) that
// records which C++ types are to be considered which mathematical type
//
// another C++ type (instead of exprtypetraits) can be used, provided it
// has the same interface

// more can be defined if desired
struct scalartrait{};
struct vectortrait{};
struct inttrait{};
struct realtrait{};
struct complextrait{};
struct matrixtrait{};
struct unsignedtrait{};

// to be interpreted as C++ type T has expr traits Traits
template<typename T, typename... Traits>
struct typetotrait {
	using Type = T;
	using TraitTup = std::tuple<Traits...>;
};

// exprtypetraits<A,B,C> is valid if A, B, and C are all "typetotrait"
// specifying the type traits for three C++ types
template<typename... TTs>
struct exprtypetraits {
	using traits = std::tuple<TTs...>; // tuple easy way of grouping
};

// interface:

template<typename T1>
struct anysamecheck {
	template<typename T2>
	constexpr bool operator()(bool b) const {
		return b | std::is_same_v<T1,T2>;
	}
};

/*
struct anysameidcheck {
	template<typename T2>
	constexpr bool operator()(bool b) const {
		return b | std::is_same_v<T1,T2>;
	}
};
*/

template<typename T1, typename F>
struct runcheck {
	const F f;

	constexpr runcheck(F ff) : f(std::move(ff)) {}

	template<typename T2>
	constexpr inline bool operator()(bool b) const {
		if constexpr (std::is_same_v<typename T2::Type,T1>)
			return tuplefold<typename T2::TraitTup>(false,f);
		else return b;
	}
};

template<typename F>
struct runcheckti {
	const std::type_info &ti;
	const F f;

	constexpr runcheckti(const std::type_info &tii, F ff)
		: ti(tii), f(std::move(ff)) {}

	// would like to make constexpr, but == is not
	template<typename T2>
	inline bool operator()(bool b) const {
		// would like to make constexpr, but == is not
		if (typeid(typename T2::Type)==ti)
			return tuplefold<typename T2::TraitTup>(false,f);
		else return b;
	}
};

template<typename T, typename Trait, typename ET>
constexpr bool hastrait
	= tuplefold<typename ET::traits>(false,runcheck<T,anysamecheck<Trait>>{anysamecheck<Trait>{}});

// would like this to be constexpr, but operator== on type_info
// is not constexpr!
template<typename Trait, typename ET>
inline bool tihastrait(const std::type_info &ti) {
	return tuplefold<typename ET::traits>(false,runcheckti<anysamecheck<Trait>>{ti,anysamecheck<Trait>{}});
}

using defaulttraits
	= exprtypetraits<
		typetotrait<int,scalartrait,inttrait>,
		typetotrait<unsigned int,scalartrait,inttrait,unsignedtrait>,
		typetotrait<long,scalartrait,inttrait>,
		typetotrait<unsigned long,scalartrait,inttrait,unsignedtrait>,
		typetotrait<long long,scalartrait,inttrait>,
		typetotrait<unsigned long long,scalartrait,inttrait,unsignedtrait>,
		typetotrait<short,scalartrait,inttrait>,
		typetotrait<unsigned short,scalartrait,inttrait,unsignedtrait>,
		typetotrait<float,scalartrait,realtrait>,
		typetotrait<double,scalartrait,realtrait>,
		typetotrait<long double,scalartrait,realtrait>
	>; // perhaps add to later?
		
#endif

