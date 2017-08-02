#ifndef TYPESET_H
#define TYPESET_H

#include <type_traits>

template<typename...> struct typeset {};

template<typename,typename>
struct typesethas {
	enum {value=0 };
	enum {only=0 };
};

template<typename T>
struct typesethas<typeset<T>,T> {
	enum {value=1 };
	enum {only=1 };
};

template<typename T, typename T2, typename... Ts>
struct typesethas<typeset<T2,Ts...>, T> {
	enum {value=typesethas<typeset<Ts...>,T>::value };
	enum {only=0 };
};

template<typename T, typename... Ts>
struct typesethas<typeset<T,Ts...>, T> {
	enum {value=1 };
	enum {only=0 };
};

template<typename>
struct onlyelement { };

template<typename T>
struct onlyelement<typeset<T>> {
	typedef T type;
};

template<typename>
struct commontype { };

template<typename... Ts>
struct commontype<typeset<Ts...>> {
	typedef typename std::common_type<Ts...>::type type;
};
	

#endif
