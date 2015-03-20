#ifndef CON_H
#define CON_H

#include <type_traits>

struct anytype {};
struct notype {
/*
	template<typename T>
	operator T () {
		static_assert(false,"cannot deduce type");
	}
*/
};


template<typename T1, typename T2, typename E=void>
struct con_help {
	typedef notype type;
};

template<typename T1>
struct con_help<T1,anytype,void> {
	typedef T1 type;
};

template<typename T1>
struct con_help<anytype,T1,std::enable_if<!std::is_same<anytype,T1>::value>> {
	typedef T1 type;
};

void voidfn();

template<typename T1, typename T2>
struct con_help<T1,T2,decltype(true ? std::declval<T1>() : std::declval<T2>(), voidfn())> {
	typedef typename std::decay<decltype(true ? std::declval<T1>() : std::declval<T2>())>::type type;
};

template<typename ...> struct common_or_none;

template<>
struct common_or_none<> {
	typedef void type;
};

template<typename T>
struct common_or_none<T> {
	typedef T type;
};


template<typename T1, typename T2>
struct common_or_none<T1,T2> {
	typedef typename con_help<T1,T2>::type type;
};

template<typename T1, typename T2, typename... Ts>
struct common_or_none<T1,T2,Ts...> {
	typedef typename common_or_none<typename common_or_none<T1,T2>::type,Ts...>::type type;
};

#endif
