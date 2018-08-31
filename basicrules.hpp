#ifndef BASICRULES_HPP
#define BASICRULES_HPP

#include "rewrite.hpp"
#include "basicops.hpp"

template<typename E>
ruleset<E> baseruleset{{
	std::make_shared<trivialconsteval<E>>(),
	std::make_shared<evalatrewrite<E>>(),
	setreorderable<E,addop,binarychainop<addop>,
				mulop,binarychainop<mulop>>(
		createsortrewrite<E,negop,addop,binarychainop<addop>,subop,
				mulop,binarychainop<mulop>,divop,
				powop,logop,absop,derivop,evalatop>()
			),
	std::make_shared<collapsechain<E,binarychainop<addop>>>(),
	std::make_shared<collapsechain<E,binarychainop<mulop>>>(),
	std::make_shared<constchaineval<E,binarychainop<addop>>>(),
	std::make_shared<constchaineval<E,binarychainop<mulop>>>()
}};

#endif
