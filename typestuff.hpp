#ifndef TYPESTUFF_H
#define TYPESTUFF_H

#include <optional>
#include <variant>
#include <tuple>
#include <typeinfo>
#include <typeindex>

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
// checks to see if A1==Tmpl<A2>
template<template<typename> typename Tmpl, typename V1, typename V2>
constexpr bool sametypeswrap(const V1 &v1, const V2 &v2) {
	return std::visit([&v2](auto&&arg) {
				using A = std::decay_t<decltype(arg)>;
				return istype<Tmpl<A>>(v2);
			},v1);
}

//----

/* // TODO: remove completely
// Does this exist in C++17 -- check

template<typename>
struct tmplparam{ };

template<template<typename> typename T, typename TT>
struct tmplparam<T<TT>> {
	using type = TT;
};

template<typename T>
using tmplparam_t = typename tmplparam<T>::type;
*/

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


/* TODO: remove completely

template<template<typename...> typename T, typename A, typename... Adds>
struct repacknomonoadd;

template<template<typename...> typename T, typename... Adds>
struct repacknomonoadd<T, std::tuple<>, Adds...> {
	using type = T<Adds...>;
};

template<template<typename...> typename T,
			typename Arg1, typename... Args, typename... Adds>
struct repacknomonoadd<T, std::tuple<Arg1,Args...>, Adds...> {
	using type = typename std::conditional<
		std::is_same<Arg1,std::monostate>::value,
		typename repacknomonoadd<T,std::tuple<Args...>,Adds...>::type,
		typename repacknomonoadd<T,std::tuple<Args...>,Arg1,Adds...>::type
			>::type;
};

template<template<typename...> typename T,
			typename... Args, typename... Adds>
struct repacknomonoadd<T, std::variant<Args...>, Adds...> {
	using type = typename repacknomonoadd<T,std::tuple<Args...>,Adds...>::type;
};

template<template<typename...> typename T, typename... Args>
using repacknomonoadd_t = typename repacknomonoadd<T,Args...>::type;
*/

//-----

// variant types as sets of types

template<typename T, typename... Ts>
struct ismem { static constexpr bool value = false; };

template<typename T, typename U, typename... Ts>
struct ismem<T,U,Ts...> {
	static constexpr bool value = std::conditional<
		std::is_same<T,U>::value,
		std::true_type,
		ismem<T,Ts...>>::type::value;
};

template<typename T, typename... Ts>
inline constexpr bool ismem_v = ismem<T,Ts...>::value;

template<typename, typename...>
struct variantunionhelp {};

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
using variantunion_t = typename variantunion<V1,V2>::type;

// variant "upgrade" to superset

template<typename Vsuper, typename Vsub>
Vsuper upgradevariant(Vsub &&v) {
	return std::visit([](auto &&x) -> Vsuper { return x; },v);
}


#endif
