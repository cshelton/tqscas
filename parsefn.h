#ifndef PARSEFN_H
#define PARSEFN_H

#include "mathfn.h"
#include <stdexcept>

// much from https://akrzemi1.wordpress.com/2011/05/11/parsing-strings-at-compile-time-part-i/

template<std::size_t N>
constexpr char nth(const char (&str)[N], std::size_t i) {
	return i>=N-1 ? throw std::out_of_range("string length exceeded parsing function expression") : str[i];
}


class exprstack {
public:
	constexpr exprstack(const exprstack &s, std::size_t i0) {
		


#endif
