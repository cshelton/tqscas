#ifndef BASICRULES_HPP
#define BASICRULES_HPP

#include "rewrite.hpp"

template<typename E>
ruleset baseruleset = {{
	std::make_shared<trivialconsteval<E>>(),
	std::make_shared<evalatrewrite<E>>(),
	createsortrewrite<E,negop,addop,binarychainop<addop>,subop,
				mulop,binarychainop<mulop>,divop,
				powop,logop,absop,derivop,evalatop>()
		->setreorderable<addop,binarychainop<addop>,
					mulop,binarychainop<mulop>>(),
	std::make_shared<collapsechain<E,binarychainop<addop>>(),
	std::make_shared<collapsechain<E,binarychainop<mulop>>(),
	std::make_shared<constchaineval<E,binarychainop<addop>>(),
	std::make_shared<constchaineval<E,binarychainop<mulop>>(),
}};

#endif
