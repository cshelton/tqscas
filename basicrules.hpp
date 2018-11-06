#ifndef BASICRULES_HPP
#define BASICRULES_HPP

#include "rewrite.hpp"
#include "basicops.hpp"
#include "coreops.hpp"
#include "scalarops.hpp"
#include "sugar.hpp"

template<typename E>
bool ismatchexp(const E &e) {
	if (e.isleaf()) return ismatcher(e);
	else return ismatcher(e) && e.children().size()==1
		&& ismatchexp(e.children()[0]);
}

template<typename E>
E chainpatternmod(const E &ex) {
	return ex.map([](const E &e) {
			if (!e.isleaf()) {
				const auto &ch = e.children();
				return std::visit([&ch](auto &&o) {
					using O = std::decay_t<decltype(o)>;
					if constexpr (istmpl_v<binarychainop,O>) {
						using BOP = typename O::baseopT;
						return std::optional<E>{std::in_place,
							matchcommop<BOP,ismatcher(ch.back())>{},ch};
					} else return std::optional<E>{};
					},e.asnode().asvariant());
			} else return std::optional<E>{};
		});
}

template<typename E>
auto tochainops(const E &ex) {
	using Eret = exprunion_t<expr1op_t<binarychainop<addop>>,
		 		expr1op_t<binarychainop<mulop>>,
				E>;

	return upgradeexpr<Eret>(ex).map([](const Eret &e) {
			if (!e.isleaf()) {
				const auto &ch = e.children();
				return std::visit([&ch](auto &&o) {
						using O = std::decay_t<decltype(o)>;
						if constexpr (std::is_same_v<addop,O>)
							return std::optional<Eret>{std::in_place,
								buildexprvec<binarychainop<addop>,Eret>
									(binarychainop<addop>{},ch)};
						else if constexpr (std::is_same_v<mulop,O>)
							return std::optional<Eret>{std::in_place,
								buildexprvec<binarychainop<mulop>,Eret>
									(binarychainop<mulop>{},ch)};
						else return std::optional<Eret>{};
					},e.asnode().asvariant());
			} else return std::optional<Eret>{};
		});
}

template<typename E, typename E1, typename E2>
auto SRR(E1 &&s, E2 &&p) {
     return SR<E>(tochainops(chainpatternmod(std::forward<E1>(s))),
				tochainops(std::forward<E2>(p)));
}

template<typename E, typename E1, typename E2, typename F>
auto SRR(E1 &&s, E2 &&p, F &&f) {
     return SR<E>(tochainops(chainpatternmod(std::forward<E1>(s))),
				tochainops(std::forward<E2>(p)),
				std::forward<F>(f));
}

template<typename E>
ruleset<E> baseruleset{{
	std::make_shared<trivialconsteval<E>>(),
	std::make_shared<evalatrewrite<E>>(),
	std::make_shared<optochain<E,binarychainop<addop>>>(),
	std::make_shared<optochain<E,binarychainop<mulop>>>(),
	setreorderable<E,addop,binarychainop<addop>,
				mulop,binarychainop<mulop>>(
		createsortrewrite<E,negop,addop,binarychainop<addop>,subop,
				mulop,binarychainop<mulop>,divop,
				powop,logop,absop,derivop,evalatop>()
			),
	std::make_shared<collapsechain<E,binarychainop<addop>>>(),
	std::make_shared<collapsechain<E,binarychainop<mulop>>>(),
	std::make_shared<constchaineval<E,binarychainop<addop>>>(),
	std::make_shared<constchaineval<E,binarychainop<mulop>>>(),
	SRR<E>(E1_+E1_, 2*P1_),
}};

#endif
