#ifndef SCALARINT_H
#define SCALARINT_H

#include "scalarbase.hpp"
#include "scalarrewrite.hpp"
#include <map>

struct subpattern {
	expr match, replace;

	struct kT {
		kT(int nn) : n(nn) {}
		int n;
	};

	struct xT {};

	static expr x = {xT{}};

	expr apply(const expr &e, const expr &x) const {
		std::map<int,expr> subs;
		if (mapit(e,match,x,subs)) return doit(replace,x,subs);
		return noexpr;
	}

	static bool mapit(const expr &e, const expr &m,
			const expr &x, std::map<int,expr> &subs) {
		if (m.isleaf()) {
			if (m.asleaf().type() == typeid(xT))
				return e.isleaf() && !isconswrt(e,x);
			if (m.asleaf().type() == typeid(kT)) {
				if (isconstwrt(e,x)) {
					int ki = MYany_cast<kT>(m.asleaf()).n;
					auto sube = subs.find(ki);
					if (sube==subs.end()) {
						subs.emplace_back(ki,e);
						return true;
					}
					return sube->second==e;
				} else return false;
			}
			return e==m;
		}
		if (m.asnode() != e.asnode()) return false;
		if (m.children().size() != e.children.size()) return false;
		for(int i=0;i<m.children.size();i++)
			if (!mapit(e.children()[i],m.children()[i],x,subs)) return false;
		return true;
	}

	static expr doit(const expr &r, const expr &x,
			const std::map<int,expr> &subs) {
		if (r.isleaf()) {
			if (r.asleaf().type() == typeid(xT))
				return x;
			if (r.asleaf().type() == typeid(kT)) {
				int ki = MYany_cast<kT>(r.asleaf()).n;
				auto sube = subs.find(ki);
				return sube->second;
			}
			return r;
		}
		std::vector<expr> newch;
		newch.reserve(r.children.size());
		for(auto &c : r.children())
			newch.emplace_back(c,x,subs);
		return {r.asnode(),newch};
	}
};


expr tableintegrate(const expr &e,const expr &x) {
	if (isconstwrt(e,x))
		return e*x;
	if (e.isleaf()) // should be just x then!
		return 0.5*x*x;
	if (e.asnode()==plusop || e.asnode()==pluschain) {
		std::vector<expr> newch;
		newch.reserve(e.children().size());
		for(auto &c : e.children()) {
			expr ce = tableintegrate(c,x);
			if (ce==noexpr) return noexpr;
			newch.emplace_back(std::move(ce));
		}
		return {pluschain,newch};
	}

			


}

expr integrate(const expr &eorig, const expr &x) {
	expr e = rewrite(eorig,scalarsimpwrt(x));
	return tableintegrate(e,x);
}

#endif
