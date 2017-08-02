
using varnameT = char;

// below could be replaced with integral_constant at the moment
template<typename T, T v>
struct staticval {
	static constexpr T value = v;
	typedef T value_type;
	typedef staticval type;
	constexpr operator value_type() const noexcept { return value; }
	constexpr value_type operator()() const noexcept { return value; }
	static constexpr bool isgrounded = true;
};

template<char...> struct tostaticval;

template<char c1, char c2, char... chars>
struct tostaticval<c1,c2,chars...> {
	enum {radix = tostaticval<c2,chars...>::radix*10};
	typedef staticval<long,radix*(c1-'0')+tostaticval<c2,chars...>::type::value> type;
};

template<char c1>
struct tostaticval<c1> {
	enum {radix = 1};
	typedef staticval<long,(c1-'0')> type;
};
	
template <char... chars>
constexpr auto operator"" _c() { return typename tostaticval<chars...>::type{}; }

template<typename T, varnameT name>
struct staticvar {
	typedef T value_type;
	typedef staticvar type;
	//constexpr operator value_type() const noexcept { return eval(); }
	//constexpr value_type operator()() const noexcept { return eval(); }
	static constexpr bool isgrounded = false;
};

template<typename...> struct allgrounded;

template<typename T1, typename... Ts>
struct allgrounded<T1,Ts...> {
	enum{value=T1::isgrounded && allgrounded<Ts...>::value};
};
template<>
struct allgrounded<> {
	enum{value=true};
};

template<template<typename...> class OP, typename... Ts>
struct staticop {
	static constexpr auto value = OP<Ts...>::value;
	typedef decltype(value) value_type;
	typedef staticop type;
	constexpr operator value_type() const noexcept { return value; }
	constexpr value_type operator()() const noexcept { return value; }
	static constexpr bool isgrounded = allgrounded<Ts...>::value;
};
	
template<typename A1, typename A2>
struct plusop {
	static constexpr auto value = A1::value + A2::value;
};

template<typename A1, typename A2>
struct plusmult {
	static constexpr auto value = A1::value * A2::value;
};
