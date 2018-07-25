#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include "typestuff.hpp"
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <cmath>
#include <cassert>

// a type to represent no (sub)expression (as a leaf in an expression tree)
// (often to represent an error or otherwise incomputable (sub)expression)
//struct noexprT {};
using noexprT = std::monostate;

// constval holds a constant (as a leaf in an expression tree)
template<typename T>
struct constval {
	T v;
	//template<typename S>
	//explicit constval(S &&val) : v{std::forward<S>(val)} {}

	constval(const T &vv) : v(vv) {}
	constval(T &&vv) : v(std::move(vv)) {}
	constval(const constval &) = default;
	constval(constval &&) = default;
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

template<typename T>
inline constexpr bool isvartype_v = istmpl_v<var,T>;
template<typename T>
inline constexpr bool isconsttype_v = istmpl_v<constval,T>;
template<typename T>
inline constexpr bool isinvalid_v = std::is_same_v<T,std::monostate> || std::is_same_v<T,noexprT>;

template<typename ...Ts>
struct exprleaf : public std::variant<noexprT, constval<Ts>..., var<Ts>...> {
	using base_t = std::variant<noexprT, constval<Ts>..., var<Ts>...>;
	using base_t::base_t;

	using values_t = std::variant<Ts...>; // Ts cannot be empty!

	base_t &asvariant() { return *this; }
	const base_t &asvariant() const { return *this; }
};

template<typename ...OPs>
struct exprnode : public std::variant<OPs...> {
	using base_t = std::variant<OPs...>;
	using base_t::base_t;

	using ops_t = std::variant<OPs...>; // OPs cannot be empty
	base_t &asvariant() { return *this; }
	const base_t &asvariant() const { return *this; }
};

// a type to record the type T without using a value
// used so that variant<typetype<A>,typetype<B>,typetype<C>>
// can, at run time, keep track of a type (one of A, B, or C) without a value
template<typename T>
struct typetype {};

// An expression!  TV is a variant type containing all of the possible
template<typename TV, typename OPV>
using expr = gentree<repack_t<exprleaf,TV>, repack_t<exprnode,OPV>>;

template<typename E, typename LF, typename NF>
auto exprfold(const E &e, LF lf, NF nf) {
	return e.fold([&lf](const auto &l)
					{ return std::visit(lf,l.asvariant());},
				[&nf](const auto &node, auto &&chval)
					{ return std::visit([&nf,&chval](auto &&n) {
							return nf(std::forward<decltype(n)>(n),
								std::forward<decltype(chval)>(chval));
							},node.asvariant());
					}
				);
}

template<typename E>
using exprmap = std::map<int,E>;
template<typename E>
using optexprmap = std::optional<exprmap<E>>;

template<typename T>
struct expraccess {};

template<typename LT, typename NT>
struct expraccess<gentree<LT,NT>> {
	using values_t = typename LT::values_t;
	using valuestype_t = innerwrap_t<typetype,values_t>;
	using anyvar_t = innerwrap_t<var,values_t>;

	using ops_t = typename NT::ops_t;

	using leaftype = LT;
	using nodetype = NT;
};


template<typename E>
using exprvalue_t = typename expraccess<E>::values_t;
template<typename E>
using exprvaluetype_t = typename expraccess<E>::valuestype_t;
template<typename E>
using expranyvar_t = typename expraccess<E>::anyvar_t;
template<typename E>
using exprleaf_t = typename expraccess<E>::leaftype;
template<typename E>
using exprop_t = typename expraccess<E>::ops_t;
template<typename E>
using exprnode_t = typename expraccess<E>::nodetype;

template<typename...>
struct exprunion{
	using type = expr<std::variant<std::monostate>,std::variant<std::monostate>>;
};

template<typename E>
struct exprunion<E> {
	using vtype = exprvalue_t<E>;
	using otype = exprop_t<E>;
	using type = E;
};

template<typename E, typename... Es>
struct exprunion<E,Es...> {
	using vtype = variantunion_t<exprvalue_t<E>,
		 		typename exprunion<Es...>::vtype>;
	using otype = variantunion_t<exprop_t<E>,
		 		typename exprunion<Es...>::otype>;
	using type = expr<vtype,otype>;
};

template<typename E, typename... Es>
using exprunion_t = typename exprunion<E,Es...>::type;

template<typename T>
using expr1type_t = expr<std::variant<T>,std::variant<std::monostate>>;
template<typename OP>
using expr1op_t = expr<std::variant<std::monostate>,std::variant<OP>>;

template<typename E, typename Eret>
E upgradeexpr(const Eret &e) {
	using ltype = exprleaf_t<E>;
	using ntype = exprnode_t<E>;
	return e.fold(
			[](auto &&l) { // convert leafs to ltype
				return E(upgradevariant<ltype>
							(std::forward<decltype(l)>(l).asvariant()));
			},
			[](auto &&n, auto &&ch) { // convert nodes to ntype
						// pass children on unchanged (already converted)
				return E(upgradevariant<ntype>
							(std::forward<decltype(n)>(n).asvariant()),
						std::forward<decltype(ch)>(ch));
			});
}

template<typename OP, typename... Es>
auto buildexpr(const OP &node, Es &&...subexprs) {
	using rett = exprunion_t<expr1op_t<OP>,std::decay_t<Es>...>;
	if constexpr ((... && (std::is_same_v<rett,Es>))) {
		return rett(node,std::forward<Es>(subexprs)...);
	} else { // must "upgrade" some/all of the subexprs
		return rett(node,upgradeexpr<rett>(subexprs)...);
	}
}

template<typename O, typename... Cs>
std::monostate evalop(const O &, Cs &&...) {
	assert(false);
}

template<typename VT, typename OPV, typename ...Args>
VT evalopdispatch(const OPV &ov, Args &&...args) {
	return std::visit([](auto &&o, auto &&...params) -> VT {
		if constexpr ((... || (std::is_same_v<std::monostate,
							std::decay_t<decltype(params)>>)))
			return std::monostate{};
		else return evalop(std::forward<decltype(o)>(o),
						std::forward<decltype(params)>(params)...);
		}, ov, std::forward<Args>(args)...);
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
			, e.asleaf().asvariant());
	} else {
		// support up to 4-arg hetrogeneous operators:
		auto &opv = e.asnode().asvariant();
		auto &ch = e.children();
		switch(ch.size()) {
			case 0: return evalopdispatch<rett>(opv);
			case 1: return evalopdispatch<rett>(opv,eval(ch[0]));
			case 2: return evalopdispatch<rett>(opv,
						   eval(ch[0]),
						   eval(ch[1]));
			case 3: return evalopdispatch<rett>(opv,
						   eval(ch[0]),
						   eval(ch[1]),
						   eval(ch[2]));
			case 4: return evalopdispatch<rett>(opv,
						   eval(ch[0]),
						   eval(ch[1]),
						   eval(ch[2]),
						   eval(ch[3]));
			default: {
				std::vector<rett> ceval; // don't do this generally
						// to avoid memory alloc
				for(auto &&c : ch)
					ceval.emplace_back(eval(c));
				return std::visit([&ceval](auto &&op) -> rett {
						return evalop(std::forward<decltype(op)>(op),
								ceval); },opv);
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

	template<typename... Ts> struct hash<gentree<Ts...>> {
		typedef gentree<Ts...> argument_type;
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
	return E{var<T>{varinfo<T>(name)}};
}


template<typename T, typename E=expr1type_t<T>>
E newvar() {
	return E{var<T>{varinfo<T>()}};
}

template<typename T, typename E=expr1type_t<T>>
E newconst(const T &v) {
	return E{constval{v}};
}

template<typename E>
expranyvar_t<E> getvar(const E &e) {
	return std::visit([](auto &&a) -> expranyvar_t<E> {
			if constexpr (isvartype_v<std::decay_t<decltype(a)>>)
					return a;
				else return var<noexprT>{};
			}
			, e.asleaf().asvariant());
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
		, e.asleaf().asvariant());
}

template<typename E>
bool isvar(const E &e) {
	return e.isleaf() && std::visit([](auto &&a) {
			return (isvartype_v<std::decay_t<decltype(a)>>);
			}
		, e.asleaf().asvariant());
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
