#ifndef EXPRBASICOPS_H
#define EXPRBASICOPS_H

#include "exprbase.hpp"
#include "exprtostr.hpp"
#include "exprcoreops.hpp"

// ops included (and "definitions" default to those in C++)
// +, -, *, /, unary-, pow, log
// (%/rem commented out b/c no consistent definition)

struct addop {};
struct subop {};
struct mulop {};
struct remop {};
struct divop {};
struct negop {};
struct powop {};
struct logop {};

// addition:
template<typename T1, typename T2,
	int_t<decltype(std::declval<std::decay_t<T1>>() +
			std::declval<std::decay_t<T2>>())> = 0>
auto evalop(const addop &, T1 &&v1, T2 &&v2) {
	return std::forward<T1>(v1)+std::forward<T2>(v2);
}
std::string symbol(const addop &) { return "+"; }
int precedence(const addop &) { return 6; }

// subtraction:
template<typename T1, typename T2,
	int_t<decltype(std::declval<std::decay_t<T1>>() -
			std::declval<std::decay_t<T2>>())> = 0>
auto evalop(const subop &, T1 &&v1, T2 &&v2) {
	return std::forward<T1>(v1)-std::forward<T2>(v2);
}
std::string symbol(const subop &) { return "-"; }
int precedence(const subop &) { return 6; }

// multiplication:
template<typename T1, typename T2,
	int_t<decltype(std::declval<std::decay_t<T1>>() *
			std::declval<std::decay_t<T2>>())> = 0>
auto evalop(const mulop &, T1 &&v1, T2 &&v2) {
	return std::forward<T1>(v1)*std::forward<T2>(v2);
}
std::string symbol(const mulop &) { return "*"; }
int precedence(const mulop &) { return 5; }

// division:
template<typename T1, typename T2,
	int_t<decltype(std::declval<std::decay_t<T1>>() /
			std::declval<std::decay_t<T2>>())> = 0>
auto evalop(const divop &, T1 &&v1, T2 &&v2) {
	return std::forward<T1>(v1)/std::forward<T2>(v2);
}
std::string symbol(const divop &) { return "/"; }
int precedence(const divop &) { return 5; }

/* removed b/c std::remainder and % (rem op) have different semantics
 * have not decided which one to use
// remainder:
template<typename T1, typename T2, typename=void>
struct haveperrem : std::false_type {};
template<typename T1, typename T2>
struct haveperrem<T1,T2,
	std::void_t<decltype(std::declval<T1>()%std::declval<T2>())>>
		: std::true_type {};
template<typename T1, typename T2, typename=void>
struct haverem : std::false_type {};
template<typename T1, typename T2>
struct haverem<T1,T2,
	std::void_t<decltype(remainder(std::declval<T1>(),std::declval<T2>()))>>
		: std::true_type {};
template<typename T1, typename T2, typename=void>
struct havestdrem : std::false_type {};
template<typename T1, typename T2>
struct havestdrem<T1,T2,
  std::void_t<decltype(std::remainder(std::declval<T1>(),std::declval<T2>()))>>
		: std::true_type {};

template<typename T1, typename T2>
auto evalop(const remop &, T1 &&v1, T2 &&v2) {
	if constexpr (haveperrem<T1,T2>)
		return std::forward<T1>(v1)%std::forward<T2>(v2);
	else if constexpr (haverem<T1,T2>)
		return remainder(std::forward<T1>(v1),std::forward<T2>(v2));
	else if constexpr (havestdrem<T1,T2>)
		return std::remainder(std::forward<T1>(v1),std::forward<T2>(v2));
	else return std::monostate{};
}
std::string symbol(const remop &) { return "%"; }
int precedence(const remop &) { return 5; }
*/

// negation:
template<typename T1, typename T2,
	int_t<decltype(-std::declval<std::decay_t<T1>>())> = 0>
auto evalop(const negop&, T1 &&v1) {
	return -std::forward<T1>(v1);
}
std::string symbol(const negop &) { return "~"; } // maybe change to "~"?
int precedence(const negop &) { return 3; }

// power:
template<typename T1, typename T2, typename=void>
struct havepow : std::false_type {};
template<typename T1, typename T2>
struct havepow<T1,T2,
	std::void_t<decltype(pow(std::declval<T1>(),std::declval<T2>()))>>
		: std::true_type {};
template<typename T1, typename T2, typename=void>
struct havestdpow : std::false_type {};
template<typename T1, typename T2>
struct havestdpow<T1,T2,
	std::void_t<decltype(std::pow(std::declval<T1>(),std::declval<T2>()))>>
		: std::true_type {};
template<typename T1, typename T2,
	std::enable_if_t<havepow<std::decay_t<T1>,std::decay_t<T2>>::value
		|| havestdpow<std::decay_t<T1>,std::decay_t<T2>>::value,int> = 0>
auto evalop(const powop &, T1 &&v1, T2 &&v2) {
	if constexpr(havepow<std::decay_t<T1>,std::decay_t<T2>>::value)
		return pow(std::forward<T1>(v1),std::forward<T2>(v2));
	else return std::pow(std::forward<T1>(v1),std::forward<T2>(v2));
}
std::string symbol(const powop &) { return "pow"; }
int precedence(const powop &) { return 4; }
std::string write(const powop &op,
		const std::vector<std::pair<std::string,int>> &subst)
{ return writeasfunc(op,subst); }

// log:
template<typename T1, typename=void>
struct haslog: std::false_type {};
template<typename T1>
struct haslog<T1, std::void_t<decltype(log(std::declval<T1>()))>>
		: std::true_type {};
template<typename T1, typename=void>
struct hasstdlog: std::false_type {};
template<typename T1>
struct hasstdlog<T1, std::void_t<decltype(std::log(std::declval<T1>()))>>
		: std::true_type {};
template<typename T1, 
	std::enable_if_t<haslog<std::decay_t<T1>>::value
		|| hasstdlog<std::decay_t<T1>>::value,int> = 0>
auto evalop(const logop &, T1 &&v1) {
	if constexpr(haslog<std::decay_t<T1>>::value)
		return log(std::forward<T1>(v1));
	else return std::log(std::forward<T1>(v1));
}
std::string symbol(const logop &) { return "log"; }
int precedence(const logop &) { return 4; }
std::string write(const logop &op,
		const std::vector<std::pair<std::string,int>> &subst)
{ return writeasfunc(op,subst); }


#endif
