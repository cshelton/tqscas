#ifndef REWRITE_HPP
#define REWRITE_HPP

//#include "exprtostr.hpp"
#include "match.hpp"
#include "subst.hpp"
#include <iostream>
#include <memory>

//#define SHOWRULES
//#define SHOWCHANGES

template<typename E>
struct rewriterule;

template<typename E>
using ruleptr = std::shared_ptr<rewriterule<E>>;

template<typename E>
struct rewriterule {
	virtual std::optional<E> apply(const E &e) const { return {}; }
	virtual ruleptr clone() const { return std::make_shared<rewriterule<E>>(*this); }
};

template<typename E>
struct ruleset {

	ruleset(std::initializer_list<ruleptr<E>> args) : rules(args) {}

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

	std::vector<ruleptr<E>> rules;

	ruleset &operator+=(const ruleset<E> &rs) {
		for(auto &r : rs.rules)
			rules.emplace_back(r->clone());
		return *this;
	}

	ruleset operator+(const ruleset<E> &rs) const {
		ruleset ret(*this);
		return ret+=rs;
	}

	template<typename T>
	std::optional<E> runrulesto(const E &e, int n, T &cache) const {
		auto loc = cache.find(e);
		if (loc!=cache.end() && loc->second>=n) return {};
		if (n==0) return {};
		std::optional<E> ret{};
		bool change=true;
		while(change) {
			change = false;
			auto newe = runrulesto(ret.value_or(e),n-1,cache);
			if (newe) { change=true; ret = newe; }
			if (!ret.value_or(e).isleaf()) {
				auto &ch = ret.value_or(e).children();
				std::vector<E> newch;
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
					ret = std::optional<E>{std::in_place,ret.value_or(e).asnode(),newch};
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

	E rewrite(const E &e) const {
	// TODO:  add cache removal (dead expr and old ones to keep memory low)
	// (note that expr needs to be made into weak_ptr -- breaks
	//  abstraction -- and then multiple ptrs could be to the
	//  same location, so the map becomes a multimap with checking)
	// (currently moved to be inside rewrite which solves many problems)
		//mutable
		std::unordered_map<E,int> cache;
		return runrulesto(e,rules.size(),cache).value_or(e);
	}
};


// COP = chain op (type)
template<typename E, typename COP>
struct optochain : public rewriterule<E> {
	virtual ruleptr clone() const {
		return std::make_shared<optochain<E,COP>>(*this);
	}
	virtual std::optional<E> apply(const E &e) const {
		if (isop<typename COP::baseopT>(e))
			return std::optional<E>{std::in_place,COP{},e.children()};
		return {};
	}
};

template<typename E, typename COP>
struct collapsechain : public rewriterule<E> {
	virtual ruleptr clone() const { return std::make_shared<collapsechain<E,COP>>(*this); }

	virtual std::optional<E> apply(const E &e) const {
		if (!isop<COP>(e)) return {};
		auto &ch = e.children();
		if (ch.size()==1)
			return std::optional<E>{std::in_place,ch[0]};
		int nnewch = 0;
		bool cont=false;
		for(auto &c : ch) 
			if (isop<COP>(c)) {
				cont = true;
				nnewch += c.children().size()-1;
			}
		if (!cont) return {};
		std::vector<E> newch;
		newch.reserve(ch.size()+nnewch);
		for(auto &c : ch) {
			if (!isop<COP>(c))
				newch.emplace_back(c);
			else {
				for(auto &c2 : c.children())
					newch.emplace_back(c2);
			}
		}
		return std::optional<E>{std::in_place,COP{},newch};
	}
};

template<typename E>
struct matchrewrite : public rewriterule<E> {
	E search, replace;

	template<typename E1, typename E2>
	matchrewrite(E1 &&pattern, E2 &&newexp)
			: search(upgradeexpr<E>(std::forward<E1>(pattern))),
			  replace(upgradeexpr<E>(std::forward<E2>(newexp))) {}
	
	virtual ruleptr clone() const { return std::make_shared<matchrewrite<E>>(*this); }

	virtual std::optional<E> apply(const E &e) const {
		auto m = match(e,search);
		if (m) return substitute(replacelocal(replace),*m);
		else return {};
	}
};

template<typename E, typename F>
struct matchrewritecond : public matchrewrite<E> {
	F condition;

	template<typename E1, typename E2>
	matchrewritecond(E1 &&pattern, E2 &&newexp, F c)
			: matchrewrite(std::forward<E1>(pattern),
					std::forward<E2>(newexp)),
				  condition(std::move(c)) {}

	virtual ruleptr clone() const { return std::make_shared<matchrewritecond<E,F>>(*this); }

	virtual std::optional<E> apply(const E &e) const {
		auto m = match(e,search);
		if (m && condition(*m)) return substitute(replace,*m);
		else return {};
	}
};

template<typename E>
ruleptr SR(const E &pattern, const E &newexp) {
	return std::make_shared<matchrewrite<E>>(pattern,newexp);
}

template<typename E, typename F>
ruleptr SR(const E &pattern, const E &newexp, F condition) {
	return std::make_shared<matchrewritecond<E,F>>(pattern,newexp,
			std::move(condition));
}

template<typename E>
struct consteval : public rewriterule<E> {
	virtual ruleptr clone() const { return std::make_shared<consteval<E>>(*this); }
	virtual std::optional<E> apply(const E &e) const {
		if (e.isleaf()) return {};
		if (!e.asnode()->caneval()) return {};
		if (isconstexpr(e))
			return std::optional<E>{std::in_place,
					newconstfromeval2<E>(eval(e))};
		return {};
	}
};

template<typename E>
struct trivialconsteval : public rewriterule<E> {
	virtual ruleptr clone() const { return std::make_shared<trivialconsteval<E>>(*this); }
	virtual std::optional<E> apply(const E &e) const {
		if (e.isleaf()) return {};
		auto &ch = e.children();
		for(auto &c : ch) 
			if (!isconst(c)) return {};
		return std::optional<E>{std::in_place,
					newconstfromeval2<E>(eval(e))};
	}
};

template<typename E,typename COP>
struct constchaineval : public rewriterule<E> {
	virtual ruleptr clone() const { return std::make_shared<constchaineval<E,COP>>(*this); }

	virtual std::optional<E> apply(const E &e) const {
		if (!isop<COP>(e)) return {};
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
		newch[ai] = newconstfromeval2<E>
				(evalnode((typename COP::baseopT){},ch[ai],ch[bi]));
		newch.erase(newch.begin()+bi);
		return std::optional<expr>{std::in_place,e.asnode(),newch};
	}
};


template<typename E>
struct evalatrewrite : public rewriterule<E> {
	virtual ruleptr clone() const { return std::make_shared<scopeeval<E>>(*this); }
	virtual std::optional<expr> apply(const expr &e) const {
		if (!isscopeop(e)) return {};
		auto se = std::dynamic_pointer_cast<scopeinfo>(e.asnode());
		auto &ch = e.children();
		if (isop<evalatop>(e))
			return substitute(ch[1],ch[0],ch[2]);
		else return {};
	}
};

template<typename E>
struct sortchildren : public rewriterule<E> {
	virtual ruleptr clone() const { return std::make_shared<sortchildren<E>>(*this); }
	optional<vset_t<E>> vars;
	// maps type to ordering (lower goes first) and whether the
	// children can be reordered
	using typemap = std::unordered_map<std::type_info,std::pair<int,bool>>;
	typemap ord;
	int maxord;

	sortchildren(typemap ordering)
		: ord(std::move(ordering)), vars{} { calcmaxord(); }
	sortchildren(typemap ordering, vset_t<E> v)
		: cops(std::move(ordering)), vars(in_place,std::move(v))
		{ calcmaxord(); }

	private:
	void calcmaxord() {
		maxord = 0;
		int minord = 0;
		for(auto [tid,attr] : ord) {
			minord = std::min(attr.first,minord);
			maxord = std::max(attr.first,maxord);
		}
		if (minord<0) {
			for(auto &[tid,attr] : ord)
				attr.first -= minord;
			maxord -= minord;
		}
	}

	public:

	int typeorder(const E &e) const {
		if (isconst(e)) return 0;
		int add = (isconstexpr(e,vars) ? 2 : 4+maxord);
		if (e.isleaf()) return 1+add;
		return std::visit([&ord,add](auto &&o) {
				using O = std::decay_t<decltype(o)>;
				auto tid = typeid(O);
				auto p = ord.find(tid);
				if (p==ord.end()) return maxord+1+add;
				else return p->second.first+add;
			}, e.asnode().asvariant());
	}

	void setvars(const vset &v) {
		vars = v;
	}

	template<typename... OPs>
	void setreorderable() {
		for(auto &[tid,attr] : ord)
			attr.second = isidmem<OPs...>(tid);
	}

	int secondordering(const E &e1, const E &e2) const {
		if (isconst(e1)) { // would like to be able to sort
					// however, the types might not allow this
			/* old code for old interface to expr
			scalarreal v1 = getconst<scalarreal>(e1);
			scalarreal v2 = getconst<scalarreal>(e2);
			if (v1<v2) return -1;
			if (v1>v2) return +1;
			*/
			return 0;
		}
		if (isvar(e1))
			return std::visit([](auto &&v1, auto &&v2) {
					using V1 = std::decay_t<decltype(v1)>;
					using V2 = std::decay_t<decltype(v2)>;
					if constexpr (istmpl_v<var,V1> && istmpl_v<var,V2>)
						return v1.name().compare(v2.name());
					else return 0; // should never get here
				}, e1.asnode().asvariant(), e2.asnode().asvariant());
		if (e1.isleaf()) return 0;
		auto &ch1 = e1.children();
		auto &ch2 = e2.children();
		for(int i=std::min(ch1.size(),ch2.size())-1;i>=0;i--) {
			int res = exprcmp(ch1[i],ch2[i]);
			if (res!=0) return res;
		}
		return ch1.size()-ch2.size();
	}

	int exprcmp(const E &e1, const E &e2) const {
		int o1 = typeorder(e1), o2 = typeorder(e2);
		if (o1!=o2) return o1-o2;
		return secondordering(e1,e2);
	}

	virtual optional<E> apply(const E &e) const {
		if (e.isleaf()) return {};
		auto &ch = e.children();
		if (ch.size()<2) return {};
		auto cmper = [this](const E &e1, const E &e2) {
			return exprcmp(e1,e2)<0; };
		return std::visit([&cmper,&ch](auto &&o) -> optional<E> {
				using O = std::decay_t<decltype(o)>;
				auto tid = typeid(O);
				auto p = ord.find(tid);
				if (p==ord.end() || !p->second.second) return {}
				for(int i=1;i<ch.size();i++) {
					if (exprcmp(ch[i-1],ch[i])>0) {
						std::vector<E> che = ch;
						std::sort(che.begin(),che.end(),cmper);
						return optional<E>{in_place,
								buildexprvec(o,std::move(che))};
					}
				}
				return {};
			}, e.asnode().asvariant());
	}
};

template<typename, typename...>
struct maketm;

template<typename E>
struct maketm {
	static typename sortchildren<E>::typemap exec(int) {
		return {};
	}
};

template<typename E, typename T, typename... Ts>
struct maketm {
	static typename sortchildren<E>::typemap exec(int i) {
		auto tm = maketm<E,Ts...>::exec(i+1);
		tm[typeid(T)] = std::make_pair(i,false);
		return rm;
	}
};


template<typename E, typename... OPS>
ruleptr createsortrewrite() {
	auto tm = maketm<E,OPS...>::exec(0);
	return std::make_shared<sortchildren<E>>(tm);
}

#endif
