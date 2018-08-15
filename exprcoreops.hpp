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
	if constexpr (leftassoc) {
		std::string ret = write(BASEOP(),vpsi{subst[0],subst[1]});
		for(int i=2;i<subst.size();i++)
			ret = write(BASEOP(),vpsi{make_pair(ret,myprec),subst[i]});
		return ret;
	} else {
		int i=subst.size()-1;
		std::string ret = write(BASEOP(),vpsi{subst[i-1],subst[i]});
		for(i-=2;i>=0;i--)
			ret = write(BASEOP(),vpsi{subst[i],make_pair(ret,myprec)});
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
	VT ans = evalopwrap<VT>(BASEOP{},vs[0],vs[1]);
	for(std::size_t i=2;i<vs.size();++i)
			ans = evalopwrap<VT>(BASEOP{},ans,vs[i]);
	return ans;
}

template<typename VT, typename BASEOP>
VT evalopvec(const binarychainop<BASEOP,false> &,
			const std::vector<VT> &vs) {
	if (vs.size()<=1) return vs[0]; // if size==0, trouble!
	std::size_t i=vs.size();
	VT ans = evalopwrap<VT>(BASEOP{},vs[i-2],vs[i-1]);
	for(i-=2;i>0;--i)
		ans = evalopwrap<VT>(BASEOP{},vs[i-1],ans);
	return ans;
}

template<typename VT, typename BASEOP>
VT evaltypeopvec(const binarychainop<BASEOP,true> &,
			const std::vector<VT> &vs) {
	if (vs.size()<=1) return vs[0]; // if size==0, trouble!
	VT ans = evaloptypewrap<VT>(BASEOP{},vs[0],vs[1]);
	for(std::size_t i=2;i<vs.size();++i)
			ans = evaloptypewrap<VT>(BASEOP{},ans,vs[i]);
	return ans;
}

template<typename VT, typename BASEOP>
VT evaltypeopvec(const binarychainop<BASEOP,false> &,
			const std::vector<VT> &vs) {
	if (vs.size()<=1) return vs[0]; // if size==0, trouble!
	std::size_t i=vs.size();
	VT ans = evaloptypewrap<VT>(BASEOP{},vs[i-2],vs[i-1]);
	for(i-=2;i>0;--i)
		ans = evaloptypewrap<VT>(BASEOP{},vs[i-1],ans);
	return ans;
}


	
struct evalatop : public scopeop {};
	// 3 children: v, e, val
	// represents evaluating e where variable v (locally to e)
	// 			is replaced by val (which can be a full expr)
	// (v first b/c this is a "scopeop" and so it must be)

constexpr int precedence(const evalatop &) {
	return 3; // is this right?
}

std::string symbol(const evalatop &) {
	return "|_";
}

std::string write(const evalatop &o,
		const std::vector<std::pair<std::string,int>> &subst) {
	return putinparen(subst[1].first,subst[1].second>=precedence(o))
		+"|_{"+subst[0].first+"="+subst[2].first+"}";
}

template<typename E>
exprvalue_t<E> evalnode(const evalatop &, const std::vector<E> &ch) {
	auto &v = ch[0];
	auto valval = eval(ch[2]);
	if (!std::visit([](auto &&a, auto &&b) {
				using A = std::decay_t<decltype(a)>;
				using B = std::decay_t<decltype(b)>;
				return std::is_same_v<A,typetype<B>>; },evaltype(v),valval))
		return noexprT{};
	return eval(ch[1],v,valval);
}

template<typename E>
exprvaluetype_t<E> evaltypenode(const evalatop &, const std::vector<E> &ch) {
	return evaltype(ch[1]);
}


#endif
