#ifndef SCALARDERIV_H
#define SCALARDERIV_H

#include "scalarbase.hpp"
#include "scalarrewrite.hpp"

expr derivreal(const expr &e, const expr &x) {
	if (e.isleaf()) {
		if (isconstwrt(e,x)) return 0.0;
		else return 1.0;
	}
	auto op = e.asnode();
	if (op==plusop || op==pluschain) {
		std::vector<expr> dch;
		dch.reserve(e.children().size());
		for(auto &c : e.children())
			dch.emplace_back(derivreal(c,x));
		return {op,dch};
	}
	if (op==multipliesop || op == multiplieschain) {
		std::vector<expr> dch;
		dch.reserve(e.children().size());
		for(int i=0;i<e.children().size();i++) {
			std::vector<expr> mch = e.children();
			mch[i] = derivreal(mch[i],x);
			dch.emplace_back(multiplieschain,mch);
		}
		return {pluschain,dch};
	}
	if (op==powerop) {
		auto f = e.children()[0];
		auto g = e.children()[1];
		auto df = derivreal(f,x);
		auto dg = derivreal(g,x);
		return e*log(f)*dg + g*pow(f,g-1)*df;
	}
	if (op==logop) {
		auto f = e.children()[0];
		auto df = derivreal(f,x);
		return df/f;
	}
}

expr deriv(const expr &e, const expr &x) {
	if (!x.isleaf() || x.asleaf().type() != typeid(var))
		return e; // error out! [TODO]
	std::vector<expr> xx{x};
	return rewrite(derivreal(rewrite(e,scalarsimpwrt(xx)),x),scalarsimprules);
}

#endif
