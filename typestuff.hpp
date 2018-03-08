#ifndef TYPESTUFF_H
#define TYPESTUFF_h

#include <optional>
#include <variant>
#include <tuple>
#include <typeinfo>
#include <typeindex>

template<typename T,typename ...Ts>
constexpr bool istype(const std::variant<Ts...> &v) {
	return std::visit([](auto&&arg) {
			using A = std::decay_t<decltype(arg)>;
			return std::is_same_v<T,A>;
			},v);
}

template<typename T, typename ...Ts>
constexpr bool isderivtype(const std::variant<Ts...> &v) {
	return std::visit([](auto&&arg) {
			using A = std::decay_t<decltype(arg)>;
			return std::is_base_of_v<T,A>;
			},v);
}

//----

// Does this exist in C++17 -- check

template<typename>
struct tmplparam{ };

template<template<typename> T, typename TT>
struct tmplparam<T<TT>> {
	using type = TT;
};

template<typename T>
using tmplparam_t = typename tmplparam<T>::type;

//-----

template<template<typename...> typename T, typename A>
struct unpackto;

template<template<typename...> typename T, typename... Args>
struct unpackto<T, std::tuple<Args...>> {
	typedef T<Args...> type;
};

template<template<typename...> typename T, typename... Args>
struct unpackto<T, std::variant<Args...>> {
	typedef T<Args...> type;
};

template<template<typename...> typename T, typename... Args>
using unpackto_t = typename unpackto<T,Args...>::type;

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

template<typename, typename...>
struct variantunionhelp {};

template<typename... Ts>
struct variantunionhelp<std::variant<Ts...>> {
	using type = std::variant<Ts...>;
};

template<typename... Ts, typename U, typename... Us>
struct variantunionhelp<std::variant<Ts...>,U,Us...> {
	using type = typename std::conditional<
		ismem<U,Ts...>::value,
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


#endif
