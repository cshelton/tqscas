#ifndef EXPRREWRITE_HPP
#define EXPRREWRITE_HPP

#include "exprtostr.hpp"
#include "exprsubst.hpp"
#include <iostream>

//#define SHOWRULES

struct rewriterule {
	virtual bool apply(expr &e) const { return false; }
};

struct binrewrite : public rewriterule {
	op matchop;

	binrewrite(op mop) : matchop(mop) {}

	virtual bool apply(expr &e) const {
		if (!e.isleaf() && e.asnode()==matchop) {
			auto &ch = e.children();
			if (valid(ch[0],ch[1])) {
				e = exec(ch[0],ch[1]);
				return true;
			}
		}
		return false;
	}

	virtual bool valid(const expr &e1, const expr &e2) const
		{ return false; }
	virtual expr exec(const expr &e1, const expr &e2) const
		{ return {matchop,e1,e2}; }
};

struct substrewrite : public rewriterule {
	std::shared_ptr<placeholdermatcher> m;
	expr pre,post;

	substrewrite(expr before, expr after,
			std::shared_ptr<placeholdermatcher> mm)
		: pre(std::move(before)), post(std::move(after)), m(mm) {}
	substrewrite(expr before, expr after) 
		: pre(std::move(before)), post(std::move(after)) {
			m = std::make_shared<placeholdermatcher();
	}

	virtual bool apply(expr &e) const {
		std::vector<subst> st;
		if (m->match(e,pre,st) && validsub(st)) {
			e = substitute(post,st);
			return true;
		}
		return false;
	}

	virtual bool validsub(const std::vector<subst> &st) const {
		return true;
	}
};

using ruleptr = std::shared_ptr<rewriterule>;

bool runrules(expr &e, const std::vector<ruleptr> &rules) {
	bool done=false;
	bool ret = false;
	//while(!done) {
	//	done = true;
		int i=0;
		for(auto &r : rules) {
			auto olde = e;
			if (r->apply(e)) {
#ifdef SHOWRULES
				std::cout << "===== (" << i << ")" << std::endl;
				std::cout << "changed " << std::endl;
				std::cout << draw(olde) << " to " << std::endl;
			    	std::cout << draw(e);
			    	std::cout << "-----------" << std::endl;
#endif
	//			done = false;
				ret = true;
				break;
			}
			++i;
		}
	//}
	return ret;
}

expr rewrite(expr e, const std::vector<ruleptr> &rules, bool &madechange) {
	madechange = false;
	bool cont = false;
	do {
		if (!e.isleaf()) {
			std::vector<expr> ch = e.children();
			bool changed = false;
			for(auto &c : ch) {
				bool mch = false;
				auto newc = rewrite(c,rules,mch);
				if (mch) {
					c = newc;
					changed = true;
					madechange = true;
				}
			}
			if (changed) e = expr(e.asnode(),ch);
		}
		cont = runrules(e,rules);
		madechange |= cont;
	} while(cont);
	return e;
}

expr rewrite(expr e, const std::vector<ruleptr> &rules) {
	bool c;
	return rewrite(std::move(e),rules,c);
}

#undef MYany_cast
#endif
