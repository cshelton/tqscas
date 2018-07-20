#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include "typestuff.hpp"
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <cmath>

struct noexprT {};

template<typename ...Ts>
struct constval {
	std::variant<Ts...> v;
	template<typename T>
	constval(T val) : v{std::move(val)} {}
};

template<typename T>
struct varinfo {
	const std::string name;

	varinfo(const std::string &n) : name(n) {}
};

template<typename T>
struct var : public std::shared_ptr<varinfo<T>> {
	var(const varinfo<T> &vi) :
		std::shared_ptr<varinfo<T>>(std::make_shared<varinfo<T>>(vi)) {}
	var(varinfo<T> &&vi) :
		std::shared_ptr<varinfo<T>>(std::make_shared<varinfo<T>>(std::move(vi))) {}
};

template<typename ...Ts>
using varvar = std::variant<var<Ts>...>;

template<typename E>
using anyvar = varvar<exprvalues<E>>;

template<typename ...Ts> // place constval first so that its index (which
			// is hard to find via type) is always 0
using leaf = std::variant<constval<Ts...>,noexprT,var<Ts...>>;

template<typename ...Ts>
using node = std::variant<std::monostate,Ts...>;

// Ts is a variant or tuple that lists the possible constant types
// Ops is a variant or tuple that lists the possible operators
/*
template<typename Ts, typename Ops>
using expr = gentree<unpackto_t<leaf,Ts>, unpackto_t<node,Ops>>;
*/

template<typename Ts, typename Ops>
struct expr : public gentree<unpackto_t<leaf,Ts>,unpackto_t<node,Ops>> {
	using valuetype = unpackto_t<std::variant,Ts>;
	using optype = unpackto_t<std::variant,Ops>;
};

template<typename E>
using exprmap = std::map<int,E>;
template<typename E>
using optexprmap = std::optional<exprmap<E>>;

template<typename T>
struct expraccess {};

/*
template<typename Ts, typename Ops>
struct expraccess<expr<Ts,Ops>> {
	using valuetype = unpackto_t<variant,Ts>;
	using optype = unpackto_t<variant,Ops>;
};

template<typename T1, typename T2> // assumes E is constructed as expr<X,Y>
struct expraccess<typename gentree<T1,T2>> {
	using valuetype = unpackto_t<std::variant,variantfirst_t<T1>>;
	using optype = unpackto_t<std::variant,variantfirst_t<T2>>;
};

template<typename E>
using exprvalues = typename expraccess<E>::valuetype;
template<typename E>
using exprops = typename expraccess<E>::optype;
*/

template<typename E>
using exprvalues = typename E::valuetype;
template<typename E>
using exprops = typename E::optype;

template<typename,typename>
struct exprunion{};

template<typename Ts1, typename Ops1, typename Ts2, typename Ops2>
struct exprunion<expr<Ts1,Ops1>,expr<Ts2,Ops2>> {
	using type = expr<variantunion_t<unpackto_t<std::variant,Ts1>,
		 					unpackto_t<std::variant,Ts2>>,
				variantunion_t<unpackto_t<std::variant,Ops1>,
							unpackto_t<std::variant,Ops2>>>;
};

template<typename E1, typename E2>
using exprunion_t = typename exprunion<E1,E2>::type;

template<typename VT, typename OP, typename ...Args>
VT evalopdispatch(const OP &o, Args &&...args) {
	return std::visit([&o](auto... params) -> VT {
			return evalop(o,std::forward<decltype(params)>(params)...);
		}, std::forward<Args>(args)...);
}

// assumes that the types are closed under the operations!
// (otherwise return type would be different)
template<typename E>
exprvalues<E> eval(const E &e) {
	using rett = exprvalues<E>;
	if (e.isleaf()) {
		if (isconst(e)) return std::get<0>(e.asleaf()).v;
		return {};
	} else {
		// support up to 4-arg hetrogeneous operators:
		const exprops<E> &op = e.asnode();
		switch(e.children.size()) {
			case 0: return evalopdispatch<rett>(op);
			case 1: return evalopdispatch<rett>(op,eval(e.children[0]));
			case 2: return evalopdispatch(op,
						   eval(e.children[0]),
						   eval(e.children[1]));
			case 3: return evalopdispatch<rett>(op,
						   eval(e.children[0]),
						   eval(e.children[1]),
						   eval(e.children[2]));
			case 4: return evalopdispatch<rett>(op,
						   eval(e.children[0]),
						   eval(e.children[1]),
						   eval(e.children[2]),
						   eval(e.children[3]));
			default: {
				std::vector<rett> ceval; // don't do this generally
						// to avoid memory alloc
				for(auto &&c : e.children())
					ceval.emplace_back(eval(c));
				return std::visit([&ceval](auto &&op) -> rett {
						return evalop(std::forward<decltype(op)>(op),
								ceval); },op);
		    }
		}
	}
}

namespace std { 
	template<typename T> struct hash<var<T>> {
		typedef var<T> argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return std::hash<std::shared_ptr<varinfo<T>>>{}(s);
		}
	};
	template<typename T> struct hash<anyvar<T>> {
		typedef anyvar<T> argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return std::visit(
						[](auto &&s2)
							{ return std::hash<decltype(s2)>{}(s2); },
						s);
		}
	};
	template<typename Ts, typename Ops> struct hash<expr<Ts,Ops>> {
		typedef expr<Ts,Ops> argument_type;
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


template<typename E, typename T>
E newvar(const std::string &name) {
	return {var<T>{varinfo<T>(name)}};
}

// TODO: remove global (somehow?)
std::size_t globalvarnum__ = 0;

template<typename E,typename T>
E newvar() {
	return newvar<E,T>(std::string("__")+std::to_string(++globalvarnum__));
}

template<typename E, typename T>
E newconst(const T &v) {
	return {constval{v}};
}

template<typename V>
struct getvarlambda {
	template<typename T>
	V operator()(const var<T> &v) {
		return v;
	}
	template<typename T>
	V operator()(const T &v) {
		return {};
	}
};

template<typename E>
anyvar<E> getvar(const E &e) {
	return std::visit(getvarlambda<anyvar<E>>{},e);
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

template<typename E>
bool isvar(const E &e) {
	return e.isleaf() && std::holds_alternative<var>(e.asleaf());
}

template<typename E>
using vset = std::unordered_set<anyvar<E>>;
template<typename E>
using vmap = std::unordered_map<anyvar<E>,anyvar<E>>;
//using vkmap = std::unordered_map<var,any>;

struct scopeop {}; // base class for any operator who first
			// argument is a local variable for the remainder

// do not call!
template<typename E>
bool isconstexpr1(const E &e,
		const std::optional<vset<E>> &vars,
		vset<E> &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return true;
		if (!isvar(e)) return false;
		auto v = getvar(e);
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
		if (isderivop<scopeop>(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return true;
	}
}

// vars == empty => "all vars"
template<typename E>
bool isconstexpr(const E &e, const std::optional<vset<E>> &vars = {}) {
	vset<E> ex;
	return isconstexpr1(e,vars,ex);
}

template<typename E>
bool isconstexpr(const E &e, const var &v) {
    vset<E> ex;
    std::optional<vset<E>> vars{std::in_place,{v}};
    return isconstexpr1(e,vars,ex);
}

template<typename E>
bool isconstexprexcept(const E &e, const var &v) {
    vset<E> ex{v};
    std::optional<vset<E>> vars{};
    return isconstexpr1(e,vars,ex);
}
	    

// do not call!
template<typename E>
bool isnonconstexpr1(const E &e,
		const std::optional<vset<E>> &vars,
		vset<E> &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return false;
		if (!isvar(e)) return false; // (not sure what to place here...
						// ... only reason this isn't !isconstexpr
		auto v = getvar(e);
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
		if (isderivop<scopeop>(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return false;
	}
}

// vars == empty => "all vars"
template<typename E>
bool isnonconstexpr(const E &e,
		const std::optional<vset<E>> &vars = {}) {
	vset<E> ex;
	return isnonconstexpr1(e,vars,ex);
}


#endif
