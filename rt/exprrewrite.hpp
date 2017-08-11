#ifndef EXPRREWRITE_HPP
#define EXPRREWRITE_HPP

#include "exprtostr.hpp"
#include "exprsubst.hpp"
#include "exprmatch.hpp"
#include <iostream>
#include <memory>

//#define SHOWRULES

struct rewriterule {
	virtual optional<expr> apply(const expr &e) const { return {}; }
};

using ruleptr = std::shared_ptr<rewriterule>;

optional<expr> runrules(const expr e, const std::vector<ruleptr> &rules) {
	bool done=false;
	bool ret = false;
	optional<expr> rete;
	int i=0;
	for(auto &r : rules) {
		if (auto newe = r->apply(rete.value_or(e))) {
#ifdef SHOWRULES
			std::cout << "===== (" << i << ")" << std::endl;
			std::cout << "changed " << std::endl;
			std::cout << draw(rete.value_or(e)) << " to " << std::endl;
			std::cout << draw(*newe);
			std::cout << "-----------" << std::endl;
#endif
			ret = true;
			rete = newe;
			break;
		}
		++i;
	}
	return rete;
}

optional<expr> rewritemaybe(const expr &e, const std::vector<ruleptr> &rules);

optional<expr> rewrite1(const expr &e, const std::vector<ruleptr> &rules) {
	if (!e.isleaf()) {
		std::vector<expr> ch = e.children();
		bool changed = false;
		for(auto &c : ch) {
			auto newc = rewritemaybe(c,rules);
			if (newc) {
				c = *newc;
				changed = true;
			}
		}
		if (changed) {
			expr rete = expr(e.asnode(),ch);
			auto res = runrules(rete,rules);
			if (res) return res;
			return {rete};
		}
	}
	return runrules(e,rules);
}

optional<expr> rewritemaybe(const expr &e, const std::vector<ruleptr> &rules) {
	optional<expr> rete = rewrite1(e,rules);
	if (!rete) return {};
	while(true) {
		auto newr = rewrite1(*rete,rules);
		if (!newr) return rete;
		rete = newr;
	}
}

// perhaps should be written in terms of e.map???
expr rewrite(const expr &e, const std::vector<ruleptr> &rules) {
	return rewritemaybe(e,rules).value_or(e);
}

struct optochain : public rewriterule {
	std::shared_ptr<opchain> cop;

	optochain(std::shared_ptr<opchain> chainop) : cop(chainop) {}

	virtual optional<expr> apply(const expr &e) const {
		if (!e.isleaf()) {
			auto &n = e.asnode();
			if (n==cop->baseop)
				return optional<expr>{in_place,
					std::static_pointer_cast<opinfo>(cop),e.children()};
		}
		return {};
	}
};

struct collapsechain : public rewriterule {
	op cop;
	bool assoc;

	collapsechain(op chainop, bool isassoc=true)
		: cop(chainop), assoc(isassoc) {}

	virtual optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		auto &n = e.asnode();
		if (n!=cop) return {};
		auto &ch = e.children();
		int nnewch = 0;
		bool cont=false;
		for(auto &c : ch) 
			if (!c.isleaf() && c.asnode()==cop) {
				cont = true;
				nnewch += c.children().size()-1;
			}
		if (!cont) return {};
		std::vector<expr> newch;
		newch.reserve(ch.size()+nnewch);
		for(auto &c : ch) {
			if (c.isleaf() || c.asnode()!=cop)
				newch.emplace_back(c);
			else {
				for(auto &c2 : c.children())
					newch.emplace_back(c2);
			}
		}
		return optional<expr>{in_place,cop,newch};
	}
};

struct matchrewrite : public rewriterule {
	expr search, replace;

	template<typename E1, typename E2>
	matchrewrite(E1 &&pattern, E2 &&newexp)
			: search(std::forward<E1>(pattern)),
			  replace(std::forward<E2>(newexp)) {}

	virtual optional<expr> apply(const expr &e) const {
		auto m = match(e,search);
		if (m) return substitute(replace,*m);
		else return {};
	}
};

template<typename E1, typename E2>
ruleptr SR(E1 &&pattern, E2 &&newexp) {
	return std::make_shared<matchrewrite>(std::forward<E1>(pattern),
								std::forward<E2>(newexp));
}

struct consteval : public rewriterule {
	virtual optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		auto &ch = e.children();
		for(auto &c : ch) 
			if (!isconst(c)) return {};
		std::vector<any> subexpr;
		subexpr.reserve(ch.size());
		for(auto &c : ch)
			subexpr.emplace_back(MYany_cast<constval>(c.asleaf()).v);
		return optional<expr>{in_place,newconst(e.asnode()->eval(subexpr))};
	}
};


#endif
