#ifndef EXPRBASE_HPP
#define EXPRBASE_HPP

#include "gentree.hpp"
#include "typestuff.hpp"
#include "exprtypetraits.hpp"
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
	using type=T;
	T v;
	//template<typename S>
	//explicit constval(S &&val) : v{std::forward<S>(val)} {}

	constval(const T &vv) : v(vv) {}
	constval(T &&vv) : v(std::move(vv)) {}
	constval(const constval &) = default;
	constval(constval &&) = default;

	template<typename S>
	constexpr bool operator==(const constval<S> &x) const {
		if constexpr (haveeq_v<decltype(v),decltype(x.v)>) 
			return v==x.v;
		else return false;
	}

};

template<typename> struct isconstvaltype : public std::false_type { };
template<typename T> struct isconstvaltype<constval<T>> : public std::true_type { };
template<typename T>
inline constexpr bool isconstvaltype_v = isconstvaltype<T>::value;

// varinfo is the "name" of a variable (of type T)
template<typename T>
struct varinfo {
	using type=T;
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
	using type=T;
	var(const varinfo<T> &vi) :
		std::shared_ptr<varinfo<T>>(std::make_shared<varinfo<T>>(vi)) {}
	var(varinfo<T> &&vi) :
		std::shared_ptr<varinfo<T>>(std::make_shared<varinfo<T>>(std::move(vi))) {}

	template<typename S, 
		std::enable_if_t<!std::is_same_v<T,S>,int> = 0>
	bool operator==(const var<S> &v) const {
		return false;
	}

	bool operator==(const var<T> &v) const {
		return (std::shared_ptr<varinfo<T>>(v)) == 
			(std::shared_ptr<varinfo<T>>(*this));
	}

	std::string name() const { return this->get()->name; }
};

template<>
struct var<noexprT> {
	using type=noexprT;
	template<typename S>
	bool operator==(const var<S> &v) const { return false; }

	std::string name() const { return "N/A"; }
};

template<typename T>
inline constexpr bool isvartype_v = istmpl_v<var,T>;
template<typename T>
inline constexpr bool isconsttype_v = istmpl_v<constval,T>;
template<typename T>
inline constexpr bool isinvalid_v = std::is_same_v<T,noexprT> || std::is_same_v<T,noexprT>;

template<typename ...Ts>
struct exprleaf : public std::variant<noexprT, constval<Ts>..., var<Ts>...> {
	using base_t = std::variant<noexprT, constval<Ts>..., var<Ts>...>;
	using base_t::base_t;

	using values_t = std::variant<Ts...>; // Ts cannot be empty!

	base_t &asvariant() { return *this; }
	const base_t &asvariant() const { return *this; }
		
};
template<typename ...Ts, typename ...Rs>
constexpr bool operator==(const exprleaf<Ts...> &x,
					const exprleaf<Rs...> &y) {
	return varianteq(x.asvariant(),y.asvariant());
}

template<typename T, typename ...Ts>
constexpr std::enable_if_t<!istmpl_v<exprleaf,T>,bool>
operator==(const exprleaf<Ts...> &x, const T &y) {
	return std::visit([&y](auto &&a) -> bool {
			if constexpr (haveeq_v<std::decay_t<decltype(a)>,T>)
				return a==y;
			else return false;
		}, x.asvariant());
}

template<typename T, typename ...Ts>
constexpr std::enable_if_t<!istmpl_v<exprleaf,T>,bool>
operator==(const T &x, const exprleaf<Ts...> &y) {
	return std::visit([&x](auto &&a) -> bool {
			if constexpr (haveeq_v<std::decay_t<decltype(a)>,T>)
				return x==a;
			else return false;
		}, y.asvariant());
}

template<typename ...OPs>
struct exprnode : public std::variant<OPs...> {
	using base_t = std::variant<OPs...>;
	using base_t::base_t;

	using ops_t = std::variant<OPs...>; // OPs cannot be empty
	base_t &asvariant() { return *this; }
	const base_t &asvariant() const { return *this; }
};

template<typename ... OPs, typename ...OP2s>
constexpr bool operator==(const exprnode<OPs...> &x, const exprnode<OP2s...> &y) {
	return std::visit([](auto &&a, auto &&b) -> bool {
		if constexpr (!std::is_same_v<std::decay_t<decltype(a)>,
						std::decay_t<decltype(b)>>)
			return false;
		else if constexpr
			(haveeq_v<std::decay_t<decltype(a)>,std::decay_t<decltype(b)>>)
				return a==b;
		else return true;
		}, x.asvariant(), y.asvariant());
}

template<typename T, typename ... OPs>
constexpr std::enable_if_t<!istmpl_v<exprnode,T>,bool>
operator==(const exprnode<OPs...> &x, const T &y) {
	return std::visit([&y](auto &&a) -> bool {
			if constexpr (!std::is_same_v<std::decay_t<decltype(a)>,T>)
				return false;
			if constexpr (haveeq_v<std::decay_t<decltype(a)>,T>)
				return a==y;
			else return true;
		}, x.asvariant());
}

template<typename T, typename ... OPs>
constexpr std::enable_if_t<!istmpl_v<exprnode,T>,bool>
operator==(const T &x, const exprnode<OPs...> &y) {
	return std::visit([&x](auto &&a) -> bool {
			if constexpr (!std::is_same_v<std::decay_t<decltype(a)>,T>)
				return false;
			if constexpr (haveeq_v<std::decay_t<decltype(a)>,T>)
				return x==a;
			else return true;
		}, y.asvariant());
}

// a type to record the type T without using a value
// used so that variant<typetype<A>,typetype<B>,typetype<C>>
// can, at run time, keep track of which type (one of A, B, or C)
// without a value
template<typename T>
struct typetype {
	using type=T;

	template<typename S,
		std::enable_if_t<!std::is_same_v<typetype<T>,S>,int> = 0>
	bool operator==(const S &) const { return false; }
	bool operator==(const typetype<T> &) const { return true; }

	template<typename S>
	bool operator!=(const S &s) const { return !(*this==s); }
};

// An expression!  TV is a variant type containing all of the possible
//   types the expression (or subexpressions) could evaluate to
//   OPV is a variant type containing all of the possible operators
//   that could be used (internal nodes)
/*
template<typename TV, typename OPV>
using expr = gentree<repack_t<exprleaf,TV>, repack_t<exprnode,OPV>>;
*/

template<typename TV, typename OPV, typename TT=defaulttraits>
struct exprtraits {
	using LT = repack_t<exprleaf,TV>;
	using NT = repack_t<exprnode,OPV>;

	using Traits = TT;

	using values_t = typename LT::values_t;
	using valuestype_t = innerwrap_t<typetype,values_t>;
	using anyvar_t = innerwrap_t<var,values_t>;

	using ops_t = typename NT::ops_t;

	using leaftype = LT;
	using nodetype = NT;
};

template<typename TV, typename OPV, typename TT=defaulttraits>
using expr = gentree<repack_t<exprleaf,TV>, repack_t<exprnode,OPV>,
	 	exprtraits<TV,OPV,TT>>;

template<typename E>
using exprinfo = typename E::ExtraType;
template<typename E>
using traits = typename exprinfo<E>::Traits;

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
using exprvalue_t = typename exprinfo<E>::values_t;
template<typename E>
using exprvaluetype_t = typename exprinfo<E>::valuestype_t;
template<typename E>
using expranyvar_t = typename exprinfo<E>::anyvar_t;
template<typename E>
using exprleaf_t = typename exprinfo<E>::leaftype;
template<typename E>
using exprop_t = typename exprinfo<E>::ops_t;
template<typename E>
using exprnode_t = typename exprinfo<E>::nodetype;

template<typename T>
struct isexpr : public std::false_type { };
template<typename T1, typename T2, typename T3>
struct isexpr<expr<T1,T2,T3>> : public std::true_type { };
template<typename T>
inline constexpr bool isexpr_v = isexpr<T>::value;

// TODO: need to propagate exprtypetraits as well
template<typename...>
struct exprunion{
	using type = expr<std::variant<noexprT>,std::variant<noexprT>>;
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
using expr1type_t = expr<std::variant<noexprT,T>,std::variant<noexprT>>;
template<typename OP>
using expr1op_t = expr<std::variant<noexprT>,std::variant<noexprT,OP>>;

template<typename E, typename Eret>
E upgradeexpr(const Eret &e) {
	if constexpr (std::is_same_v<E,Eret>) return e;
	else {
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
}

template<typename OP, typename... Es>
auto buildexpr(const OP &node, Es &&...subexprs) {
	using rett = exprunion_t<expr1op_t<OP>,std::decay_t<Es>...>;
	return rett(node,upgradeexpr<rett>(subexprs)...);
}

template<typename OP, typename E>
auto buildexprvec(const OP &node, const std::vector<E> &ch) {
	using rett = exprunion_t<expr1op_t<OP>,E>;
	if constexpr (std::is_same_v<rett,E>)
		return rett(node,ch);
	else {
		std::vector<rett> rch;
		for(auto &c : ch) rch.emplace_back(upgradeexpr<rett>(c));
		return rett(node,std::move(rch));
	}
}
template<typename OP, typename E>
auto buildexprvec(const OP &node, std::vector<E> &&ch) {
	using rett = exprunion_t<expr1op_t<OP>,E>;
	if constexpr (std::is_same_v<rett,E>)
		return rett(node,std::move(ch));
	else {
		std::vector<rett> rch;
		for(auto &c : ch) rch.emplace_back(upgradeexpr<rett>(c));
		return rett(node,std::move(rch));
	}
}

// takes an op and the args (in raw type), returns the value of the op
// applied to the args (in raw type)
// ETT is the type that describes the type traits for the C++ types 
//   used (see exprtypetraits.hpp)
template<typename ETT>
noexprT evalop(...) {
	assert(false);
}

// takes an op and a vector of variants, returns the value in the
// variant type given as the second template argument
template<typename ETT, typename VT>
VT evalopvec(...) {
	assert(false);
}

template<typename ETT, typename VTT>
VTT evaltypeopvec(...) {
	assert(false);
}

template<typename ETT, typename RT, typename OP, typename... Args>
RT evalopwrap(const OP &o, Args&&...args) {
	return std::visit([&o](auto &&...as) -> RT {
		if constexpr ((... || (std::is_same_v<noexprT,
							std::decay_t<decltype(as)>>)))
			return noexprT{};
		else return evalop<ETT>(o,std::forward<decltype(as)>(as)...);
		}, std::forward<Args>(args)...);
}

template<typename ETT, typename RT, typename OP, typename... Args>
RT evaloptypewrap(const OP &o, Args&&...args) {
	return std::visit([&o](auto &&...as) -> RT {
		if constexpr ((... || (std::is_same_v<typetype<noexprT>,
							std::decay_t<decltype(as)>>)))
			return typetype<noexprT>{};
		else return
			typetype<
				decltype(evalop<ETT>(o,
						std::declval<typename std::decay_t<decltype(as)>::type>()...)
					)>{};

		}, std::forward<Args>(args)...);
}

// for an operator, this (evalnode) should be overloaded if
// the operator should *not* be evaulated by first evaluating the
// subtrees, and then calling evalop on the result
// (for instance for a "big-sum" operator)
//   [if you do so for an operator, you also need to overload evaltypenode
//    (below)]
// otherwise, just overload evalop to supply an answer given fully
// evaluated subtrees
template<typename OP, typename E>
exprvalue_t<E> evalnode(const OP &o, const std::vector<E> &ch) {
	using rett = exprvalue_t<E>;
	switch(ch.size()) {
		case 0: return evalopwrap<traits<E>,rett>(o);
		case 1: return evalopwrap<traits<E>,rett>(o,eval(ch[0]));
		case 2: return evalopwrap<traits<E>,rett>(o,eval(ch[0]),eval(ch[1]));
		case 3: return evalopwrap<traits<E>,rett>(o,eval(ch[0]),eval(ch[1]),eval(ch[2]));
		case 4: return evalopwrap<traits<E>,rett>(o,eval(ch[0]),eval(ch[1]),eval(ch[2]),eval(ch[3]));
		default: std::vector<rett> ceval;
			    for(auto &&c : ch) {
				    ceval.emplace_back(eval(c));
				    if (istype<noexprT>(ceval.back()))
					    return noexprT{};
			    }
			    return evalopvec<traits<E>,rett>(o,ceval);
	}
}

template<typename OP, typename E>
exprvaluetype_t<E> evaltypenode(const OP &o, const std::vector<E> &ch) {
	using rett = exprvaluetype_t<E>;
	switch(ch.size()) {
		case 0: return evaloptypewrap<traits<E>,rett>(o);
		case 1: return evaloptypewrap<traits<E>,rett>(o,evaltype(ch[0]));
		case 2: return evaloptypewrap<traits<E>,rett>(o,evaltype(ch[0]),evaltype(ch[1]));
		case 3: return evaloptypewrap<traits<E>,rett>(o,evaltype(ch[0]),evaltype(ch[1]),evaltype(ch[2]));
		case 4: return evaloptypewrap<traits<E>,rett>(o,evaltype(ch[0]),evaltype(ch[1]),evaltype(ch[2]),evaltype(ch[3]));
		default: std::vector<rett> ceval;
			    for(auto &&c : ch) {
				    ceval.emplace_back(evaltype(c));
				    if (istype<typetype<noexprT>>(ceval.back()))
					    return typetype<noexprT>{};
			    }
			    return evaltypeopvec<traits<E>,rett>(o,ceval);
	}
}


// assumes that the types are closed under the operations!
// (otherwise return type would be different)
template<typename E>
exprvalue_t<E> eval(const E &e) {
	using rett = exprvalue_t<E>;
	if (e.isleaf())
		return std::visit([](auto &&l) -> rett {
				using L = std::decay_t<decltype(l)>;
				if constexpr (isconstvaltype_v<L>)
					return l.v;
				else return noexprT{};
			}, e.asleaf().asvariant());
	else return std::visit([&e](auto &&n) -> rett {
				return evalnode(std::forward<decltype(n)>(n),
							e.children());
			}, e.asnode().asvariant());
}

template<typename TV, typename E=expr<TV,std::variant<noexprT>>>
E newconstfromeval(TV &&v);
template<typename T, typename E=expr1type_t<std::decay_t<T>>>
E newconst(T &&v);

template<typename E, typename TV>
E newconstfromeval2(TV &&v) { return newconstfromeval<TV,E>(std::forward<TV>(v));}
template<typename E, typename T>
E newconst2(T &&v) { return newconst<T,E>(std::forward<T>(v)); }

// eval under the substitution that v1 becomes v2 (v1 needs to be
// a leaf and v2 needs to be an exprvalue_t<E>)
// currently done via rewrite, but probably could be more efficient
// but then evalnode and the like would need to pass along the
// extra parameters (like the map of substitutions as these accumulate)
// TODO: make more efficient
template<typename E>
exprvalue_t<E> eval(const E &e, const E &v1, const exprvalue_t<E> &v2) {
	return eval(e,v1,newconstfromeval2<E>(v2));
}
template<typename E, typename V2,
	std::enable_if_t<!std::is_same_v<V2,E>
				&& !std::is_same_v<V2,exprvalue_t<E>>,int> =0>
exprvalue_t<E> eval(const E &e, const E &v1, const V2 &v2) {
	return eval(e,v1,newconst2<E>(v2));
}
template<typename E>
exprvalue_t<E> eval(const E &e, const E &v1, const E &v2) {
	return eval(e.map([&v1,&v2](const E &ex) {
				if (exprsame(ex,v1))
					return std::optional<E>{v2};
				else return std::optional<E>{};
			}));
}
	
// same as eval, but only calculates the type that would be returned
// (wrapped in a variant of typetypes -- see above)
// so that variables do not need to be instantiated
template<typename E>
exprvaluetype_t<E> evaltype(const E &e) {
	using rett = exprvaluetype_t<E>;
	if (e.isleaf())
		return std::visit([](auto &&l) -> rett {
				using L = std::decay_t<decltype(l)>;
				if constexpr (std::is_same_v<L,noexprT>)
					return typetype<noexprT>{};
				else return typetype<typename L::type>{};
			}, e.asleaf().asvariant());
	else return std::visit([&e](auto &&n) -> rett {
				return evaltypenode(std::forward<decltype(n)>(n),
							e.children());
			}, e.asnode().asvariant());
}

namespace std { 
	template<typename T> struct hash<var<T>> {
		typedef var<T> argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return std::hash<std::shared_ptr<varinfo<T>>>{}(s);
		}
	};

	template<> struct hash<var<noexprT>> {
		typedef var<noexprT> argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return 0;
		}
	};

	template<typename... Ts> struct hash<gentree<Ts...>> {
		typedef gentree<Ts...> argument_type;
		typedef std::size_t result_type;
		result_type operator()(argument_type const &s) const {
			return s.ptrhash();
		}
	};

	template<typename Ts, typename OPs, typename Trs, typename Ts2, typename OPs2, typename Trs2>
	bool operator==(const gentree<Ts,OPs,Trs> &e1, const gentree<Ts2,OPs2,Trs2> &e2) {
		return e1.sameptr(e2);
	}
	template<typename Ts, typename OPs, typename Trs, typename Ts2, typename OPs2, typename Trs2>
	bool operator!=(const gentree<Ts,OPs,Trs> &e1, const gentree<Ts2,OPs2,Trs2> &e2) {
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

template<typename E> // if v is not an expression consisting of a leaf
		// of just a variable, the behavior is *undefined*
E newvarsametype(const E &v) {
	return std::visit([](auto &&v) {
			return E{std::decay_t<decltype(v)>{}}; },
			v.asleaf().asvariant());
}

template<typename T, typename E=expr1type_t<std::decay_t<T>>>
E newconst(T &&v) {
	return E{constval<std::decay_t<T>>{std::forward<T>(v)}};
}

template<typename TV, typename E=expr<std::decay_t<TV>,std::variant<noexprT>>>
E newconstfromeval(TV &&v) {
	return std::visit([](auto &&v) { // decay below is bad if
							// expression tree is to hold
							// references or const, but that
							// seems unlikely use??
			return E{constval<std::decay_t<decltype(v)>>
						{std::forward<decltype(v)>(v)}}; },v);
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

struct scopeop {}; // base class for any operator who first
			// argument is a local variable for the remainder

template<typename E>
bool isscopeop(const E &e) {
	return !e.isleaf() && isderivtype<scopeop>(e.asnode());
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

template<typename... VTs>
using vset = std::unordered_set<std::variant<noexprT,var<VTs>...>>;
template<typename E>
using vset_t = std::unordered_set<expranyvar_t<E>>;
template<typename E1, typename E2=E1>
using vmap_t = std::unordered_map<expranyvar_t<E1>,expranyvar_t<E2>>;
//using vkmap = std::unordered_map<var,any>;


// do not call!
template<typename E, typename... VTs>
bool isconstexpr1(const E &e,
		const std::optional<vset<VTs...>> &vars,
		vset_t<E> &exceptvars) {
	if (e.isleaf()) {
		if (isconst(e)) return true;
		if (!isvar(e)) return false;
		auto v = getvar(e);
		return (vars && vars->find(v)==vars->end())
				|| exceptvars.find(v)!=exceptvars.end();
	} else {
		if (isscopeop(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.emplace(v);
		}
		for(auto &c : e.children())
			// no need to undo changes to exceptvars, because 
			// we will return false all the way back to the top
			if (!isconstexpr1(c,vars,exceptvars)) return false;
		if (isscopeop(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.erase(v);
		}
		return true;
	}
}

// vars == no value => "all vars" (different from an empty set)
template<typename E, typename... VTs>
bool isconstexpr(const E &e, const std::optional<vset<VTs...>> &vars = {}) {
	vset_t<E> ex;
	return isconstexpr1(e,vars,ex);
}

template<typename E, typename T>
bool isconstexpr(const E &e, const var<T> &v) {
	vset_t<E> ex;
	std::optional<vset<T>> vars{std::in_place,{v}};
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
		if (isscopeop(e)) {
			auto v = getvar(e.children()[0]);
			exceptvars.emplace(v);
		}
		for(auto &c : e.children())
			// no need to undo changes to exceptvars, because 
			// we will return true all the way back to the top
			if (isnonconstexpr1(c,vars,exceptvars)) return true;
		if (isscopeop(e)) {
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


template<typename E1, typename E2>
bool exprsame(const E1 &e1, const E2 &e2) {
	return e1.sameas(e2);
}

#endif
