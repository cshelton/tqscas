#ifndef SYMMATHCALC_H
#define SYMMATHCALC_H

#include "symmath.h"

template<typename R, typename RX>
constexpr auto derivative(const constsym<R> &f, const sym<RX> &x) {
	return constsym<R>(0);
}

template<typename R>
constexpr auto derivative(const sym<R> &f, const sym<R> &x) {
	return f.sameas(x) ? constsym<R>(1) : constsym<R>(0);
}

template<typename F, typename RX>
constexpr auto derivative(const applyop<fnnegate,F> &f, const sym<RX> &x) {
	return -derivative(std::get<0>(f.fs),x);
}

template<typename F1, typename F2, typename RX>
constexpr auto derivative(const applyop<fnplus,F1,F2> &f, const sym<RX> &x) {
	return derivative(std::get<0>(f.fs),x) +
					derivative(std::get<1>(f.fs),x);
}

template<typename F1, typename F2, typename RX>
constexpr auto derivative(const applyop<fnmultiplies,F1,F2> &f, const sym<RX> &x) {
	return derivative(std::get<0>(f.fs),x)*std::get<1>(f.fs)
			+ std::get<0>(f.fs)*derivative(std::get<1>(f.fs),x);
}

template<typename R, typename RX>
constexpr auto integral(const constsym<R> &f, const sym<RX> &x) {
	return f*x;
}

template<typename R, typename RX>
constexpr auto integral(const sym<R> &f, const sym<RX> &x) {
	return constsym<R>(0.5)*f*f;
}

template<typename F, typename RX>
constexpr auto integral(const applyop<fnnegate,F> &f, const sym<RX> &x) {
	return -integral(std::get<0>(f.fs),x);
}

template<typename F1, typename F2, typename RX>
constexpr auto integral(const applyop<fnplus,F1,F2> &f, const sym<RX> &x) {
	return integral(std::get<0>(f.fs),x) +
					integral(std::get<1>(f.fs),x);
}

#endif

