#ifndef EXPRSUBST_HPP
#define EXPRSUBST_HPP

#include "exprbase.hpp"
#include <map>

expr substitute(const expr &e, const expr &x, const expr &v) {
	return e.map([x,v](const expr &ex) {
			if (ex==x) return optional<expr>{v};
			return optional<expr>{};
			});
}

// x -> v
template<typename T>
expr substitute(const expr &e, const expr &x, const T &v) {
	return substitute(e,x,newconst(v));
}

// x -> v
struct subst {
	subst(expr xx, expr vv) : x(std::move(xx)), v(std::move(vv)) {}
	expr x,v;
};

expr substitute(const expr &e, const subst &st) {
	return substitute(e,st.x,st.v);
}

expr substitute(const expr &e, const std::vector<subst> &st) {
	return e.map([st](const expr &ex) {
			for(auto &s : st)
				if (ex==s.x) return optional<expr>{s.v};
			return optional<expr>{};
			});
};

//------------------

struct placeholder {
	int num;
};

expr P(int i) { return expr{placeholder{i}}; }

expr P0_ = P(0);
expr P1_ = P(1);
expr P2_ = P(2);
expr P3_ = P(3);
expr P4_ = P(4);
expr P5_ = P(5);
expr P6_ = P(6);
expr P7_ = P(7);
expr P8_ = P(8);
expr P9_ = P(9);

expr substitute(const expr &e, const std::map<int,expr> st) {
	return e.map([st](const expr &ex) {
			if (ex.isleaf() && ex.asleaf().type()==typeid(placeholder)) {
				int n = MYany_cast<placeholder>(ex.asleaf()).num;
				auto l = st.find(n);
				if (l!=st.end()) return optional<expr>{l->second};
			}
			return optional<expr>{};
		});
}

#endif

