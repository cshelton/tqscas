#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include "typestuff.hpp"
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <cmath>

struct noexprT {};

template<typename ...Ts>
struct constval {
	std::variant<Ts...> v;
	template<typename T>
	constval(T val) : v{std::move(val)} {}
};

struct varinfo {
	std::string name;
	const std::type_info &t;

	varinfo(const std::string &n, const std::type_info &ti) : name(n), t(ti) {}
};

struct var : public std::shared_ptr<varinfo> {
	var(const varinfo &vi) :
		std::shared_ptr<varinfo>(std::make_shared<varinfo>(vi)) {}
	var(varinfo &&vi) :
		std::shared_ptr<varinfo>(std::make_shared<varinfo>(std::move(vi))) {}
};

template<typename ...Ts> // place constval first so that its index (which
			// is hard to find via type) is always 0
using leaf = std::variant<constval<Ts...>,noexprT,var>;

template<typename ...Ts>
using node = std::variant<std::monostate,...Ts>;

// Ts is a variant or tuple that lists the possible constant types
// Ops is a variant or tuple that lists the possible operators
template<typename Ts, typename Ops>
using expr = gentree<unpackto_t<leaf,Ts>, unpackto_t<node,Ops>>;

template<typename T>
struct expraccess {};

template<typename Ts, typename Ops>
struct expraccess<expr<Ts,Ops> {
	using valuetype = unpackto_t<variant,Ts>;
	using optype = unpackto_t<variant,Ops>;
};

template<typename E>
using exprvalues = typename expraccess<E>::valuetype;
template<typename E>
using exprops = typename expraccess<E>::optype;

template<typename,typename
struct exprmerge{};

template<typename Ts1, typename Ops1, typename Ts2, typename Ops2>
struct exprmerge<expr<Ts1,Ops1>,expr<Ts2,Ops2>> {
	using type = expr<variantunion_t<unpackto_t<variant,Ts1>,
		 					unpackto_t<variant,Ts2>>,
				variantunion_t<unpackto_t<variant,Ops1>,
							unpackto_t<variant,Ops2>>>;
};

template<typename E1, typename E2>
using exprmerge_t = typename exprmerge<E1,E2>::type;

template<typename VT, typename OP, typename ...Args>
VT evalopdispatch(const OP &o, Args &&args) {
	return std::visitor([&o](auto... params) {
			return evalop(o,std::forward<decltype(params)>(params)...);
		}, std::forward<Args>(args)...);
}

// assumes that the types are closed under the operations!
template<E>
exprvalues<E> eval(const E &e) {
	using rett = exprvalues<E>;
	if (e.isleaf()) {
		if (isconst(e)) return std::get<2>(e.asleaf()).v;
		return {};
	} else {
		// support up to 4-arg hetrogeneous operators:
		const exprops &op = e.asnode();
		switch(e.children.size()) {
			case 0: return evalopdispatch(op);
			case 1: return evalopdispatch(op,eval(e.children[0]));
			case 2: return evalopdispatch(op,
						   eval(e.children[0]),
						   eval(e.children[1]));
			case 3: return evalopdispatch(op,
						   eval(e.children[0]),
						   eval(e.children[1]),
						   eval(e.children[2]));
			case 4: return evalopdispatch(op,
						   eval(e.children[0]),
						   eval(e.children[1]),
						   eval(e.children[2]),
						   eval(e.children[3]));
			default: {
				std::vector<rett> ceval; // don't do this generally
						// to avoid memory alloc
				for(auto &&c : e.children())
					ceval.emplace_back(eval(c));
				return std::visitor([&ceval](auto &&op) -> rett {
						return evalop(std::forward<decltype(op)>(op),
								ceval); },op);
		    }
		}
	}
}

namespace std { 
	template<> struct hash<var> {
		typedef var argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return std::hash<std::shared_ptr<varinfo>>{}(s);
		}
	};
	template<typename Ts, typename Ops> struct hash<expr<Ts,Ops>> {
		typedef expr argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return s.ptrhash();
		}
	};

	template<typename Ts, typename Ops, typename Ts2, typename Ops2>
	bool operator==(const expr<Ts,Ops> &e1, const expr<Ts2,Ops2> &e2) {
		return e1.sameptr(e2);
	}
	template<typename Ts, typename Ops, typename Ts2, typename Ops2>
	bool operator!=(const expr<Ts,Ops> &e1, const expr<Ts2,Ops2> &e2) {
		return !(e1==e2);
	}
}


template<typename E>
E newvar(const std::string &name, const std::type_info &ti) {
	return {var{varinfo(name,ti)}};
}

template<typename E, typename T>
E newvar(const std::string &name) {
	return {var{varinfo(name,typeid(T))}};
}

// TODO: remove global (somehow?)
std::size_t globalvarnum__ = 0;

template<typename E>
E newvar(const std::type_info &ti) {
	return newvar<E>(std::string("__")+std::to_string(++globalvarnum__),ti);
}

template<typename E, typename T>
E newvar() {
	return newvar<E>(typeid(T));
}


template<typename E, typename T>
E newconst(const T &v) {
	return {constval{v}};
}

template<typename E>
var getvar(const E &e) {
	return std::get<var>(e.asleaf());
}

template<typename E>
const std::type_info &getvartype(const E &e) {
	return getvar(e)->t;
}

template<typename T, typename E>
bool isop(const E &e) {
	return !e.isleaf() && std::holds_alternative<T>(e.asnode());
}

template<typename T, typename E>
bool isderivop(const E &e) {
	return !e.isleaf() && isderivtype<T>(e.asnode());
}

template<typename E, typename OP>
bool isop(const E &e, const OP &o) {
	return isop<OP>(e) && std::get<OP>(e.asnode())==o;
}

template<typename E>
bool isconst(const E &e) {
	return e.isleaf() && e.asleaf().index()==0;
}

bool isvar(const expr &e) {
	return e.isleaf() && std::holds_alternative<var>(e.asleaf());
}

using vset = std::unordered_set<var>;
using vmap = std::unordered_map<var,var>;
//using vkmap = std::unordered_map<var,any>;

struct scopeop {}; // base class for any operator who first
			// argument is a local variable for the remainder

// do not call!
template<typename E>
bool isconstexpr1(const E &e,
		const optional<vset> &vars,
		vset &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return true;
		if (!isvar(e)) return false;
		var v = getvar(e);
		return (vars && vars->find(v)==vars->end())
				|| exceptvars.find(v)!=exceptvars.end();
	} else {
		if (isderivop<scopeop>(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.emplace(v);
		}
		for(auto &c : e.children())
			// no need to undo changes to exceptvars, because 
			// we will return false all the way back to the top
			if (!isconstexpr1(c,vars,exceptvars)) return false;
		if (n) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return true;
	}
}

// vars == empty => "all vars"
template<typename E>
bool isconstexpr(const E &e, const optional<vset> &vars = {}) {
	vset ex;
	return isconstexpr1(e,vars,ex);
}

template<typename E>
bool isconstexpr(const E &e, const var &v) {
    vset ex;
    optional<vset> vars{in_place,{v}};
    return isconstexpr1(e,vars,ex);
}

template<typename E>
bool isconstexprexcept(const E &e, const var &v) {
    vset ex{v};
    optional<vset> vars{};
    return isconstexpr1(e,vars,ex);
}
	    

// do not call!
template<typename E>
bool isnonconstexpr1(const E &e,
		const optional<vset> &vars,
		vset &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return false;
		if (!isvar(e)) return false; // (not sure what to place here...
						// ... only reason this isn't !isconstexpr
		var v = getvar(e);
		return (!vars || vars->find(v)!=vars->end())
				&& exceptvars.find(v)==exceptvars.end();
	} else {
		if (isderivop<scopeop>(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.emplace(v);
		}
		for(auto &c : e.children())
			// no need to undo changes to exceptvars, because 
			// we will return true all the way back to the top
			if (isnonconstexpr1(c,vars,exceptvars)) return true;
		if (n) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return false;
	}
}

// vars == empty => "all vars"
template<typename E>
bool isnonconstexpr(const E &e,
		const optional<vset> &vars = {}) {
	vset ex;
	return isnonconstexpr1(e,vars,ex);
}


#endif
