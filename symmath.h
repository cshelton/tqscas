#ifndef SYMMATH_H
#define SYMMATH_H

#include <utility>
#include <tuple>
#include <type_traits>
#include <cmath>
#include <stdexcept>
#include <string>
#include <cstring>
#include <iostream>
#include "ctstring.h"
#include "commonunion.h"

// some predefinition declarations
template<typename RANGE> struct rtconstsym;
template<typename RANGE, RANGE k> struct ctconstsym;
template<typename RANGE> struct ctconstsymzero;
template<typename RANGE> struct ctconstsymidentity;
template<typename DERIV, typename RANGE> struct sym;
template<typename LHS, typename RHS> struct assignexpr;
template<typename A1,typename A2> struct assignpair;
template<typename T,typename EN=void> struct symtypeinfo {};
struct absmathexpr {};


// some TMP constructs to allow detection of math symbols 
template<typename T>
struct makevoid {
	typedef void type;
};

template<typename> struct getrange;


/*
template<typename T, typename R=void>
struct is_mathsym {
	enum { value = 0 };
};

template<typename T>
struct is_mathsym<T,typename makevoid<typename getrange<T>::type>::type> {
	enum { value = 1 };
};
*/

template<typename T>
struct is_mathsym {
	enum { value = std::is_base_of<absmathexpr,typename std::decay<T>::type>::value };
};

// derivative rules & type information (needs to be seen first, unfortunately)
//
template<typename T1, typename T2, typename EN=void> struct symtypepairinfo{
	typedef T1 derivtype; // just to satisfy the compiler -- shouldn't be used
	enum { hasderiv = 0 };
};

template<>
struct symtypeinfo<std::string,void> {
	static std::string identity() { return {0}; }
	static std::string zero() { return {0}; }
	enum { ctconst = 0 };
};

template<typename T>
struct symtypeinfo<T,
	typename std::enable_if<std::is_arithmetic<T>::value>::type> {
	static constexpr T identity() { return {1}; }
	static constexpr T zero() { return {0}; }
	enum { ctconst = 1 };

};

template<typename T1,typename T2>
struct symtypepairinfo<T1,T2,
		typename std::enable_if<std::is_floating_point<T1>::value
				&& std::is_floating_point<T2>::value>::type> {
	typedef typename std::common_type<T1,T2>::type derivtype;
	enum { simplifyadd0 = 1 };
	enum { simplifymult0 = 1 };
	enum { simplifymult1 = 1 };
	enum { hasderiv = 1 };
};

template<typename T1, typename T2>
struct symtypepairinfo<T1,T2,
		typename std::enable_if<std::is_integral<T1>::value
				&& std::is_integral<T2>::value>::type> {
	typedef typename std::common_type<T1,T2>::type derivtype; // maybe?
	enum { simplifyadd0 = 1 };
	enum { simplifymult0 = 1 };
	enum { simplifymult1 = 1 };
	enum { hasderiv = 1 };
};

template<typename T1, typename T2>
struct symtypepairinfo<T1,T2,
		typename std::enable_if<std::is_floating_point<T1>::value
				&& std::is_integral<T2>::value>::type> {
	typedef typename std::common_type<T1,T2>::type derivtype;
	enum { simplifyadd0 = 1 };
	enum { simplifymult0 = 1 };
	enum { simplifymult1 = 1 };
	enum { hasderiv = 1 };
};

template<typename T1, typename T2>
struct symtypepairinfo<T1,T2,
		typename std::enable_if<std::is_integral<T1>::value
				&& std::is_floating_point<T2>::value>::type> {
	typedef typename std::common_type<T1,T2>::type derivtype;
	enum { simplifyadd0 = 1 };
	enum { simplifymult0 = 1 };
	enum { simplifymult1 = 1 };
	enum { hasderiv = 1 };
};

template<typename T1,typename E>
struct symtypepairinfo<T1,E,typename std::enable_if<is_mathsym<E>::value>::type>
		: public symtypepairinfo<T1,typename getrange<E>::type> {
};

// mathexpr is the base type for all types that represent
// a symbolic math expression (and know their type)
// DERIV = derived type (see CRTP) [not "deriviative"]
//
// the derived class must implement
// * dosubst (on both assignexpr and typelessassign)
// * doderiv (for any is_mathsym expression)
// * doval
// * doname
// * doprecedence
// * doprint
template<typename DERIV,typename RANGE>
struct mathexpr : public absmathexpr {
	typedef RANGE range;

	constexpr const DERIV &dclassref() const
		{ return static_cast<const DERIV &>(*this); }
	DERIV *dclassref()
		{ return static_cast<DERIV &>(*this); }

	struct getval {
		template<typename E>
		constexpr auto operator()(E &&e) const
			{ return std::forward<E>(e).doval(); }
	};
	template<typename V, typename E>
	struct getsubst {
		const V &v; const E &e;
		constexpr getsubst(const V &vv, const E &ee) : v(vv), e(ee) {}
		template<typename EX>
		constexpr auto operator()(EX &&ex) const
			{ return std::forward<EX>(ex).dosubst(v,e); }
	};
	struct recomb  {
		template<typename OP, typename... Ts>
		constexpr auto operator()(OP &&op, Ts &&...ts) const
			{ return std::forward<OP>(op)(std::forward<Ts>(ts)...); }
	};
	constexpr auto val() const { return fold(recomb{},getval{}); }

	// derivative 
	template<typename E,
		typename std::enable_if<is_mathsym<E>::value>::type *EN=nullptr>
	constexpr auto d(const E &x) const
		{ return dclassref().doderiv(x); }

	constexpr auto isconst() const { return dclassref().doisconst(); }

	template<typename COMBF, typename LEAFF>
	constexpr auto fold(COMBF &&cf, LEAFF &&lf) const {
		return dclassref().dofold(std::forward<COMBF>(cf),
							std::forward<LEAFF>(lf));
	}

	template<typename V, typename E>
	constexpr auto operator[](const assignexpr<V,E> &a) const
		{ return dclassref().splitassign(a.lhs,a.rhs); }

	template<typename V, typename E>
	constexpr auto splitassign(const V &v, const E &e) const
		{ return fold(recomb{},getsubst<V,E>{v,e}); }

	template<typename A1, typename A2>
	constexpr auto operator[](const assignpair<A1,A2> &a) const {
		return dclassref()[a.first][a.second];
	}

	constexpr int precedence() const { return dclassref().doprecedence(); }
	constexpr int doprededence() const { return 0; }

	template<typename OP2>
	constexpr bool doisconst() const { return false; }


	constexpr const auto name() const { return dclassref().doname(); }
	constexpr const auto doname() const { return ""; }
	template<typename COMBF, typename LEAFF> // default to "leaf"
	constexpr auto dofold(COMBF &&cf, LEAFF &&lf) const
			{ return std::forward<LEAFF>(lf)(dclassref()); }
	void print(std::ostream &os, int prec=1000) const { return dclassref().doprint(os,prec); }
	
};


template<typename...>
struct commonrange;

template<typename T1, typename T2, typename... Ts>
struct commonrange<T1,T2,Ts...> {
	typedef typename std::common_type<typename getrange<T1>::type,
			typename commonrange<T2,Ts...>::type>::type type;
};

template<typename T>
struct commonrange<T> {
	typedef typename getrange<T>::type type;
};


#define UNIONBASE : public mathexpr<cu_impl<void,Ts...>, typename commonrange<Ts...>::type>

DEFUNION(mathexprunion,UNIONBASE,dosubst,dofold,doderiv,doval,doisconst,doname,doprecedence,doprint)

template<typename T>
struct getrange {
	typedef typename std::decay<T>::type::range type;
};

template<typename T1, typename... Ts>
struct getrange<commonunion::mathexprunion_node_impl::cu_impl<void,T1,Ts...>> {
	typedef typename getrange<T1>::type type;
};

// assign expression (which consists just of a pair of symbolic expressions:
template<typename LHS, typename RHS>
struct assignexpr {
	static_assert(std::is_same<typename getrange<LHS>::type, typename getrange<RHS>::type>::value,"assignment doesn't preserve type");

	typedef typename getrange<LHS>::type range;

	typename std::decay<LHS>::type lhs;
	typename std::decay<RHS>::type rhs;
	template<typename T1, typename T2>
	constexpr assignexpr(T1 &&l, T2 &&r) : lhs(std::forward<T1>(l)),
			rhs(std::forward<T2>(r)) {}

	void print(std::ostream &os) const {
		lhs.print(os); os << " <- "; rhs.print(os);
	}
};

// use == to build assignexpr:
template<typename E1, typename E2, 
		typename std::enable_if<is_mathsym<E1>::value && is_mathsym<E2>::value>::type *E=nullptr>
constexpr auto operator==(E1 &&e1, E2 &&e2) {
	return assignexpr<
				typename std::decay<E1>::type,
				typename std::decay<E2>::type>
		{std::forward<E1>(e1),std::forward<E2>(e2)};
}

// use == to build assignexpr (when rhs is a constant)
template<typename E1,
		typename std::enable_if<is_mathsym<E1>::value>::type *E=nullptr>
constexpr
assignexpr<typename std::decay<E1>::type,
			rtconstsym<typename getrange<E1>::type>>
operator==(E1 &&e1, const typename getrange<E1>::type &e2) {
	return {std::forward<E1>(e1),rtconstsym<typename getrange<E1>::type>{e2}};
}

// assignpair represents a tree of assignments
//  (A1 and A2 are each either another assignpair or an assignexpr)
template<typename A1,typename A2>
struct assignpair {
	template<typename T1, typename T2>
	constexpr assignpair(T1 &&l, T2 &&r) : first(std::forward<T1>(l)),
						second(std::forward<T2>(r)) {}
	A1 first;
	A2 second;

	void print(std::ostream &os) const {
		first.print(os); os << " & "; second.print(os);
	}
};

// use & to build assignpair (and thereby whole lists
//    -- or trees -- of assignments):
// (below is tedeous, but done)
template<typename A1, typename A2, typename A3, typename A4>
constexpr assignpair<assignpair<A1,A2>,assignpair<A3,A4>>
operator&(const assignpair<A1,A2> &a1, const assignpair<A3,A4> &a2) {
	return {a1,a2};
}
template<typename A1, typename A2, typename A3, typename A4>
constexpr assignpair<assignpair<A1,A2>,assignpair<A3,A4>>
operator&(assignpair<A1,A2> &&a1, const assignpair<A3,A4> &a2) {
	return {std::move(a1),a2};
}
template<typename A1, typename A2, typename A3, typename A4>
constexpr assignpair<assignpair<A1,A2>,assignpair<A3,A4>>
operator&(const assignpair<A1,A2> &a1, assignpair<A3,A4> &&a2) {
	return {a1,std::move(a2)};
}
template<typename A1, typename A2, typename A3, typename A4>
constexpr assignpair<assignpair<A1,A2>,assignpair<A3,A4>>
operator&(assignpair<A1,A2> &&a1, assignpair<A3,A4> &&a2) {
	return {std::move(a1),std::move(a2)};
}

template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignexpr<V,E>,assignpair<A1,A2>>
operator&(const assignexpr<V,E> &a1, const assignpair<A1,A2> &a2) {
	return {a1,a2};
}
template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignexpr<V,E>,assignpair<A1,A2>>
operator&(assignexpr<V,E> &&a1, const assignpair<A1,A2> &a2) {
	return {std::move(a1),a2};
}
template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignexpr<V,E>,assignpair<A1,A2>>
operator&(const assignexpr<V,E> &a1, assignpair<A1,A2> &&a2) {
	return {a1,std::move(a2)};
}
template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignexpr<V,E>,assignpair<A1,A2>>
operator&(assignexpr<V,E> &&a1, assignpair<A1,A2> &&a2) {
	return {std::move(a1),std::move(a2)};
}

template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignpair<A1,A2>,assignexpr<V,E>>
operator&(const assignpair<A1,A2> &a1, const assignexpr<V,E> &a2) {
	return {a1,a2};
}
template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignpair<A1,A2>,assignexpr<V,E>>
operator&(assignpair<A1,A2> &&a1, const assignexpr<V,E> &a2) {
	return {std::move(a1),a2};
}
template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignpair<A1,A2>,assignexpr<V,E>>
operator&(const assignpair<A1,A2> &a1, assignexpr<V,E> &&a2) {
	return {a1,std::move(a2)};
}
template<typename V, typename E, typename A1, typename A2>
constexpr assignpair<assignpair<A1,A2>,assignexpr<V,E>>
operator&(assignpair<A1,A2> &&a1, assignexpr<V,E> &&a2) {
	return {std::move(a1),std::move(a2)};
}

template<typename V1, typename V2, typename E1, typename E2>
constexpr assignpair<assignexpr<V1,E1>,assignexpr<V2,E2>>
operator&(const assignexpr<V1,E1> &a1, const assignexpr<V2,E2> &a2) {
	return {a1,a2};
}
template<typename V1, typename V2, typename E1, typename E2>
constexpr assignpair<assignexpr<V1,E1>,assignexpr<V2,E2>>
operator&(assignexpr<V1,E1> &&a1, const assignexpr<V2,E2> &a2) {
	return {std::move(a1),a2};
}
template<typename V1, typename V2, typename E1, typename E2>
constexpr assignpair<assignexpr<V1,E1>,assignexpr<V2,E2>>
operator&(const assignexpr<V1,E1> &a1, assignexpr<V2,E2> &&a2) {
	return {a1,std::move(a2)};
}
template<typename V1, typename V2, typename E1, typename E2>
constexpr assignpair<assignexpr<V1,E1>,assignexpr<V2,E2>>
operator&(assignexpr<V1,E1> &&a1, assignexpr<V2,E2> &&a2) {
	return {std::move(a1),std::move(a2)};
}


//-----------------------------------------------------------
// SPECIFIC SYMBOLS AND EXPRESSIONS
//-----------------------------------------------------------

struct absconstexpr { };

// a constant
template<typename DERIV, typename RANGE>
struct constsym : public mathexpr<DERIV,RANGE> , public absconstexpr {
	void doprint(std::ostream &os, int) const { os << this->val(); }

	template<typename V, typename E>
	constexpr DERIV dosubst(const V &, const E &) const
		{ return this->dclassref(); }

	template<typename E, typename symtypepairinfo<RANGE,E>::derivtype *EN=nullptr>
	constexpr
	ctconstsymzero<typename symtypepairinfo<RANGE,E>::derivtype>
	doderiv(const E &) const { return {}; };
};

// special case of "zero"
template<typename RANGE>
struct ctconstsymzero : public constsym<ctconstsymzero<RANGE>,RANGE> {
	constexpr auto doval() const { return symtypeinfo<RANGE>::zero(); }
	constexpr bool doisconst() const { return true; }
};

// special case of "one"
template<typename RANGE>
struct ctconstsymidentity : public constsym<ctconstsymidentity<RANGE>,RANGE> {
	constexpr auto doval() const { return symtypeinfo<RANGE>::identity(); }
	constexpr bool doisconst() const { return true; }
};

// at compile time:
template<typename RANGE, RANGE value>
struct ctconstsym : public constsym<ctconstsym<RANGE,value>,RANGE> {
	static_assert(std::is_integral<RANGE>::value,"compile-time constants only valid for integral types -- at least until C++17");
	constexpr auto doval() const { return value; }
	constexpr bool doisconst() const { return true; }
	void doprint(std::ostream &os, int) const { os << this->val() << "CT"; }
};

// at runtime:
template<typename RANGE>
struct rtconstsym : public constsym<rtconstsym<RANGE>,RANGE> {
	void doprint(std::ostream &os, int) const { os << this->val() << "RT"; }
	constexpr rtconstsym(const RANGE &r) : k(r) {}
	constexpr rtconstsym(RANGE &&r) : k(std::move(r)) {}
	constexpr auto doval() const { return k; }
	constexpr bool doisconst() const { return true; }

	RANGE k;
};

struct abssymexpr { };

// a symbol (like "x"), still abstract as there are a few ways of
// specifying the name of the symbol
template<typename DERIV, typename RANGE>
struct sym : public mathexpr<DERIV,RANGE>, public abssymexpr {
	template<typename E>
	constexpr bool samesym(const E &e) const {
		return this->dclassref().samesym(e);
	}

	constexpr RANGE doval() const {
		return false ? RANGE{0} : throw std::logic_error("symbol has no value");
	}
	constexpr bool doisconst() const { return false; }

	template<typename E>
	constexpr
	auto
	doderiv(const E &e) const { 
		typedef typename symtypepairinfo<RANGE,E>::derivtype R;
		return RTEXPRSELECT(mathexprunion,samesym(e),
			ctconstsymidentity<R>{},
			ctconstsymzero<R>{});
	}

	template<typename V, typename E,
		typename std::enable_if<
			std::is_same<typename getrange<V>::type,RANGE>::value>::type
						*EN=nullptr>
	constexpr auto
	dosubst(const V &v, const E &e) const {
		return RTEXPRSELECT(mathexprunion,samesym(v), e, this->dclassref());
	}

	template<typename V, typename E>
	constexpr
	typename std::enable_if<!std::is_same<typename getrange<V>::type,RANGE>::value,
		DERIV>::type
	dosubst(const V &v, const E &e) const {
		return this->dclassref();
	}

	void doprint(std::ostream &os, int) const { os << nametostring(this->name()); }
};

template<typename RANGE,char ...N> struct staticsym;

template<typename T>
struct is_staticsym {
	enum { value = 0 };
};

template<typename RANGE, char ...N>
struct is_staticsym<staticsym<RANGE,N...>> {
	enum { value = 1 };
};

// a symbol whose name is specified at compile time
// for instance: staticsym<double,'x,'2'> blah;
//    blah is now the representation of "x2"
// probably would make code more readable to write
// 		staticsym<double,'x','2'> x2;
template<typename RANGE,char ...N>
struct staticsym : public sym<staticsym<RANGE,N...>,RANGE> {

	constexpr staticsym() {}
	constexpr staticsym(const staticsym<RANGE,N...> &) {}

	typedef staticsym<RANGE,N...> mytype;
	static constexpr const char n[] = {N...,0};

	constexpr ctstring<N...> doname() const {
		return {};
	}
	
	constexpr bool samesym(const sym<mytype,RANGE> &s) const
		{ return true; }

	template<typename R, char... M>
	constexpr bool samesym(const sym<staticsym<R,M...>,R> &s) const 
		{ return false; }

	template<typename E>
	constexpr bool samesym(const E &e) const
		{ return !cmpnames(e.name(),this->name()); }

	template<typename R, typename E, char... M>
	constexpr mytype dosubst(const sym<staticsym<R,M...>,R> &, const E &) const
		{ return *this; }

	template<typename E>
	constexpr E dosubst(const mytype &, const E &e) const
		{ return e; }

	template<typename V, typename E,
		typename std::enable_if<
			!is_staticsym<V>::value &&
			std::is_same<RANGE,
				typename getrange<V>::type>::value>::type *EN=nullptr>
	constexpr auto
	dosubst(const V &v, const E &e) const
	{ return RTEXPRSELECT(mathexprunion,samesym(v), e, *this); }

	template<typename V, typename E,
		typename std::enable_if<
			!is_staticsym<V>::value &&
			!std::is_same<RANGE,
				typename getrange<V>::type>::value>::type *EN=nullptr>
	constexpr auto
	dosubst(const V &v, const E &e) const
	{ return *this; } 

	constexpr ctconstsymidentity<RANGE> doderiv(const sym<mytype,RANGE> &s) const
		{ return {}; }

	template<typename R, char... M>
	constexpr ctconstsymzero<RANGE> doderiv(const sym<staticsym<R,M...>,R> &s) const 
		{ return {}; }

	template<typename E,
		typename std::enable_if<
				!is_staticsym<E>::value>::type *EN=nullptr>
	constexpr auto
	doderiv(const E &e) const
	{ return RTEXPRSELECT(mathexprunion,!cmpnames(e.name(),this->name()),
			ctconstsymidentity<RANGE>{},
			ctconstsymzero<RANGE>{}); }
		
	constexpr RANGE doval() const {
		return false ? symtypeinfo<RANGE>::zero() : throw std::logic_error(std::string("symbol ")+nametostring(this->name())+" is unassigned");
	}
	constexpr bool doisconst() const { return false; }
};


// symbol whose name is set at compile time
// for instance: dynsym<double> blah("x2");
// probably better written as dynsym<double> x2("x2");
//  (to avoid confusion)
template<typename RANGE>
struct dynsym : public sym<dynsym<RANGE>,RANGE> { 
	const char *n;
/*
	constexpr
	dynsym(const char *name) : n(name) { 
	}
*/

	dynsym(const char *name) {
		n = strcpy(new char[strlen(name)+1],name);
	}
	dynsym(const std::string name) {
		n = strcpy(new char[name.size()+1],name.c_str());
	}
	dynsym(const dynsym<RANGE> &d) {
		n = strcpy(new char[strlen(d.n)+1],d.n);
	}
	dynsym(dynsym<RANGE> &&d) {
		n = d.n;
		d.n = nullptr;
	}
	~dynsym() {
		if (n) delete []n;
	}
	dynsym<RANGE> &operator=(const dynsym<RANGE> &d) {
		if (this==&d) return *this;
		if (n) delete []n;
		n = strcpy(new char[strlen(d.n)+1],d.n);
		return *this;
	}
	dynsym<RANGE> &operator=(dynsym<RANGE> &&d) {
		if (n) delete []n;
		n = d.n;
		d.n = nullptr;
		return *this;
	}

	typedef dynsym<RANGE> mytype;

	//constexpr
	const char *doname() const {
		return n;
	}
	
	template<typename T>
	//constexpr
	bool samesym(const T &s) const
		{ return !cmpnames(this->name(),s.name()); }

	constexpr RANGE doval() const {
		return false ? RANGE{0} : throw std::logic_error(std::string("symbol ")+n+" is unassigned");
	}
	constexpr bool doisconst() const { return false; }

};

// information for a particular operation on a particular set of types:
template<typename OP, typename... Ts>
struct opinfo {
	enum {commutes = 0 };
	enum {precedence = 100 };
	enum {infix = 0 };
	enum {prefix= 0 };
	enum {isfunc = 1 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "unknownop";
};


// a method to invoke val using arguments from a tuple:
/* based on stackoverflow 7858817, answer by Walter */
template<typename F1, typename F2, typename... Fs, std::size_t... I>
constexpr auto doopfn_help(F1 &&f1, F2 &&f2, const std::tuple<Fs...> &fs, 
			std::index_sequence<I...>) {
	return std::forward<F1>(f1)(
			std::forward<F2>(f2)(std::get<I>(fs))...); }
// computes f1(f2(fs)...) (if fs were "expanded as a parameter pack")
template<typename F1, typename F2, typename... Fs>
constexpr auto doopfn(F1 &&f1, F2 &&f2, const std::tuple<Fs...> &fs) {
	return doopfn_help(std::forward<F1>(f1),
			std::forward<F2>(f2),fs,std::index_sequence_for<Fs...>{});
}


struct absopexpr {};

// An expression representing an operation (OP) on types F1 through Fn
template<typename OP, typename F1, typename... Fs>
struct symop : public mathexpr<symop<OP,F1,Fs...>,
				decltype(std::declval<OP>()(
					std::declval<typename getrange<F1>::type>(),
					std::declval<typename getrange<Fs>::type>()...))
			>, public absopexpr {
	typedef std::tuple<F1,Fs...> TT;
	typedef mathexpr<symop<OP,F1,Fs...>,
			decltype(std::declval<OP>()(
					std::declval<typename getrange<F1>::type>(),
					std::declval<typename getrange<Fs>::type>()...))
		> baseT;

	OP op;
	TT fs;

	constexpr symop(const F1 &f1, const Fs &...fs) : fs(f1,fs...) {}
	constexpr symop(const OP &o, const F1 &f1, const Fs &...fs) : op(o), fs(f1,fs...) {}


	constexpr bool doisconst() const { return false; }

	// would like to you lambdas (see dosubst below), but they 
	// don't support constexpr yet, so here are simple lambdas
	// written out in "long hand"
	// begin silliness
	template<typename E>
	struct opder {
		const E &e;
		const symop<OP,F1,Fs...> &t;
		constexpr opder(const E &ee, const symop<OP,F1,Fs...> &tt)
				: e(ee), t(tt) {}
		template<typename... Ts>
		constexpr auto operator()(Ts &&...ts) {
			return opinfo<OP,F1,Fs...>::doderiv(e,t.op,
					std::forward<Ts>(ts)...);
		}
	};
	struct idfn {
		constexpr idfn() {}
		template<typename T>
		constexpr auto operator()(T &&t) const { return std::forward<T>(t); }
	};
	template<typename F>
	struct applyfn {
		const F &f;
		const OP &op;
		template<typename FF>
		constexpr applyfn(const OP &oopp, FF &&ff)
				: op(oopp), f(std::forward<FF>(ff)) {}
		template<typename... Ts>
		constexpr auto operator()(Ts &&...ts) const
			{ return f(op,std::forward<Ts>(ts)...); }
	};
	template<typename CF, typename LF>
	struct recurfold {
		const CF &cf;
		const LF &lf;
		constexpr recurfold(const CF &ccf, const LF &llf) : cf(ccf), lf(llf) {}
		template<typename T>
		constexpr auto operator()(T &&t) const
			{ return std::forward<T>(t).fold(cf,lf); }
	};
		
	// end silliness

	template<typename E>
	constexpr auto doderiv(const E &e) const
		{ return doopfn(opder<E>{e,*this},idfn{},
			//[e, this](auto... param) {
			//	return opinfo<OP,F1,Fs...>::doderiv(e,this->op,param...);
			//},
			//[](auto x) {
			//	return x;
			//},
			fs); }

	template<typename COMBF, typename LEAFF>
	constexpr auto dofold(COMBF &&cf, LEAFF &&lf) const {
		return doopfn(applyfn<typename std::decay<COMBF>::type>
							{op,cf},
				recurfold<typename std::decay<COMBF>::type,
						typename std::decay<LEAFF>::type>
					{cf,std::forward<LEAFF>(lf)},fs);
	}


	constexpr int doprecedence() const
		{ return opinfo<OP,F1,Fs...>::precedence; }

	template<std::size_t I>
	void printsubtree(std::ostream &os,
			int myprec=opinfo<OP,F1,Fs...>::precedence) const {
		std::get<I>(fs).print(os,myprec);
	}

	template<std::size_t I,
			typename std::enable_if<I+1==
				std::tuple_size<TT>::value>::type *EN=nullptr>
	void printallsubs(std::ostream &os,
		const char *between, int myprec=opinfo<OP,F1,Fs...>::precedence) const {
		if (I>0) os << between;
		printsubtree<I>(os,opinfo<OP,F1,Fs...>::rightassoc || std::tuple_size<TT>::value!=2 ? myprec : myprec-1);
	}

	template<std::size_t I,
			typename std::enable_if<I+1<
				std::tuple_size<TT>::value>::type *EN=nullptr>
	void printallsubs(std::ostream &os,
		const char *between, int myprec=opinfo<OP,F1,Fs...>::precedence) const {
		if (I>0) os << between;
		printsubtree<I>(os,opinfo<OP,F1,Fs...>::rightassoc && std::tuple_size<TT>::value==2 ? myprec-1 : myprec);
		printallsubs<I+1>(os,between,myprec);
	}
		
	void doprint(std::ostream &os,int parprec) const {
		if (parprec<opinfo<OP,F1,Fs...>::precedence) os << '(';
		if (std::tuple_size<TT>::value==0) //????
			os << opinfo<OP,F1,Fs...>::name;
		else {
			if (opinfo<OP,F1,Fs...>::prefix) {
				os << opinfo<OP,F1,Fs...>::name;
				printallsubs<0>(os," ");
			} else if (opinfo<OP,F1,Fs...>::isfunc) {
				os << opinfo<OP,F1,Fs...>::name;
				os << '(';
				printallsubs<0>(os,",",1000);
				os << ')';
			} else if (opinfo<OP,F1,Fs...>::infix) {
				printallsubs<0>(os,opinfo<OP,F1,Fs...>::name);
			} else {// postfix?
				printallsubs<0>(os," ");
				os << opinfo<OP,F1,Fs...>::name;
			}
		}
		if (parprec<opinfo<OP,F1,Fs...>::precedence) os << ')';
	}


};


// Here are the definitions of the operations (generic for any types):
// the ones in std:: (like std::plus) do not have constexpr operator()
// (they are supposed to in C++14, but clang and g++ do not yet have this)

struct symplus {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)+std::forward<T2>(t2))){
		return std::forward<T1>(t1)+std::forward<T2>(t2);
	}
};
struct symminus {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)-std::forward<T2>(t2))){
		return std::forward<T1>(t1)-std::forward<T2>(t2);
	}
};
struct symmultiplies {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)*std::forward<T2>(t2))){
		return std::forward<T1>(t1)*std::forward<T2>(t2);
	}
};
struct symdivides {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)/std::forward<T2>(t2))){
		return std::forward<T1>(t1)/std::forward<T2>(t2);
	}
};
struct symmodulus {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)%std::forward<T2>(t2))){
		return std::forward<T1>(t1)%std::forward<T2>(t2);
	}
};
// unary -
struct symnegate {
	template<typename T>
	constexpr auto operator()(T &&t) const
			noexcept(noexcept(-std::forward<T>(t))) {
		return -std::forward<T>(t);
	}
};
// unary +
struct symposite {
	template<typename T>
	constexpr auto operator()(T &&t) const
			noexcept(noexcept(-std::forward<T>(t))) {
		return +std::forward<T>(t);
	}
};

// and their associated information
// (commutes is set to 0 -- to be overridden in cases where it is
//  known to commute)
template<typename T1, typename T2>
struct opinfo<symplus,T1,T2> {
	enum {commutes = 0 };
	enum {precedence = 6 };
	enum {infix = 1 };
	enum {prefix= 0 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "+";

	template<typename E>
	constexpr static auto doderiv(const E &e, const symplus &,
			const T1 &left, const T2 &right)
		{ return left.d(e) + right.d(e); }
};
template<typename T1, typename T2>
struct opinfo<symminus,T1,T2> {
	enum {commutes = 0 };
	enum {precedence = 6 };
	enum {infix = 1 };
	enum {prefix= 0 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "-";

	template<typename E>
	constexpr static auto doderiv(const E &e, const symminus &,
			const T1 &left, const T2 &right)
		{ return left.d(e) - right.d(e); }
};
template<typename T1, typename T2>
struct opinfo<symmultiplies,T1,T2> {
	enum {commutes = 0 };
	enum {precedence = 5 };
	enum {infix = 1 };
	enum {prefix= 0 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "*";

	template<typename E>
	constexpr static auto doderiv(const E &e, const symmultiplies &,
			const T1 &left, const T2 &right)
		{ return left.d(e)*right + left*right.d(e); }
};
template<typename T1, typename T2>
struct opinfo<symdivides,T1,T2> {
	enum {commutes = 0 };
	enum {precedence = 5 };
	enum {infix = 1 };
	enum {prefix= 0 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "/";

	template<typename E>
	constexpr static auto doderiv(const E &e, const symdivides &,
			const T1 &left, const T2 &right)
		{ return left.d(e)/right - left*right.d(e)/(right*right); }
		//{ return (left.d(e)*right - left*right.d(e))/(right*right); }
};
template<typename T1, typename T2>
struct opinfo<symmodulus,T1,T2> {
	enum {commutes = 0 };
	enum {precedence = 5 };
	enum {infix = 1 };
	enum {prefix= 0 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "%";
};
template<typename T>
struct opinfo<symnegate,T> {
	enum {commutes = 0 };
	enum {precedence = 3 };
	enum {infix = 0 };
	enum {prefix= 1 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "-";

	template<typename E>
	constexpr static auto doderiv(const E &e, const symnegate &, const T &ex)
		{ return -ex.d(e); } 
};
template<typename T>
struct opinfo<symposite,T> {
	enum {commutes = 0 };
	enum {precedence = 3 };
	enum {infix = 0 };
	enum {prefix= 1 };
	enum {isfunc = 0 };
	enum {rightassoc = 0 };
	static constexpr char const *name = "+";

	template<typename E>
	constexpr static auto doderiv(const E &e, const symposite&, const T &ex)
		{ return +ex.d(e); } 
};

// a quick helper that "does" an "op"
template<typename OP, typename... Fs>
constexpr auto doop(Fs &&...fs) {
	return symop<OP,typename std::remove_reference<Fs>::type...>
				(std::forward<Fs>(fs)...);
}

// matching the C++ operators (+, -, etc) to the types above
// (and including the chance that one or more arguments might be
//  a constant -- but not all the arguments! :) )
//+:
template<typename F1, typename F2,
	typename std::enable_if<is_mathsym<F1>::value
				&& is_mathsym<F2>::value>::type *E=nullptr>
constexpr auto operator+(F1 &&f1, F2 &&f2) {
	return doop<symplus>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator+(F1 &&f1, const typename getrange<F1>::type &k) {
	return doop<symplus>(std::forward<F1>(f1),
			rtconstsym<typename getrange<F1>::type>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator+(const typename getrange<F1>::type &k, F1 &&f1) {
	return doop<symplus>(rtconstsym<typename getrange<F1>::type>(k),
			std::forward<F1>(f1));
}

//-:
template<typename F1, typename F2,
	typename std::enable_if<is_mathsym<F1>::value
				&& is_mathsym<F2>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1, F2 &&f2) {
	return doop<symminus>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1, const typename getrange<F1>::type &k) {
	return doop<symminus>(std::forward<F1>(f1),
			rtconstsym<typename getrange<F1>::type>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator-(const typename getrange<F1>::type &k, F1 &&f1) {
	return doop<symminus>(rtconstsym<typename getrange<F1>::type>(k),
			std::forward<F1>(f1));
}

//*:
template<typename F1, typename F2,
	typename std::enable_if<is_mathsym<F1>::value
				&& is_mathsym<F2>::value>::type *E=nullptr>
constexpr auto operator*(F1 &&f1, F2 &&f2) {
	return doop<symmultiplies>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator*(F1 &&f1, const typename getrange<F1>::type &k) {
	return doop<symmultiplies>(std::forward<F1>(f1),
			rtconstsym<typename getrange<F1>::type>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator*(const typename getrange<F1>::type &k, F1 &&f1) {
	return doop<symmultiplies>(rtconstsym<typename getrange<F1>::type>(k),
			std::forward<F1>(f1));
}

// /:
template<typename F1, typename F2,
	typename std::enable_if<is_mathsym<F1>::value
				&& is_mathsym<F2>::value>::type *E=nullptr>
constexpr auto operator/(F1 &&f1, F2 &&f2) {
	return doop<symdivides>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator/(F1 &&f1, const typename getrange<F1>::type &k) {
	return doop<symdivides>(std::forward<F1>(f1),
			rtconstsym<typename getrange<F1>::type>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator/(const typename getrange<F1>::type &k, F1 &&f1) {
	return doop<symdivides>(rtconstsym<typename getrange<F1>::type>(k),
			std::forward<F1>(f1));
}

// %:
template<typename F1, typename F2,
	typename std::enable_if<is_mathsym<F1>::value
				&& is_mathsym<F2>::value>::type *E=nullptr>
constexpr auto operator%(F1 &&f1, F2 &&f2) {
	return doop<symmodulus>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator%(F1 &&f1, const typename getrange<F1>::type &k) {
	return doop<symmodulus>(std::forward<F1>(f1),
			rtconstsym<typename getrange<F1>::type>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator%(const typename getrange<F1>::type &k, F1 &&f1) {
	return doop<symmodulus>(rtconstsym<typename getrange<F1>::type>(k),
			std::forward<F1>(f1));
}

// unary -
template<typename F1,
			typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1) {
	return doop<symnegate>(std::forward<F1>(f1));
}
// unary +
template<typename F1,
			typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator+(F1 &&f1) {
	return doop<symposite>(std::forward<F1>(f1));
}

// expr type checking:

template<typename E,typename En=void>
struct exprinfo {
	enum { possibleop = 1 }; 
	enum { possibleconst = 1 };
	enum { possiblesym = 1 };
	enum { definiteop = 0 }; 
	enum { definiteconst = 0 }; 
	enum { definitesym = 0 }; 
	enum { singleexpr = 0 };
};

template<typename E>
struct exprinfo<E, typename std::enable_if<
						std::is_base_of<absconstexpr,E>::value>::type> {
	enum { possibleop = 0 };
	enum { possibleconst = 1 };
	enum { possiblesym = 0 };
	enum { definiteop = 0 }; 
	enum { definiteconst = 1 }; 
	enum { definitesym = 0 }; 
	enum { singleexpr = 1 };
};

template<typename E>
struct exprinfo<E, typename std::enable_if<
						std::is_base_of<abssymexpr,E>::value>::type> {
	enum { possibleop = 0 };
	enum { possibleconst = 0 };
	enum { possiblesym = 1 };
	enum { definiteop = 0 }; 
	enum { definiteconst = 0 }; 
	enum { definitesym = 1 }; 
	enum { singleexpr = 1 };
};

template<typename E>
struct exprinfo<E, typename std::enable_if<
						std::is_base_of<absopexpr,E>::value>::type> {
	enum { possibleop = 1 };
	enum { possibleconst = 0 };
	enum { possiblesym = 0 };
	enum { definiteop = 1 }; 
	enum { definiteconst = 0 }; 
	enum { definitesym = 0 }; 
	enum { singleexpr = 1 };
};

template<typename T1, typename T2, typename... Ts>
struct exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T1,T2,Ts...>,void> {
	enum { possibleop = exprinfo<T1>::possibleop || exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T2,Ts...>>::possibleop };
	enum { possibleconst = exprinfo<T1>::possibleconst || exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T2,Ts...>>::possibleconst };
	enum { possiblesym = exprinfo<T1>::possiblesym || exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T2,Ts...>>::possiblesym };
	enum { definiteeop = exprinfo<T1>::definiteeop && exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T2,Ts...>>::definiteeop };
	enum { definiteeconst = exprinfo<T1>::definiteeconst && exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T2,Ts...>>::definiteeconst };
	enum { definiteesym = exprinfo<T1>::definiteesym && exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T2,Ts...>>::definiteesym };
	enum { singleexpr = 0 };
};

template<typename T1>
struct exprinfo<commonunion::mathexprunion_node_impl::cu_impl<void,T1>,void> {
	enum { possibleop = exprinfo<T1>::possibleop };
	enum { possibleconst = exprinfo<T1>::possibleconst };
	enum { possiblesym = exprinfo<T1>::possiblesym };
	enum { definiteop = exprinfo<T1>::definiteop };
	enum { definiteconst = exprinfo<T1>::definiteconst };
	enum { definitesym = exprinfo<T1>::definitesym };
	enum { singleexpr = 1 };
};

// simplify:

/*
template<std::size_t I>
struct simplifyrule {
	template<typename E,
		typename std::enable_if<!exprinfo<E>::possibleop>::type *EN=nullptr>
	static E operator()(const E &e) { return e; }
	template<typename E,
		typename std::enable_if<exprinfo<E>::possibleop>::type *EN=nullptr>
	static E operator()(const E &e) { return e; }
};

template<>struct simplifyrule<0> {
	template<typename E>
	static E 

template<typename E, typename std::enable_if<is_mathsym<E>::value>::type *EN=nullptr>
constexpr auto simplify(E &&e) { 
	return simplifyrule<0>::(std::forward<E>(e));
}

*/

#endif
