#ifndef VARSET_H
#define VARSET_H

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <string.h>

template<typename T>
struct var {
	constexpr var(const char *n) : name(n) {}
	const char *name;
};


template<typename T1, typename T2, typename E=void>
struct cov_help
{
	typedef void type;
};

void voidfn() {}

template<typename T1, typename T2>
struct cov_help<T1,T2,decltype(true ? std::declval<T1>() : std::declval<T2>(),voidfn())> {
	typedef decltype(true ? std::declval<T1>() : std::declval<T2>()) type;
};

template<typename ...> struct common_or_void;

template<>
struct common_or_void<> {
	typedef void type;
};

template<typename T>
struct common_or_void<T> {
	typedef T type;
};


template<typename T1, typename T2>
struct common_or_void<T1,T2> {
	typedef typename cov_help<T1,T2>::type type;
};

template<typename T1, typename T2, typename... Ts>
struct common_or_void<T1,T2,Ts...> {
	typedef typename common_or_void<typename common_or_void<T1,T2>::type,Ts...>::type type;
};

template<typename... Ts> struct varset {};

template<typename T, typename... Ts>
struct varset<T,Ts...> {
	template<typename NT, typename... NTS>
	constexpr varset(NT &&n, NTS &&... ns)
		: vname(std::forward<NT>(n)), vnames(std::forward<NTS>(ns)...) {}

	var<T> vname;
	varset<Ts...> vnames;

	typedef typename common_or_void<T,Ts...>::type commontype;
};


template<typename RETT, typename V>
struct deduce_ret {
	typedef typename std::conditional<std::is_same<RETT,void>::value,typename V::commontype,RETT>::type type;
};


template<typename RETT=void, typename T, typename S>
constexpr typename std::enable_if<std::is_convertible<T,typename deduce_ret<RETT,varset<T>>::type>::value,typename deduce_ret<RETT,varset<T>>::type>::type
getarg(const char *n, const varset<T> &vs, const S &arg1) {
	return (strcmp(n,vs.vname.name)!=0) ? throw std::logic_error(std::string("could not find variable named ")+n) : arg1;
}

template<typename RETT=void, typename T, typename S>
constexpr typename std::enable_if<!std::is_convertible<T,typename deduce_ret<RETT,varset<T>>::type>::value,typename deduce_ret<RETT,varset<T>>::type>::type
getarg(const char *n, const varset<T> &vs, const S &arg1) {
	return throw std::logic_error(std::string("could not find variable named ")+n);
}

template<typename RETT=void, typename T, typename S, typename... Ts,typename... Ss>
constexpr typename std::enable_if<std::is_convertible<T,typename deduce_ret<RETT,varset<T,Ts...>>::type>::value,typename deduce_ret<RETT,varset<T,Ts...>>::type>::type
getarg(const char *n, const varset<T,Ts...> &vs, const S &arg1, const Ss&... args) {
	static_assert(sizeof...(Ts)<=sizeof...(Ss),"too few parameters");
	static_assert(sizeof...(Ts)>=sizeof...(Ss),"too many parameters");
	return (strcmp(n,vs.vname.name)==0) ? arg1 : getarg<RETT>(n,vs.vnames,args...);
}

template<typename RETT=void, typename T, typename S, typename... Ts,typename... Ss>
constexpr typename std::enable_if<!std::is_convertible<T,typename deduce_ret<RETT,varset<T,Ts...>>::type>::value,typename deduce_ret<RETT,varset<T,Ts...>>::type>::type
getarg(const char *n, const varset<T,Ts...> &vs, const S &arg1, const Ss&... args) {
	static_assert(sizeof...(Ts)<=sizeof...(Ss),"too few parameters");
	static_assert(sizeof...(Ts)>=sizeof...(Ss),"too many parameters");
	return getarg<RETT>(n,vs.vnames,args...);
}

template<typename ...Ts>
struct varsetval : varset<Ts...> {
	template<typename ...Ss>
	constexpr varsetval(const varset<Ts...> &vars, Ss &&... ss)
		: varset<Ts...>(vars), vals(std::forward<Ss>(ss)...) {}
	template<typename ...Ss>
	constexpr varsetval(varset<Ts...> &&vars, Ss &&... ss)
		: varset<Ts...>(std::move(vars)), vals(std::forward<Ss>(ss)...) {}

	std::tuple<Ts...> vals;
};

// integer_sequence not yet defined in g++ 4.8.x

// template<typename... Ts>
// using ind_seq = std::index_sequence<Ts...>

// template<typename... Ts>
// using ind_seq_for = std::index_sequence_for<Ts...>

// from stack overflow question # 17424477
template<typename T, T... I> struct int_sequence {
	typedef T value_type;
	static constexpr size_t size() noexcept { return sizeof...(I); }
};

template<typename T, T N, T... I>
struct make_helper {
	typedef typename 

template<typename T, typename N>
using make_int_seq = typename gen_seq<T,N>::type;

template<std::size_t... Ints>
using ind_seq = int_seq<std::size_t, Ints...>;

template<std::size_t N>
using make_ind_seq = make_int_seq<std::size_t, N>;

template<typename... Ts>
using ind_seq_for = make_ind_seq<sizeof...(T)>;



template<typename RETT,typename ...Ts>
struct getarg_help {
	const varsetval<Ts...> args;

	template<typename S>
	constexpr getarg_help(S &&a) : args(std::forward<S>(a)) {}
	
	template<std::size_t ...I>
	constexpr auto callit(const char *n, ind_seq<I...>) 
		-> decltype(getarg<RETT>(n,args,std::get<I>(args.vals)...)) {
		return getarg<RETT>(n,args,std::get<I>(args.vals)...);
	}
	
	constexpr auto operator()(const char *n) 
		-> decltype(callit(n,ind_seq_for<Ts...>{})) {
		return callit(n,ind_seq_for<Ts...>{});
	}
};

template<typename RETT=void, typename ...Ts>
auto getarg(const char *n, const varsetval<Ts...> &vals)
	-> decltype(getarg_help<RETT,Ts...>{vals}(n)) {
	return getarg_help<RETT,Ts...>{vals}(n);
}

#endif
