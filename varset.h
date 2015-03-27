#ifndef VARSET_H
#define VARSET_H

#include <type_traits>
#include <utility>
#include <stdexcept>
#include <string.h>
#include <vector>
#include "con.h"
#include <complex>

template<typename T>
struct typeT {};

template<typename RETT, typename V>
struct deduce_ret {
	typedef RETT type;
};

template<typename V>
struct deduce_ret<anytype,V> {
typedef typename V::commontype type;
};

//template<typename T, typename E=void> struct tovar;

template<typename T> struct unnamedvar;

template<typename T, typename E=void>
struct tovar {
	typedef unnamedvar<T> type;
};

template<typename T>
struct tovar<T,typename T::commontype> {
	typedef T type;
};

template<typename T>
using makevar = typename tovar<T>::type; 

template<typename V> struct insttype;

template<typename T>
using inst = typename insttype<T>::type;

template<typename VV, typename VL>
struct assigntype {

	typedef VV vartype;
	typedef VL valtype;

	const VV vars;
	VL vals;

	template<typename T1, typename T2>
	constexpr assigntype(T1 &&a1, T2 &&a2) : vars(std::forward<T1>(a1)), vals(std::forward<T2>(a2)) {}

	template<typename T>
	constexpr assigntype(T &&a) : vars(), vals(std::forward<T>(a)) {}

	constexpr auto operator[](const char *n) const { return vals.getarg(n,vars); }
	auto operator[](const char *n) { return vals.getarg(n,vars); }
};

template<typename T>
using assign = assigntype<makevar<T>,inst<makevar<T>>>;

template<typename RETT, typename T>
constexpr typename std::enable_if<std::is_convertible<T,RETT>::value,RETT>::type
getarglist(int i, const T &arg1) {
	return i<0 ? (throw std::logic_error("could not find variable"), arg1) : arg1;
}

template<typename RETT, typename T>
constexpr typename std::enable_if<!std::is_convertible<T,RETT>::value,RETT>::type
getarglist(int i, const T &arg1) {
	return i<0 ? (throw std::logic_error("could not find variable"), arg1)
			: (throw std::logic_error("types don't match"), arg1);
}

template<typename RETT, typename T, typename... Ts>
constexpr typename std::enable_if<std::is_convertible<T,RETT>::value,RETT>::type
getarglist(int i, const T &arg1, Ts &&... args) {
	return i<0 ? getarg(i-1,std::forward<Ts>(args)...) : arg1;

}

template<typename RETT, typename T, typename... Ts>
constexpr typename std::enable_if<!std::is_convertible<T,RETT>::value,RETT>::type
getarglist(int i, const T &arg1, Ts &&... args) {
	return getarg(i-1,std::forward<Ts>(args)...);
}


template<typename RETT=anytype, typename V, typename... Ts>
constexpr typename deduce_ret<RETT,V>::type
getarglist(const char *n, const V &vs, Ts &&...args) {
	return getarg(vs.template getindex<RETT>(n),std::forward<Ts>(args)...);
}

template<typename RETT=anytype, typename V, typename VL>
constexpr typename deduce_ret<RETT,V>::type
getarg(const char *n, const V &vs, const VL &vl) {
	return vl.get(vs.template getindex<RETT>(n),typeT<typename deduce_ret<RETT,V>::type>{});
}

template<typename RETT=anytype, typename V, typename VL>
typename deduce_ret<RETT,V>::type &
getarg(const char *n, const V &vs, VL &vl) {
	return vl.get(vs.template getindex<RETT>(n),typeT<typename deduce_ret<RETT,V>::type>{});
}

template<typename RETT=anytype, typename V>
constexpr typename deduce_ret<RETT,V>::type
getarg(int i, const V &vl) {
	return vl.get(i,typeT<typename deduce_ret<RETT,V>::type>{});
}


template<typename RETT=anytype, typename V>
typename deduce_ret<RETT,V>::type &
getarg(int i, V &vl) {
	return vl.get(i,typeT<typename deduce_ret<RETT,V>::type>{});
}

template<typename RETT=anytype, typename T>
constexpr auto getarg(const char *n, const assign<T> &A) {
	return getarg(n,A.vars,A.vals);
}

template<typename RETT=anytype, typename T>
constexpr auto getarg(const char *n, assign<T> &A) {
	return getarg(n,A.vars,A.vals);
}

template<typename B>
struct varbase {
	B *bthis() { return static_cast<B *>(this); }
	constexpr const B *bthis() const { return static_cast<const B *>(this); }

	template<typename S=anytype>
	constexpr int operator[](const char *n) const {
		return bthis()->realgetindex(n,typeT<S>{});
	}

	template<typename S=anytype>
	constexpr int getindex(const char *n) const {
		return bthis()->realgetindex(n,typeT<S>{});
	}

	template<typename S=anytype>
	constexpr int getindex(const char *n, typeT<S>) const {
		return bthis()->realgetindex(n,typeT<S>{});
	}
};

template<typename B>
struct valbase {

	B *bthis() { return static_cast<B *>(this); }
	constexpr const B *bthis() const { return static_cast<const B *>(this); }

	template<typename RETT=anytype>
	constexpr decltype(auto) operator[](int i) const { //-> decltype(getarg<RETT>(i,this->bthis())) {
		return getarg<RETT>(i,*bthis());
	}

	template<typename RETT=anytype>
	decltype(auto) operator[](int i) { //-> decltype(getarg<RETT>(i,this->bthis())) {
		return getarg<RETT>(i,*bthis());
	}

};

template<typename T>
struct unnamedvar : varbase<unnamedvar<T>> {
	typedef T commontype;

	constexpr unnamedvar() {}

	template<typename S, typename std::enable_if<!std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n, typeT<S>) const {
		return -1;
	}

	template<typename S, typename std::enable_if<std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n,typeT<S>) const {
		return n==nullptr ? 0 : -1;
	}

	constexpr int realgetindex(const char *n, typeT<anytype>) const {
		return n==nullptr ? 0 : -1;
	}

	constexpr int nvar() const { return 1; }
};

template<typename T>
struct var : varbase<var<T>> {
	typedef T commontype;

	constexpr var(const char *n) : name(n) {}

	template<typename S, typename std::enable_if<!std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n, typeT<S>) const {
		return -1;
	}

	template<typename S, typename std::enable_if<std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n,typeT<S>) const {
		return ((n==nullptr && name==nullptr)
			|| (n!=nullptr && name!=nullptr && !strcmp(n,name))) ? 0 : -1;
	}

	constexpr int realgetindex(const char *n, typeT<anytype>) const {
		return ((n==nullptr && name==nullptr)
			|| (n!=nullptr && name!=nullptr && !strcmp(n,name))) ? 0 : -1;
	}

	constexpr int nvar() const { return 1; }


	const char *name;
};


template<typename T>
struct assigntype<var<T>,inst<var<T>>> {
	typedef var<T> vartype;
	typedef inst<var<T>> valtype;
	const vartype vars;
	valtype vals;

	template<typename T1, typename T2>
	constexpr assigntype(T1 &&a1, T2 &&a2) : vars(std::forward<T1>(a1)), vals(std::forward<T2>(a2)) {}

	template<typename A>
	constexpr assigntype(A &&a) : vars(), vals(std::forward<A>(a)) {}

	constexpr auto operator[](const char *n) const { return vals.getarg(n,vars); }
	auto operator[](const char *n) { return vals.getarg(n,vars); }

	constexpr operator T() const { return (T)vals; }
};

template<typename T>
struct assigntype<unnamedvar<T>,inst<unnamedvar<T>>> {
	typedef unnamedvar<T> vartype;
	typedef inst<unnamedvar<T>> valtype;
	const vartype vars;
	valtype vals;

	template<typename T1, typename T2>
	constexpr assigntype(T1 &&a1, T2 &&a2) : vars(std::forward<T1>(a1)), vals(std::forward<T2>(a2)) {}

	template<typename A>
	constexpr assigntype(A &&a) : vars(), vals(std::forward<A>(a)) {}

	constexpr auto operator[](const char *n) const { return vals.getarg(n,vars); }
	auto operator[](const char *n) { return vals.getarg(n,vars); }

	constexpr operator T() const { return (T)vals; }
};


template<typename T>
struct varval : public valbase<varval<T>> {
	typedef T commontype;
	T val;

	constexpr varval(T &&t) : val(std::move(t)) {}
	constexpr varval(const T &t) : val(t) {}

	constexpr operator T() const { return val; }

	constexpr const T &get(int i,typeT<T>) const {
		return i==0 ? val : (throw std::logic_error("could not find value"), val);
	}

	constexpr const T &get(int i,typeT<anytype>) const {
		return i==0 ? val : (throw std::logic_error("could not find value"), val);
	}

	template<typename RETT, typename std::enable_if<std::is_convertible<T,RETT>::value && !std::is_same<T,RETT>::value>::type *E=nullptr>
	constexpr RETT get(int i,typeT<RETT>) const {
		return i==0 ? val : (throw std::logic_error("could not find value"), val);
	}

	T &get(int i,typeT<T>) {
		return i==0 ? val : (throw std::logic_error("could not find value"), val);
	}

	T &get(int i,typeT<anytype>) {
		return i==0 ? val : (throw std::logic_error("could not find value"), val);
	}


	constexpr int nval() const { return 1; }
};

template<typename T>
struct unnamedmultvar : varbase<unnamedmultvar<T>> {
	typedef T commontype;

	constexpr unnamedmultvar(int n) : nv(n) {}

	template<typename S, typename std::enable_if<!std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n,typeT<S>,int i=0) const {
		return -1;
	}

	template<typename S, typename std::enable_if<std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n,typeT<S>,int i) const {
		return n==nullptr && i<nv ? 0 : -1;
	}

	constexpr int realgetindex(const char *n,typeT<anytype>,int i) const {
		return n==nullptr && i<nv ? 0 : -1;
	}

	int nv;

	int nvar() const { return nv; }
};

template<typename T>
struct multvar : varbase<multvar<T>> {
	typedef T commontype;

	multvar(std::initializer_list<const char *> ns) : names(ns) {}

	template<typename S, typename std::enable_if<!std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n,typeT<S>,int i=0) const {
		return -1;
	}

	template<typename S, typename std::enable_if<std::is_convertible<T,S>::value>::type *E=nullptr>
	constexpr int realgetindex(const char *n,typeT<S>,int i=0) const {
		return i>=names.size() ? -1 : 
				(names[i]==nullptr && n==nullptr) ? i :
					(n!=nullptr && names[i]!=nullptr && !strcmp(n,names[i])) ? i :
						realgetindex(n,typeT<anytype>{},i+1);
	}

	constexpr int realgetindex(const char *n,typeT<anytype>,int i=0) const {
		return i>=names.size() ? -1 : 
				(names[i]==nullptr && n==nullptr) ? i :
					(n!=nullptr && names[i]!=nullptr && !strcmp(n,names[i])) ? i :
						realgetindex(n,typeT<anytype>{},i+1);
	}

	const std::vector<const char *> names;

	int nvar() const { return names.size(); }
};

template<typename T>
struct multvarval : public valbase<multvarval<T>> {
	typedef T commontype;

	std::vector<T> val;

	multvarval(std::initializer_list<T> args) : val(args) {}
	template<typename S>
	multvarval(int n, S &&v) : val(n,std::forward<S>(v)) {}

	const T &get(int i,typeT<T>) const {
		return i<nval() && i>=0
			? val[i] : (throw std::logic_error("could not find value"), val[i]);
	}

	const T &get(int i,typeT<anytype>) const {
		return i<nval() && i>=0
			? val[i] : (throw std::logic_error("could not find value"), val[i]);
	}

	template<typename RETT, typename std::enable_if<!std::is_same<T,RETT>::value
					&& std::is_convertible<T,RETT>::value>::type *E=nullptr>
	RETT get(int i,typeT<RETT>) const {
		return i<nval() && i>=0
			? val[i] : (throw std::logic_error("could not find value"), val[i]);
	}

	template<typename RETT, typename std::enable_if<!std::is_convertible<T,RETT>::value>::type *E=nullptr>
	RETT get(int i,typeT<RETT>) const {
		return (throw std::logic_error("could not find value"), val[i]);
	}

	T &get(int i,typeT<T>) {
		return i<nval() && i>=0
			? val[i] : (throw std::logic_error("could not find value"), val[i]);
	}

	T &get(int i,typeT<anytype>) {
		return i<nval() && i>=0
			? val[i] : (throw std::logic_error("could not find value"), val[i]);
	}

	template<typename RETT, typename std::enable_if<!std::is_same<T,RETT>::value>::type *E=nullptr>
	RETT &get(int i,typeT<RETT>) {
		return (throw std::logic_error("could not find value with exact same type"), val[i]);
	}

	inline int nval() const { return val.size(); }
};


template<typename VT1, typename VT2>
struct varpair : public std::pair<VT1,VT2>, public varbase<varpair<VT1,VT2>> {
	typedef typename common_or_none<typename VT1::commontype,typename VT2::commontype>::type commontype;

	template<typename NT, typename... NTS>
	constexpr varpair(NT &&n, NTS &&... ns) 
		: std::pair<VT1,VT2>(std::forward<NT>(n),VT2{std::forward<NTS>(ns)...}) {}

	template<typename S>
	constexpr int realgetindex(const char *n, typeT<S>) const {
		return getindexhelp(this->first.template getindex<S>(n,typeT<S>{}),n,typeT<S>{});
	}

	template<typename S>
	constexpr int getindexhelp(int find, const char *n, typeT<S>) const {
		return find<0 ? getindexhelp2(this->second.template getindex<S>(n,typeT<S>{})) : find;
	}

	constexpr int getindexhelp2(int find) const {
		return find<0 ? -1 : find+this->first.nvar();
	}

	constexpr int nvar() const { return this->first.nvar()+this->second.nvar(); }
};



template<typename RT, typename CT, bool same, typename E=void>
struct getret {
	typedef RT type;
};

template<typename CT>
struct getret<CT,CT,true,
		typename std::enable_if<!std::is_same<CT,
							typename std::add_lvalue_reference<CT>::type>::value>::type> {
	typedef const CT & type;
};

template<typename V, typename T, typename E=void>
struct getable {
	static constexpr bool value = false;
};

template<typename T, typename S>
struct getable<varval<T>,S,typename std::enable_if<std::is_convertible<T,S>::value>::type> {
	static constexpr bool value = true;
};

template<typename T, typename S>
struct getable<multvarval<T>,S,typename std::enable_if<std::is_convertible<T,S>::value>::type> {
	static constexpr bool value = true;
};

template<typename V, typename T>
struct setable {
	static constexpr bool value = false;
};

template<typename T>
struct setable<varval<T>,T> {
	static constexpr bool value = true;
};

template<typename T>
struct setable<multvarval<T>,T> {
	static constexpr bool value = true;
};

template<typename V>
struct allsame {
	static constexpr bool value = false;
};

template<typename T>
struct allsame<var<T>> {
	static constexpr bool value = true;
};

template<typename T>
struct allsame<multvar<T>> {
	static constexpr bool value = true;
};

template<typename T>
struct allsame<varval<T>> {
	static constexpr bool value = true;
};

template<typename T>
struct allsame<multvarval<T>> {
	static constexpr bool value = true;
};
	



template<typename VV1, typename VV2>
struct varvalpair : public std::pair<VV1,VV2>, public valbase<varvalpair<VV1,VV2>> {
	typedef typename common_or_none<typename VV1::commontype,typename VV2::commontype>::type commontype;
	typedef varvalpair<VV1,VV2> thistype;

	template<typename V, typename... Vs>
	constexpr varvalpair(V &&v, Vs &&...vs) 
		: std::pair<VV1,VV2>(std::forward<V>(v),VV2{std::forward<Vs>(vs)...}) {}

	template<typename RETT=commontype>
	constexpr typename std::enable_if<
					getable<VV1,RETT>::value && getable<VV2,RETT>::value,
					typename getret<RETT,commontype,allsame<thistype>::value>::type
								>::type
	get(int i,typeT<RETT>) const {
		return i<this->first.nval() ? this->first.get(i,typeT<RETT>{}) : this->second.get(i-this->first.nval(),typeT<RETT>{});
	}

	template<typename Q=thistype>
	constexpr typename std::enable_if<!std::is_same<typename Q::commontype,notype>::value,const commontype &>::type
	get(int i,typeT<anytype>) const {
		return i<this->first.nval() ? this->first.get(i,typeT<commontype>{}) : this->second.get(i-this->first.nval(),typeT<commontype>{});
	}

	template<typename RETT=commontype>
	constexpr typename std::enable_if<
					getable<VV1,RETT>::value && !getable<VV2,RETT>::value,
					typename getret<RETT,commontype,allsame<thistype>::value>::type
						>::type
	get(int i,typeT<RETT>) const {
		return i<this->first.nval() ? this->first.get(i,typeT<RETT>{}) : 
			(throw std::logic_error("could not find value"), this->first.get(i,typeT<RETT>{}));
	}

	template<typename RETT=commontype>
	constexpr typename std::enable_if<
					!getable<VV1,RETT>::value && getable<VV2,RETT>::value,
					typename getret<RETT,commontype,allsame<thistype>::value>::type
						>::type
	get(int i,typeT<RETT>) const {
		return i<this->first.nval() ? (throw std::logic_error("could not find value"), this->second.get(i-this->first.nval(),typeT<RETT>{})) : this->second.get(i-this->first.nval(),typeT<RETT>{});
	}

/*
	template<typename Q=thistype>
	constexpr typename std::enable_if<allsame<Q>::value, const commontype &>::type
	get(int i,typeT<anytype>) const {
		return i<this->first.nval() ? this->first.get(i,typeT<commontype>{}) : this->second.get(i-this->first.nval(),typeT<commontype>{});
	}

	template<typename Q=thistype>
	constexpr typename std::enable_if<allsame<Q>::value, const commontype &>::type
	get(int i,typeT<commontype>) const {
		return i<this->first.nval() ? this->first.get(i,typeT<commontype>{}) : this->second.get(i-this->first.nval(),typeT<commontype>{});
	}

	template<typename RETT=commontype>
	constexpr typename std::enable_if<
			!allsame<thistype>::value && std::is_same<RETT,typename VV1::commontype>::value
			        && !std::is_same<RETT,typename VV2::commontype>::value,
			const RETT &
			>::type
	get(int i,typeT<RETT>) const {
		return i<this->first.nval() ? this->first.get(i,typeT<RETT>{}) : 
				(throw std::logic_error("could not find value"), this->first.get(i,typeT<RETT>{}));
	}

	template<typename RETT=commontype>
	constexpr typename std::enable_if<
			!allsame<thistype>::value && !std::is_same<RETT,typename VV1::commontype>::value
			        &&   std::is_same<RETT,typename VV2::commontype>::value,
			RETT &
			>::type
	get(int i,typeT<RETT>) {
		return i<this->first.nval() ?
			(throw std::logic_error("could not find value"), this->second.get(i-this->first.nval(),typeT<RETT>{}))
			: this->second.get(i-this->first.nval(),typeT<RETT>{});
	}
*/

	template<typename Q=thistype>
	typename std::enable_if<allsame<Q>::value, commontype &>::type
	get(int i,typeT<anytype>) {
		return i<this->first.nval() ? this->first.get(i,typeT<commontype>{}) : this->second.get(i-this->first.nval(),typeT<commontype>{});
	}

	template<typename Q=thistype>
	typename std::enable_if<allsame<Q>::value, commontype &>::type
	get(int i,typeT<commontype>) {
		return i<this->first.nval() ? this->first.get(i,typeT<commontype>{}) : this->second.get(i-this->first.nval(),typeT<commontype>{});
	}

	template<typename RETT=commontype>
	typename std::enable_if<
			!allsame<thistype>::value && std::is_same<RETT,typename VV1::commontype>::value
			        && !std::is_same<RETT,typename VV2::commontype>::value,
			RETT &
			>::type
	get(int i,typeT<RETT>) {
		return i<this->first.nval() ? this->first.get(i,typeT<RETT>{}) : 
				(throw std::logic_error("could not find value"), this->first.get(i,typeT<RETT>{}));
	}

	template<typename RETT=commontype>
	typename std::enable_if<
			!allsame<thistype>::value && !std::is_same<RETT,typename VV1::commontype>::value
			        &&   std::is_same<RETT,typename VV2::commontype>::value,
			RETT &
			>::type
	get(int i,typeT<RETT>) {
		return i<this->first.nval() ?
			(throw std::logic_error("could not find value"), this->second.get(i-this->first.nval(),typeT<RETT>{}))
			: this->second.get(i-this->first.nval(),typeT<RETT>{});
	}
};

template<typename V1, typename V2, typename S>
struct getable<varvalpair<V1,V2>,S,
		typename std::enable_if<getable<V1,S>::value || getable<V2,S>::value>::type> {
	static constexpr bool value = true;
};

template<typename V1, typename V2, typename T>
struct setable<varvalpair<V1,V2>,T> {
	static constexpr bool value = setable<V1,T>::value || setable<V2,T>::value;
};

template<typename V1, typename V2>
struct allsame<varpair<V1,V2>> {
	static constexpr bool value = 
		allsame<V1>::value && allsame<V2>::value &&
		std::is_same<typename V1::commontype,typename V2::commontype>::value;
};

template<typename V1, typename V2>
struct allsame<varvalpair<V1,V2>> {
	static constexpr bool value = 
		allsame<V1>::value && allsame<V2>::value &&
		std::is_same<typename V1::commontype,typename V2::commontype>::value;
};

template<typename T>
struct insttype<var<T>> {
	typedef varval<T> type;
};

template<typename T>
struct insttype<unnamedvar<T>> {
	typedef varval<T> type;
};

template<typename T>
struct insttype<multvar<T>> {
	typedef multvarval<T> type;
};

template<typename T>
struct insttype<unnamedmultvar<T>> {
	typedef varval<T> type;
};

template<typename T1,typename T2>
struct insttype<varpair<T1,T2>> {
	typedef varvalpair<typename insttype<T1>::type, typename insttype<T2>::type> type;
};

template<typename T>
struct insttype<const var<T>> {
	typedef varval<T> type;
};

template<typename T>
struct insttype<const unnamedvar<T>> {
	typedef varval<T> type;
};

template<typename T>
struct insttype<const multvar<T>> {
	typedef multvarval<T> type;
};

template<typename T>
struct insttype<const unnamedmultvar<T>> {
	typedef varval<T> type;
};

template<typename T1,typename T2>
struct insttype<const varpair<T1,T2>> {
	typedef varvalpair<typename insttype<T1>::type, typename insttype<T2>::type> type;
};






#endif
