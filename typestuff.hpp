#ifndef TYPESTUFF_H
#define TYPESTUFF_H

#include <optional>
#include <variant>
#include <tuple>
#include <typeinfo>
#include <typeindex>
#include "typecmp.hpp"

template<typename T1, typename T2, typename=void>
struct haveeq : std::false_type {};
template<typename T1, typename T2>
struct haveeq<T1,T2,
     std::void_t<decltype((std::declval<T1>())==(std::declval<T2>()))>>
          : std::true_type {};

template<typename T1, typename T2>
inline constexpr bool haveeq_v = haveeq<T1,T2>::value;

// this looks like it is the same as variant<...>::operator==
// except that the two variants don't have to have the same set of types!
template<typename... T1s, typename... T2s>
bool varianteq(const std::variant<T1s...> &x, const std::variant<T2s...> &y) {
	return std::visit([](auto &&a, auto &&b) -> bool {
			if constexpr (haveeq_v<std::decay_t<decltype(a)>,
								std::decay_t<decltype(b)>>)
					return a==b;
			else return false;
			}, x, y);
}

// would like to use
/*
template<typename...>
using int_t = int;
*/
// but must use
template<typename...> struct make_int { typedef int type; };
template<typename...Ts> using int_t = typename make_int<Ts...>::type;
// because of CWG 1558 (see https://en.cppreference.com/w/cpp/types/void_t)
// until compiler fixes it

template<typename T>
struct variantfirst {};

template<typename T, typename... Ts>
struct variantfirst<std::variant<T,Ts...>> {
	using type = T;
};

template<typename T>
using variantfirst_t = typename variantfirst<T>::type;

// note: not needed, std::holds_alternative does exactly this!
template<typename T,typename ...Ts>
constexpr bool istype(const std::variant<Ts...> &v) {
	return std::holds_alternative<T>(v);
	/*
	return std::visit([](auto&&arg) {
			using A = std::decay_t<decltype(arg)>;
			return std::is_same_v<T,A>;
			},v);
			*/
}

template<typename T, typename ...Ts>
constexpr bool isderivtype(const std::variant<Ts...> &v) {
	return std::visit([](auto&&arg) {
			using A = std::decay_t<decltype(arg)>;
			return std::is_base_of_v<T,A>;
			},v);
}

template<typename T, typename ...Ts>
constexpr T convertto(const std::variant<Ts...> &v) {
	return std::visit([](auto &&x) -> T {
			return x; },v);
}


//----

// checks if T == Tmpl<X> for some X
template<template<typename> typename Tmpl, typename T>
struct istmpl : std::false_type {};

template<template<typename> typename Tmpl, typename... X>
struct istmpl<Tmpl,Tmpl<X...>> : std::true_type{};

template<template<typename> typename Tmpl, typename T>
inline constexpr bool istmpl_v = istmpl<Tmpl,T>::value;


//----

// V1 and V2 should be std::variant<...>
// checks to see if they are currently holding the same type
template<typename V1, typename V2>
constexpr bool sametypes(const V1 &v1, const V2 &v2) {
	return std::visit([&v2](auto&&arg) {
				using A = std::decay_t<decltype(arg)>;
				return istype<A>(v2);
			},v1);
}

// same as sametypes above, but
//   let A1 be type stored in V1
//   and A2 be type stored in V2
// checks to see if Tmpl<A1>==A2
template<template<typename> typename Tmpl, typename V1, typename V2>
constexpr bool sametypeswrap(const V1 &v1, const V2 &v2) {
	return std::visit([&v2](auto&&arg) {
				using A = std::decay_t<decltype(arg)>;
				return istype<Tmpl<A>>(v2);
			},v1);
}

//----
// to change variant<A,B,C> into variant<T<A>,T<B>,T<C>>

template<template<typename> typename T, typename V>
struct innerwrap{};

template<template<typename> typename T, typename... ABC>
struct innerwrap<T,std::variant<ABC...>> {
	using type = std::variant<T<ABC>...>;
};

template<template<typename> typename T, typename V>
using innerwrap_t = typename innerwrap<T,V>::type;

//-----

template<template<typename...> typename T, typename A>
struct repack;

template<template<typename...> typename T, typename... Args>
struct repack<T, std::tuple<Args...>> {
	typedef T<Args...> type;
};

template<template<typename...> typename T, typename... Args>
struct repack<T, std::variant<Args...>> {
	typedef T<Args...> type;
};

template<template<typename...> typename T, typename... Args>
using repack_t = typename repack<T,Args...>::type;


//-----

// variant types as sets of types
// currently sorted, but ismem doesn't rely on this

template<typename T, typename... Ts>
struct ismem { static constexpr bool value = false; };

template<typename T, typename U, typename... Ts>
struct ismem<T,U,Ts...> {
	static constexpr bool value = std::conditional<
		std::is_same<T,U>::value,
		std::true_type,
		ismem<T,Ts...>>::type::value;
};

template<typename T, typename V>
struct varismem : std::false_type {};

template<typename T, typename... Ts>
struct varismem<T,std::variant<Ts...>> : ismem<T,Ts...> {};

template<typename T, typename... Ts>
inline constexpr bool ismem_v = ismem<T,Ts...>::value;

template<typename T, typename V>
inline constexpr bool varismem_v = varismem<T,V>::value;

template<typename, typename...>
struct variantunionhelp {};


template<typename T, typename V>
struct tuplcons;

template<typename T, typename... Ts>
struct tuplcons<T,std::tuple<Ts...>> {
	using type = std::tuple<T,Ts...>;
};

template<typename T>
struct tuplcar;

template<typename T, typename... Ts>
struct tuplcar<std::tuple<T,Ts...>> {
	using type = T;
};

template<typename T>
struct tuplcdr;

template<typename T, typename...Ts>
struct tuplcdr<std::tuple<T,Ts...>> {
	using type = std::tuple<Ts...>;
};

template<typename S, typename T>
struct inserttuple;

template<typename S>
struct inserttuple<S,std::tuple<>> {
	using type = std::tuple<S>;
};

template<typename S, typename T, typename... Ts>
struct inserttuple<S,std::tuple<T,Ts...>> {
	using type = typename std::conditional<
		cmp<S,T>(),
			std::tuple<S,T,Ts...>,
		//else
			typename tuplcons<T,typename inserttuple<
								S,std::tuple<Ts...>>::type>::type
		>::type;
};

template<typename V>
struct sorttuple;

template<typename T>
struct sorttuple<std::tuple<T>> {
	using type = std::tuple<T>;
};

template<typename T, typename S, typename... Ts>
struct sorttuple<std::tuple<T,S,Ts...>> {
	using postsort = typename sorttuple<std::tuple<S,Ts...>>::type;
	using postcar = typename tuplcar<postsort>::type;
	using postcdr = typename tuplcdr<postsort>::type;
	using type = typename std::conditional<
			cmp<T,postcar>(),
				typename tuplcons<T,postsort>::type,
			//else
			// bubble sort:  replace sorttuple below with insert tuple
			//  to make insertion sort
				typename tuplcons<postcar,
							typename inserttuple<T,postcdr>::type
						>::type
		>::type;
};

template<typename T>
using sorttuple_t = typename sorttuple<T>::type;

template<typename V>
using sortvariant_t = repack_t<std::variant,sorttuple_t<repack_t<std::tuple,V>>>;

template<typename... Ts>
struct variantunionhelp<std::variant<Ts...>> {
	using type = std::variant<Ts...>;
};

template<typename... Ts, typename U, typename... Us>
struct variantunionhelp<std::variant<Ts...>,U,Us...> {
	using type = typename std::conditional<
		ismem_v<U,Ts...>,
		typename variantunionhelp<std::variant<Ts...>,Us...>::type,
		typename variantunionhelp<std::variant<Ts...,U>,Us...>::type>::type;
};

template<typename, typename>
struct variantunion{};

template<typename V, typename... Us>
struct variantunion<V,std::variant<Us...>> {
	using type = typename variantunionhelp<V,Us...>::type;
};

template<typename V1, typename V2>
using variantunion_t = sortvariant_t<typename variantunion<V1,V2>::type>;
//using variantunion_t = typename variantunion<V1,V2>::type;

// variant "upgrade" to superset
template<typename Vsuper, typename Vsub>
Vsuper upgradevariant(Vsub &&v) {
	return std::visit([](auto &&x) -> Vsuper { return x; },v);
}

// variant "regrade" (?) to Vret, if possible
template<typename Vret, typename Vinit>
std::optional<Vret> regradevariant(Vinit &&v) {
	return std::visit([](auto &&x) -> std::optional<Vret> {
			using X = std::decay_t<decltype(x)>;
			if constexpr (varismem_v<X,Vret>)
				return std::optional<Vret>{x};
			else return {};
		},v);
}

//--------------

template<typename... Ts>
constexpr bool isidmem(const std::type_info &ti) {
	return (... || (ti==typeid(Ts)));
}

//--------------

template<typename...>
struct typefoldimpl;

template<>
struct typefoldimpl<> {
	template<typename S, typename F>
	constexpr static auto exec(S &&s, F &&f) { return s; }
};

template<typename T, typename... Ts>
struct typefoldimpl<T,Ts...> {
	template<typename S, typename F>
	constexpr static auto exec(S &&s, F &&f) {
		return f.template operator()<T>(
				typefoldimpl<Ts...>::exec(std::forward<S>(s),f)
			);
	}
};

template<typename>
struct tuplefoldimpl;

template<typename... Ts>
struct tuplefoldimpl<std::tuple<Ts...>> {
	template<typename S, typename F>
	constexpr static auto exec(S &&s, F &&f) {
		return typefoldimpl<Ts...>::exec(std::forward<S>(s),std::forward<F>(f));
	}
};

template<typename S, typename F, typename... Ts>
constexpr auto typefold(S &&s, F &&f) {
	return typefoldimpl<Ts...>::exec(std::forward<S>(s), std::forward<F>(f));
}

template<typename Tup, typename S, typename F>
constexpr auto tuplefold(S &&s, F &&f) {
	return tuplefoldimpl<Tup>::exec(std::forward<S>(s), std::forward<F>(f));
}

#endif
