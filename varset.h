#ifndef VARSET_H
#define VARSET_H

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <string.h>

template<typename T>
struct var {
	constexpr var(const char *n) : name(n) {}
	const char *name;
};

template<typename... Ts> struct varset {};

template<typename T, typename Enable=void, typename... Ts>
struct common_or_void {
	typedef void type;
};

template<typename T, typename... Ts>
struct common_or_void<T,typename std::common_type<T,Ts...>::type,Ts...> {
	typedef typename std::common_type<T,Ts...>::type type;
};

template<typename T, typename... Ts>
struct varset<T,Ts...> {
	template<typename NT, typename... NTS>
	constexpr varset(NT &&n, NTS &&... ns)
		: v(std::forward<NT>(n)), vs(std::forward<NTS>(ns)...) {}

	var<T> v;
	varset<Ts...> vs;

	typedef typename common_or_void<T,Ts...>::type commontype;
};

template<typename RETT, typename S, typename... Ss>
constexpr RETT
getarg(const char *n, const varset<> &vs, const S &arg1, const Ss&... args) {
	static_assert(true,"too many arguments");
	return RETT{};
}

/*
template<typename RETT, typename T, typename S>
constexpr typename std::enable_if<std::is_convertible<T,RETT>::value,RETT>::type
getarg(const char *n, const varset<T> &vs, const S &arg1) {
	return (strcmp(n,vs.v.name)==0) ? arg1 : RETT{};
	//throw std::out_of_range(std::string("could not find variable named ")+n);
}

template<typename RETT, typename T, typename S>
constexpr typename std::enable_if<!std::is_convertible<T,RETT>::value,RETT>::type
getarg(const char *n, const varset<T> &vs, const S &arg1) {
	return RETT{};//throw std::out_of_range(std::string("could not find variable named ")+n);
}
*/

template<typename RETT, typename T, typename S, typename... Ts,typename... Ss>
//constexpr typename varset<T,Ts...>::commontype
constexpr typename std::enable_if<std::is_convertible<T,RETT>::value,RETT>::type
getarg(const char *n, const varset<T,Ts...> &vs, const S &arg1, const Ss&... args) {
	return (strcmp(n,vs.v.name)==0) ? arg1 : getarg<RETT>(n,vs.vs,args...);
}

template<typename RETT, typename T, typename S, typename... Ts,typename... Ss>
//constexpr typename varset<T,Ts...>::commontype
constexpr typename std::enable_if<!std::is_convertible<T,RETT>::value,RETT>::type
getarg(const char *n, const varset<T,Ts...> &vs, const S &arg1, const Ss&... args) {
	return getarg<RETT>(n,vs.vs,args...);
}

#endif
