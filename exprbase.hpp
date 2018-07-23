#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include "typestuff.hpp"
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <cmath>

// a type to represent no expression (as a leaf in an expression tree)
struct noexprT {};

// constval holds a constant (as a leaf in an expression tree)
template<typename T>
struct constval {
	T v;
	template<typename S>
	constval(S &&val) : v{std::forward<S>(val)} {}
};

template<typename> struct isconstvaltype : public std::false_type { };
template<typename T> struct isconstvaltype<constval<T>> : public std::true_type { };
template<typename T>
inline constexpr bool isconstvaltype_v = isconstvaltype<T>::value;

// varinfo is the "name" of a variable (of type T)
template<typename T>
struct varinfo {
	const std::string name;

	varinfo(const std::string &n) : name(n) {}
	varinfo() : name(std::string("__")+std::to_string(++globalvarnum__)+
			typeid(T).name()) {}

	private:
	// TODO: perhaps replace with boost::uuid ??
	std::size_t globalvarnum__ = 0;
};

// var holds a variable (by shared_ptr to varinfo) of type T
// (as a leaf in an expression tree)
template<typename T>
struct var : public std::shared_ptr<varinfo<T>> {
	var(const varinfo<T> &vi) :
		std::shared_ptr<varinfo<T>>(std::make_shared<varinfo<T>>(vi)) {}
	var(varinfo<T> &&vi) :
		std::shared_ptr<varinfo<T>>(std::make_shared<varinfo<T>>(std::move(vi))) {}
};

template<typename> struct isvartype : public std::false_type { };
template<typename T> struct isvartype<var<T>> : public std::true_type { };
template<typename T>
inline constexpr bool isvartype_v = isvartype<T>::value;

template<typename ...Ts>
using leaf = std::variant<noexprT, constval<Ts>..., var<Ts>...>;

template<typename ...OPs>
using node = std::variant<std::monostate,OPs...>;

// a type to record T without using a value
// used so that variant<typetype<A>,typetype<B>,typetype<C>>
// can, at run time, keep track of a type (one of A, B, or C) without a value
template<typename T>
struct typetype {};

// An expression!  TV is a variant type containing all of the possible
template<typename TV, typename OPV>
struct expr : public gentree<repacknomonoadd_t<leaf,TV>,
						repacknomonoadd_t<node,OPV>> {
	using base = gentree<repacknomonoadd_t<leaf,TV>,
					repacknomonoadd_t<node,OPV>>;
	using base::base;

	using valuetype = repacknomonoadd_t<std::variant,TV,noexprT>;
	using valuetypetype = innerwrap_t<typetype,valuetype>;
	using optype = repack_t<std::variant,OPV>;
	using anyvartype = innerwrap_t<var,valuetype>;
};

template<typename E>
using exprmap = std::map<int,E>;
template<typename E>
using optexprmap = std::optional<exprmap<E>>;

template<typename T>
struct expraccess {};

template<typename E>
using exprvalue_t = typename E::valuetype;
template<typename E>
using exprvaluetype_t = typename E::valuetypetype;
template<typename E>
using exprop_t = typename E::optype;
template<typename E>
using expranyvar_t = typename E::anyvartype;

template<typename,typename>
struct exprunion{};

template<typename TV1, typename OPV1, typename TV2, typename OPV2>
struct exprunion<expr<TV1,OPV1>,expr<TV2,OPV2>> {
	using type = expr<variantunion_t<repack_t<std::variant,TV1>,
		 					repack_t<std::variant,TV2>>,
				variantunion_t<repack_t<std::variant,OPV1>,
							repack_t<std::variant,OPV2>>>;
};

template<typename E1, typename E2>
using exprunion_t = typename exprunion<E1,E2>::type;

template<typename T>
using expr1type_t = expr<std::variant<T>,std::variant<std::monostate>>;
template<typename OP>
using expr1op_t = expr<std::variant<std::monostate>,std::variant<OP>>;

template<typename VT, typename OP, typename ...Args>
VT evalopdispatch(const OP &o, Args &&...args) {
	return std::visit([&o](auto... params) -> VT {
			return evalop(o,std::forward<decltype(params)>(params)...);
		}, std::forward<Args>(args)...);
}

// assumes that the types are closed under the operations!
// (otherwise return type would be different)
template<typename E>
exprvalue_t<E> eval(const E &e) {
	using rett = exprvalue_t<E>;
	if (e.isleaf()) {
		return std::visit([](auto &&a) -> rett {
				if constexpr (isconstvaltype_v<std::decay_t<decltype(a)>>)
					return a.v;
				else return noexprT{};
			}
			, e.asleaf());
	} else {
		// support up to 4-arg hetrogeneous operators:
		const exprop_t<E> &op = e.asnode();
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

	template<typename Ts, typename OPs> struct hash<expr<Ts,OPs>> {
		typedef expr<Ts,OPs> argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return s.ptrhash();
		}
	};

	template<typename Ts, typename OPs, typename Ts2, typename OPs2>
	bool operator==(const expr<Ts,OPs> &e1, const expr<Ts2,OPs2> &e2) {
		return e1.sameptr(e2);
	}
	template<typename Ts, typename OPs, typename Ts2, typename OPs2>
	bool operator!=(const expr<Ts,OPs> &e1, const expr<Ts2,OPs2> &e2) {
		return !(e1==e2);
	}
}


template<typename T, typename E=expr1type_t<T>>
E newvar(const std::string &name) {
	return {var<T>{varinfo<T>(name)}};
}


template<typename T, typename E=expr1type_t<T>>
E newvar() {
	return {var<T>{varinfo<T>()}};
}

template<typename T, typename E=expr1type_t<T>>
E newconst(const T &v) {
	return {constval{v}};
}

template<typename E>
expranyvar_t<E> getvar(const E &e) {
	return std::visit([](auto &&a) -> expranyvar_t<E> {
			if constexpr (isvartype_v<std::decay_t<decltype(a)>>)
					return a;
				else return var<noexprT>{};
			}
			, e.asleaf());
}

template<typename OP, typename E>
bool isop(const E &e) {
	return !e.isleaf() && std::holds_alternative<OP>(e.asnode());
}

template<typename OP, typename E>
bool isderivop(const E &e) {
	return !e.isleaf() && isderivtype<OP>(e.asnode());
}

template<typename E, typename OP>
bool isop(const E &e, const OP &o) {
	return isop<OP>(e) && std::get<OP>(e.asnode())==o;
}

template<typename E>
bool isconst(const E &e) {
	return e.isleaf() && std::visit([](auto &&a) {
			return (isconstvaltype_v<std::decay_t<decltype(a)>>);
			}
		, e.asleaf());
}

template<typename E>
bool isvar(const E &e) {
	return e.isleaf() && std::visit([](auto &&a) {
			return (isvartype_v<std::decay_t<decltype(a)>>);
			}
		, e.asleaf());
}

template<typename E>
using vset_t = std::unordered_set<expranyvar_t<E>>;
template<typename E>
using vmap_t = std::unordered_map<expranyvar_t<E>,expranyvar_t<E>>;
//using vkmap = std::unordered_map<var,any>;

struct scopeop {}; // base class for any operator who first
			// argument is a local variable for the remainder

// do not call!
template<typename E>
bool isconstexpr1(const E &e,
		const std::optional<vset_t<E>> &vars,
		vset_t<E> &exceptvars) {
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
bool isconstexpr(const E &e, const std::optional<vset_t<E>> &vars = {}) {
	vset_t<E> ex;
	return isconstexpr1(e,vars,ex);
}

template<typename E>
bool isconstexpr(const E &e, const expranyvar_t<E> &v) {
    vset_t<E> ex;
    std::optional<vset_t<E>> vars{std::in_place,{v}};
    return isconstexpr1(e,vars,ex);
}

template<typename E>
bool isconstexprexcept(const E &e, const expranyvar_t<E> &v) {
    vset_t<E> ex{v};
    std::optional<vset_t<E>> vars{};
    return isconstexpr1(e,vars,ex);
}
	    

// do not call!
template<typename E>
bool isnonconstexpr1(const E &e,
		const std::optional<vset_t<E>> &vars,
		vset_t<E> &exceptvars) {
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
		const std::optional<vset_t<E>> &vars = {}) {
	vset_t<E> ex;
	return isnonconstexpr1(e,vars,ex);
}

#endif
