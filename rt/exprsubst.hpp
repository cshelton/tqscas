#ifndef EXPRSUBST_HPP
#define EXPRSUBST_HPP

#include "exprbase.hpp"

// x -> v
expr substitute(const expr &e, const expr &x, const expr &v) {
	return e.fold([&x,&v](const leaf &l) {
				expr ret(l);
				return ret==x ? v : ret;
			},
			[&x,&v](const op &o, const std::vector<expr> &ch) {
				expr ret{o,ch};
				return ret==x ? v : ret;
			});
}

// x -> v
template<typename T>
expr substitute(const expr &e, const expr &x, const T &v) {
	return subst(e,x,newconst(v));
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
	return e.fold([&st](const leaf &l) {
				expr ret(l);
				for(auto &s : st) if (ret==st.x) return st.v;
				return ret;
			},
			[&x,&v](const op &o, const std::vector<expr> &ch) {
				expr ret{o,ch};
				for(auto &s : st) if (ret==st.x) return st.v;
				return ret;
			});
};

struct placeholdermatcher {
	// does template t match expression e? (return in st)
	bool match(const expr &e, const expr &t, std::vector<subst> &st) const {
		std::map<placeholderT,expr> subs;
		if (match(e,t,subs)) {
			for(auto &s : subs)
				st.emplace_back(s.first,s.second);
			return true;
		}
		return false;
	}

	virtual bool matchleaf(const expr &e, const expr &r,
				std::map<placeholderT,expr> &st) const {
		return r==e;
	}

	virtual bool matchop(const expr &e, const expr &r,
				std::map<placeholderT,expr> &st) const {
		if (e.isleaf() || e.asnode()!=r.asnode()) return false;
		auto &ce = e.children(), re = r.children();
		if (ce.size()!=re.size()) return false;
		for(int i=0;i<ce.size();i++)
			if (!match(ce[i],re[i],st)) return false;
		return true;
	}

	bool match(const expr &e, const expr &r,
				std::map<placeholderT,expr> &st) const {
		if (r.isleaf()) {
			if (r.asleaf().type() == typeid(placeholderT)) {
				auto p = MYany_cast<placeholderT>(r.asleaf());
				auto l = st.find(p);
				if (l == st.end()) {
					if (p.m->matches(e)) {
						st.emplace_back(p,e);
						return true;
					}
					return false;
				}
				return e==p.second;
			}
			return matchleaf(e,r,st);
		} 
		return matchop(e,r,st);
	}
};

struct constmatcher : public matcher {
	expr x;
	constmatcher(expr wrt = allvars) : x(std::move(wrt)) {}

	virtual bool matches(const expr &e) const {
		return isconstwrt(e,x);
	}
};

struct exprmatcher : public matcher {
	virtual bool matches(const expr &e) const {
		return true;
	}
};

expr genericconst(int i, expr wrt=allvars) {
	return {placeholderT{i,new constmatcher(wrt)}};
}

expr genericexpr(int i) {
	return {placeholderT{i,new exprmatcher()}};
}

expr k1 = genericconst(1);
expr k2 = genericconst(2);
expr k3 = genericconst(3);

expr e1 = genericexpr(1);
expr e2 = genericexpr(2);
expr e3 = genericexpr(3);

expr convertconstwrt(const expr &e, const expr &wrt) {
	return e.fold([wrt](const leaf &l) {
			if (l.asleaf().type()==typeid(placeholderT)) {
				auto p = MYany_cast<placeholderT>(l.asleaf());
				auto mm = std::dynamic_pointer_cast_cast<constmatcher>(p);
				if (mm) return genericconst(p.n,wrt);
			}
			return expr{l};
			},
			[wrt](const op &o, const std::vector<expr> &ch) {
				return expr{o,ch};
			});
}

#endif
