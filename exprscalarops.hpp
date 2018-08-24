#ifndef EXPRSCALAROPS_H
#define EXPRSCALAROPS_H

#include "exprbasicops.hpp"
#include <cmath>
#include <type_traits>

#include <iostream> // TODO: delete

// defines two operators, for scalars:
// abs and deriv

// has one child
struct absop {};

// has three children: represents deriv of ch[1] wrt ch[0] eval at ch[0]=ch[2]
// (so ch[0] is local to the subexprs)
struct derivop : scopeop {};


// NOTE: this is *not* the same as std::is_scalar !!
template<typename T>
inline constexpr bool is_numeric_scalar_v = std::is_arithmetic_v<T>;

template<typename T,
				std::enable_if_t<std::is_integral_v<T>,int> =0>
static constexpr T epscbrt() { return T{1}; }

// Newton-Raphson to calculate (std::cbrt is not constexpr)
template<typename T>
constexpr T cecbrt(T x) {
	T y{1};
	while(1) {
		T newy = T{2}/T{3}*y - x/(T{3}*y*y);
		if (newy==y) break;
		y = newy;
	}
	return y;
}

template<typename T,
	std::enable_if_t<std::is_arithmetic_v<T> && !std::is_integral_v<T>,int> =0>
static constexpr T epscbrt() {
	return cecbrt(std::numeric_limits<T>::epsilon());
}

template<typename T,
	std::enable_if_t<is_numeric_scalar_v<T> && std::is_signed_v<T>,int> =0>
auto evalop(const absop &, T &&x) {
	return std::abs(std::forward<T>(x));
}
template<typename T,
	std::enable_if_t<is_numeric_scalar_v<T> && std::is_unsigned_v<T>,int> =0>
auto evalop(const absop &, T &&x) {
	return x;
}

std::string symbol(const absop &) { return "abs"; }
int precedence(const absop &) { return 10; } 
std::string write(const absop &op, 
		const std::vector<std::pair<std::string,int>> &subst) {
	return std::string("|")+subst[0].first+"|";
}


template<typename E>
exprvalue_t<E> evalnode(const derivop &, const std::vector<E> &ch) {
	// assuming that the goal is *numeric* differentiation
	// (otherwise, should apply rewrite rules first to remove derivop!)
	
	// a simple version of what's explained in 5.7 of Numerical Recipes in C
	// (not the full Ridders algorithm, just symmetric difference)
	auto xv = eval(ch[2]);
	auto &v = ch[0];
	auto &e = ch[1];
	if (!std::visit([](auto &&a, auto &&b) {
				using A = std::decay_t<decltype(a)>;
				using B = std::decay_t<decltype(b)>;
				return std::is_same_v<A,typetype<B>>; },evaltype(v),xv))
		return noexprT{};

	return std::visit([&v,&e](auto &&x) -> exprvalue_t<E> {
				using X = std::decay_t<decltype(x)>;
				if constexpr (is_numeric_scalar_v<X>) {
					constexpr auto eps = epscbrt<X>();
					X h = eps*std::max(X{1},std::abs(x));
					X xplus = x+h, xminus = x-h;
					X xdiff = xplus-xminus; // might not be 2*h
					//std::cout << std::endl;
					//std::cout << h << ' ' << xplus << ' ' << xminus << ' ' << xdiff << std::endl;
					return std::visit([&xdiff](auto &&fp, auto &&fm)
									-> exprvalue_t<E> {
							using FP = std::decay_t<decltype(fp)>;
							using FM = std::decay_t<decltype(fm)>;
							if constexpr (is_numeric_scalar_v<FP>
									&& is_numeric_scalar_v<FM>)
								return (fp-fm)/xdiff;
							else return noexprT{};
						}, eval(e,v,xplus),eval(e,v,xminus));
				} else {
					return noexprT{};
				}
			}, xv);
}

template<typename E>
exprvaluetype_t<E> evaltypenode(const derivop &, const std::vector<E> &ch) {
	return evaltype(ch[1]);
}

std::string symbol(const derivop &) { return "der"; }
int precedence(const derivop &) { return 5; }
std::string write(const derivop &o,
		const std::vector<std::pair<std::string,int>> &subst) {
	if (subst[0].first==subst[2].first)
		return std::string("d/d")+
			subst[0].first+
			putinparen(subst[1].first,true);
	else return std::string("d/d")+
			subst[0].first+
			putinparen(subst[1].first,true)
			+"|_{"+subst[0].first+"="+subst[2].first+"}";
}

#endif
