#ifndef EXPRREWRITE_HPP
#define EXPRREWRITE_HPP

//#include "exprtostr.hpp"
#include "exprmatch.hpp"
#include "exprsubst.hpp"
#include <iostream>
#include <memory>

//#define SHOWRULES
//#define SHOWCHANGES

struct rewriterule;

using ruleptr = std::shared_ptr<rewriterule>;


struct rewriterule {
	virtual std::optional<expr> apply(const expr &e) const { return {}; }
	virtual void setvars(const vset &v) {}
	virtual ruleptr clone() const { return std::make_shared<rewriterule>(*this); }
};



struct ruleset {

	ruleset(std::initializer_list<ruleptr> args) : rules(args) {}

	ruleset(ruleset &&rs) : rules(std::move(rs.rules)) {
	}

	ruleset(const ruleset &rs) {
		rules.reserve(rs.rules.size());
		for(auto &r : rs.rules)
			rules.emplace_back(r->clone());
	}
	ruleset(ruleset &rs) {
		rules.reserve(rs.rules.size());
		for(auto &r : rs.rules)
			rules.emplace_back(r->clone());
	}

	template<typename... T>
	ruleset(T &&...args) : rules(std::forward<T>(args)...) {}

	std::vector<ruleptr> rules;

	ruleset &operator+=(const ruleset &rs) {
		for(auto &r : rs.rules)
			rules.emplace_back(r->clone());
		return *this;
	}

	ruleset operator+(const ruleset &rs) const {
		ruleset ret(*this);
		return ret+=rs;
	}

	void setvar(const var &v) {
		setvars(vset{{v}});
	}

	void setvars(const vset &vars) {
		for(auto &r : rules) r->setvars(vars);
	}

	template<typename T>
	std::optional<expr> runrulesto(const expr &e, int n, T &cache) const {
		auto loc = cache.find(e);
		if (loc!=cache.end() && loc->second>=n) return {};
		if (n==0) return {};
		std::optional<expr> ret{};
		bool change=true;
		while(change) {
			change = false;
			auto newe = runrulesto(ret.value_or(e),n-1,cache);
			if (newe) { change=true; ret = newe; }
			if (!ret.value_or(e).isleaf()) {
				auto &ch = ret.value_or(e).children();
				std::vector<expr> newch;
				for(int i=0;i<ch.size();i++) {
					auto nc = runrulesto(ch[i],n,cache);
					if (nc) {
						if (newch.empty()) {
							newch.reserve(ch.size());
							for(int j=0;j<i;j++)
								newch.emplace_back(ch[j]);
						}
						newch.emplace_back(*nc);
					} else if (!newch.empty()) newch.emplace_back(ch[i]);
				}
				if (!newch.empty()) {
					ret = std::optional<expr>{in_place,ret.value_or(e).asnode(),newch};
					change = true;
				}
			}
			newe = rules[n-1]->apply(ret.value_or(e));
			if (newe) {
#ifdef SHOWRULES
				std::cout << "===== (" << n-1 << ")" << std::endl;
				std::cout << "changed " << std::endl;
				std::cout << draw(ret.value_or(e)) << " to " << std::endl;
				std::cout << draw(*newe);
				std::cout << "-----------" << std::endl;
#endif
				change=true;
				ret = newe;
			}
		}
		if (!ret) {
			if (loc!=cache.end()) loc->second = n;
			else cache.emplace(e,n);
		}
		return ret;
	}

	expr rewrite(const expr &e) const {
	// TODO:  add cache removal (dead expr and old ones to keep memory low)
	// (note that expr needs to be made into weak_ptr -- breaks
	//  abstraction -- and then multiple ptrs could be to the
	//  same location, so the map becomes a multimap with checking)
	// (currently moved to be inside rewrite which solves many problems)
		//mutable
		std::unordered_map<expr,int> cache;
		return runrulesto(e,rules.size(),cache).value_or(e);
	}
};


/*


std::optional<expr> runrules(const expr &e, const std::vector<ruleptr> &rules) {
	bool done=false;
	bool ret = false;
	std::optional<expr> rete;
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


std::optional<expr> rewritemaybe(const expr &e, const std::vector<ruleptr> &rules);

std::optional<expr> rewrite1(const expr &e, const std::vector<ruleptr> &rules) {
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

std::optional<expr> rewritemaybe(const expr &e, const std::vector<ruleptr> &rules) {
	std::optional<expr> rete = rewrite1(e,rules);
	if (!rete) return {};
	while(true) {
		auto newr = rewrite1(*rete,rules);
		if (!newr) {
#ifdef SHOWCHANGES
			std::cout << "complete change of " << std::endl;
			std::cout << "\t" << tostring(e) << std::endl;
			std::cout << "to" << std::endl;
			std::cout << "\t" << tostring(*rete) << std::endl;
#endif
			return rete;
		}
		rete = newr;
	}
}

template<typename F>
std::optional<expr> runcomplete(const expr &e, const std::vector<ruleptr> &rules, F f) {
	std::optional<expr> ret = f(e,rules);
	if (!ret) return ret;
	while(1) {
		auto newe = f(*ret,rules);
		if (!newe) return ret;
		ret = newe;
	}
}

std::optional<expr> runallrules(const expr &e, const std::vector<ruleptr> &rules) {
	return runcomplete(e,rules,runrules);
}

std::optional<expr> runtree(const expr &e, const std::vector<ruleptr> &rules) {
	auto ret = runallrules(e,rules);
	if (!ret) return {};
	return std::optional<expr>{in_place,
			ret->map([&rules](const expr &ex) { return runtree(ex,rules); })};
}

std::optional<expr> runalltree(const expr &e, const std::vector<ruleptr> &rules) {
	return runcomplete(e,rules,runtree);
}
*/

// perhaps should be written in terms of e.map???
/*
expr rewrite(const expr &e, const std::vector<ruleptr> &rules) {
	return rewritemaybe(e,rules).value_or(e);
}
*/
/*
 // does not work b/c rules not rerun on changed children
expr rewrite(const expr &e, const std::vector<ruleptr> &rules) {
	return e.map([&rules](const expr &ex) { return runallrules(ex,rules); });
}
*/
/*
expr rewrite(const expr &e, const std::vector<ruleptr> &rules) {
	return e.map([&rules](const expr &ex) { return runalltree(ex,rules); });
}
*/

struct optochain : public rewriterule {
	std::shared_ptr<opchain> cop;

	optochain(std::shared_ptr<opchain> chainop) : cop(chainop) {}

	virtual ruleptr clone() const { return std::make_shared<optochain>(*this); }

	virtual std::optional<expr> apply(const expr &e) const {
		if (isop(e,cop->baseop))
			return std::optional<expr>{in_place,cop,e.children()};
		return {};
	}
};

struct collapsechain : public rewriterule {
	op cop;
	bool assoc;

	collapsechain(op chainop, bool isassoc=true)
		: cop(chainop), assoc(isassoc) {}

	virtual ruleptr clone() const { return std::make_shared<collapsechain>(*this); }

	virtual std::optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		auto &n = e.asnode();
		if (n!=cop) return {};
		auto &ch = e.children();
		if (ch.size()==1)
			return std::optional<expr>{in_place,ch[0]};
		int nnewch = 0;
		bool cont=false;
		for(auto &c : ch) 
			if (isop(c,cop)) {
				cont = true;
				nnewch += c.children().size()-1;
			}
		if (!cont) return {};
		std::vector<expr> newch;
		newch.reserve(ch.size()+nnewch);
		for(auto &c : ch) {
			if (!isop(c,cop))
				newch.emplace_back(c);
			else {
				for(auto &c2 : c.children())
					newch.emplace_back(c2);
			}
		}
		return std::optional<expr>{in_place,cop,newch};
	}
};

struct matchrewrite : public rewriterule {
	expr search, replace;

	template<typename E1, typename E2>
	matchrewrite(E1 &&pattern, E2 &&newexp)
			: search(std::forward<E1>(pattern)),
			  replace(std::forward<E2>(newexp)) {}
	
	virtual ruleptr clone() const { return std::make_shared<matchrewrite>(*this); }

	virtual void setvars(const vset &v) {
		std::optional<expr> nsearch = search.mapmaybe([v](const expr &e) {
				if (!e.isleaf() ||
					e.asleaf().type()!=typeid(matchleaf)) return std::optional<expr>{};
				auto ml = MYany_cast<matchleaf>(e.asleaf());
				auto asconst = std::dynamic_pointer_cast<matchconstwrt>(ml);
				if (asconst)
					return std::optional<expr>{in_place,makematchleaf<matchconstwrt>(v)};
				auto asnonconst = std::dynamic_pointer_cast<matchnonconstwrt>(ml);
				if (asnonconst)
					return std::optional<expr>{in_place,makematchleaf<matchnonconstwrt>(v)};
				return std::optional<expr>{};
			});
		if (nsearch) search = *nsearch;
	}


	virtual std::optional<expr> apply(const expr &e) const {
		auto m = match(e,search);
		if (m) return substitute(replacelocal(replace),*m);
		else return {};
	}
};

template<typename F>
struct matchrewritecond : public matchrewrite {
	F condition;

	template<typename E1, typename E2>
	matchrewritecond(E1 &&pattern, E2 &&newexp, F c)
			: matchrewrite(std::forward<E1>(pattern),
					std::forward<E2>(newexp)),
				  condition(std::move(c)) {}

	virtual ruleptr clone() const { return std::make_shared<matchrewritecond>(*this); }

	virtual std::optional<expr> apply(const expr &e) const {
		auto m = match(e,search);
		if (m && condition(*m)) return substitute(replace,*m);
		else return {};
	}
};

template<typename E1, typename E2>
ruleptr SR(E1 &&pattern, E2 &&newexp) {
	return std::make_shared<matchrewrite>(std::forward<E1>(pattern),
								std::forward<E2>(newexp));
}

template<typename E1, typename E2, typename F>
ruleptr SR(E1 &&pattern, E2 &&newexp, F condition) {
	return std::make_shared<matchrewritecond<F>>(std::forward<E1>(pattern),
					std::forward<E2>(newexp), std::move(condition));
}

struct consteval : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<consteval>(*this); }
	virtual std::optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		if (!e.asnode()->caneval()) return {};
		if (isconstexpr(e)) return std::optional<expr>{in_place,
						newconst(e.asnode()->eval(e.children()))};
		return {};
	}
};

struct trivialconsteval : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<trivialconsteval>(*this); }
	virtual std::optional<expr> apply(const expr &e) const {
		if (e.isleaf()) return {};
		auto &ch = e.children();
		for(auto &c : ch) 
			if (!isconst(c)) return {};
		std::vector<any> subexpr;
		subexpr.reserve(ch.size());
		for(auto &c : ch)
			subexpr.emplace_back(MYany_cast<constval>(c.asleaf()).v);
		return std::optional<expr>{in_place,newconst(e.asnode()->opeval(subexpr))};
	}
};

struct constchaineval : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<constchaineval>(*this); }
	std::shared_ptr<opchain> cop;

	constchaineval(std::shared_ptr<opchain> chainop) : cop(chainop) {}

	virtual std::optional<expr> apply(const expr &e) const {
		if (!isop(e,cop)) return {};
		auto &ch = e.children();
		int ai=-1,bi=-1;
		for(int i=0;i<ch.size();i++)
			if (isconst(ch[i])) {
				if (ai>=0) {
					bi=i;
					break;
				} else ai = i;
			}
		if (bi==-1) return {};
		auto newch(ch);
		newch[ai] = newconst(cop->baseop->opeval(getconstany(ch[ai]),getconstany(ch[bi])));
		newch.erase(newch.begin()+bi);
		return std::optional<expr>{in_place,e.asnode(),newch};
	}
};


struct scopeeval : public rewriterule {
	virtual ruleptr clone() const { return std::make_shared<scopeeval>(*this); }
	virtual std::optional<expr> apply(const expr &e) const {
		if (!isop<scopeinfo>(e)) return {};
		auto se = std::dynamic_pointer_cast<scopeinfo>(e.asnode());
		auto &ch = e.children();
		//if (!se->caneval(ch)) {
			if (isop(e,evalatop))
				return substitute(ch[1],ch[0],ch[2]);
			else return {};
		//}
		//return std::optional<expr>{in_place,newconst(se->eval(ch))};
	}
};

#endif
