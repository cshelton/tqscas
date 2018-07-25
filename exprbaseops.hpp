#ifndef EXPRBASEOPS_HPP
#define EXPRBASEOPS_HPP

#include "exprbase.hpp"
#include "exprtostr.hpp"
#include <iostream>

/* An operator is a type with no state. 
 *
 * If OP is the type, it should support
 *
 * auto evalop(const OP &, args...)
 * 		should perform the op for whatever underlying types it allows
 * std::string symbol(const OP &)
 * 		returns a symbol to be used in a tree layout of an expr
 * int precedence(const OP &) 
 * 		returns precedence (smaller binds more tightly)
 * std::string write(const OP &,const std::vector<std::pair<std::string,int>> &)
 * 		returns a string (written representation) of the expression 
 * 		tree rooted at OP. The second argument is the string for each
 * 		child and its precedence
 * 		(precedence=0 implies a variable or constant)
 */


/*
 * One abstract supertype for op defined in exprbase.hpp: scopeop
 *
 * Two concrete op types defined here:  binarychainop and evalatop
 *
 * Anything else is up to the domain
 */

template<typename BASEOP, bool leftassoc=true>
struct binarychainop {};

template<typename BASEOP, bool leftassoc>
constexpr int precedence(binarychainop<BASEOP,leftassoc>) {
	return precedence(BASEOP());
}

template<typename BASEOP, bool leftassoc>
std::string symbol(binarychainop<BASEOP,leftassoc>) {
	return symbol(BASEOP())+std::string("...");
}

template<typename BASEOP, bool leftassoc>
std::string write(binarychainop<BASEOP,leftassoc>,
		const std::vector<std::pair<std::string,int>> &subst) {
	using vpsi = std::vector<std::pair<std::string,int>>;
	if (subst.size()<=2) return write(BASEOP(),subst);
	int myprec = precedence(BASEOP());
	if (leftassoc) {
		std::string ret = write(BASEOP(),vpsi{subst[0],subst[1]});
		for(int i=2;i<subst.size();i++)
			ret += write(BASEOP(),vpsi{make_pair(ret,myprec),subst[i]});
		return ret;
	} else {
		int i=subst.size()-1;
		std::string ret = write(BASEOP(),vpsi{subst[i-1],subst[i]});
		for(i-=2;i>=0;i--)
			ret += write(BASEOP(),vpsi{subst[i],make_pair(ret,myprec)});
		return ret;
	}
}

template<typename BASEOP, typename T, bool leftassoc>
auto evalop(const binarychainop<BASEOP,leftassoc> &, T &&v) {
	return v;
}

template<typename BASEOP, typename T1, typename T2, typename... Ts>
auto evalop(const binarychainop<BASEOP,true> &o, T1 &&v1, T2 &&v2, Ts &&...vs) {
	return evalop(o,evalop(BASEOP(),std::forward<T1>(v1),
								std::forward<T2>(v2)),
			std::forward<Ts>(vs)...);
}

template<typename BASEOP, typename T1, typename T2, typename... Ts>
auto evalop(const binarychainop<BASEOP,false> &o, T1 &&v1, T2 &&v2, Ts &&...vs) {
	return evalop(BASEOP(),
			std::forward<T1>(v1),
			evalop(o,std::forward<T2>(v2),std::forward<Ts>(vs)...));
}

template<typename VT, typename BASEOP>
VT evalopvec(const binarychainop<BASEOP,true> &,
			const std::vector<VT> &vs) {
	if (vs.size()<=1) return vs[0]; // if size==0, trouble!
	VT ans = evalopdispatchknownop<VT>(BASEOP{},vs[0],vs[1]);
	for(std::size_t i=2;i<vs.size();++i)
			ans = evalopdispatchknownop<VT>(BASEOP{},ans,vs[i]);
	return ans;
}

template<typename VT, typename BASEOP>
VT evalopvec(const binarychainop<BASEOP,false> &,
			const std::vector<VT> &vs) {
	if (vs.size()<=1) return vs[0]; // if size==0, trouble!
	std::size_t i=vs.size();
	VT ans = evalopdispatchknownop<VT>(BASEOP{},vs[i-2],vs[i-1]);
	for(i-=2;i>0;--i)
		ans = evalopdispatchknownop<VT>(BASEOP{},vs[i-1],ans);
	return ans;
}


	
struct evalatop : public scopeop {};
	// 3 children: v, e, val
	// represents evaluating e where variable v (locally to e)
	// 			is replaced by val (which can be a full expr)

constexpr int precedence(evalatop) {
	return 3;
}

std::string symbol(evalatop) {
	return "|_";
}

std::string write(evalatop,
		const std::vector<std::pair<std::string,int>> &subst) {
	if (subst[1].second>=3)
		return std::string("(")+subst[1].first+std::string(")")
			+"|_{"+subst[0].first+"="+subst[2].first+"}";
	else
		return subst[1].first
			+"|_{"+subst[0].first+"="+subst[2].first+"}";
	/*
	return putinparen(subst[1].first,subst[1].second>=3)
		+"|_{"+subst[0].first+"="+subst[2].first+"}";
	*/
}

template<typename E>
auto evalop(evalatop, const E &v, const E &expr, const E &val) {
    E valval{eval(val)};
    // could use substitute (from exprsubst.hpp), but that might
    // create circular dependencies (???)
    return eval(expr.map([v,valval](const E &ex) {
			    if (ex.sameas(v)) return std::optional<E>{valval};
			    return std::optional<E>{};
			    }));
}

template<typename E>
E evalat(const E &e, const E &localx, const E &x) {
	return {evalatop(),localx,e,x};
}



#endif
