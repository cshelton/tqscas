#ifndef SYMMATH_H
#define SYMMATH_H

#include <utility>
#include <type_traits>
#include <cmath>
#include <stdexcept>
#include "value_ptr.h"
#include <string>
#include <cstring>
#include <iostream>

// some predefinition declarations
template<typename RANGE> struct rtconstsym;
template<typename RANGE> struct ctconstsymzero;
template<typename RANGE> struct ctconstsymidentity;
struct typelessassign;
struct typelesspseudoexpr;
template<typename RANGE> struct abspseudoexpr;
template<typename RANGE> struct rtmathexpr;
template<typename IT> struct pseudoexpr;
template<typename DERIV, typename RANGE> struct sym;
template<typename LHS, typename RHS> struct assignexpr;
template<typename A1,typename A2> struct assignpair;
template<typename T1, typename T2, typename EN=void> struct derivativetype {};
template<typename T,typename EN=void> struct typebasics {};

using typelesspseudoptr = value_ptr<typelesspseudoexpr,default_clone<typelesspseudoexpr>>;
// a pointer to an "pseudo" expr with type RANGE
template<typename RANGE>
using pseudoptr = value_ptr<abspseudoexpr<RANGE>,default_clone<abspseudoexpr<RANGE>>>;

// some TMP constructs to allow detection of math symbols 
//  and compile-time math symbols (latter for double dispatch)
template<typename T>
struct makevoid {
	typedef void type;
};

template<typename T, typename R=void>
struct is_mathsym {
	enum {value = 0 };
};

template<typename T>
struct is_mathsym<T,typename makevoid<typename std::remove_reference<T>::type::range>::type> {
	enum {value = 1 };
};

template<typename T, typename R=void>
struct is_rtmathsym {
	enum {value = 0 };
};

template<typename T>
struct is_rtmathsym<T,typename makevoid<typename std::remove_reference<T>::type::rtrange>::type> {
	enum {value= 1};
};

template<typename T>
struct has_deriv {
private:
	template<typename S>
	static constexpr auto check(S*)
		-> typename std::enable_if<
				std::is_object<decltype(std::declval<S>().
						doderiv(std::declval<typelesspseudoptr>()))>
				::value,
				std::true_type>::type {
			return std::true_type{}; }

	template<typename>
	static constexpr std::false_type check(...) { return std::false_type{}; }

	typedef decltype(check<T>(nullptr)) type;
public:
	static constexpr bool value = type::value;
};

template<typename T>
struct getrange {
	typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type B;
	typedef typename B::range type;
};


// mathexpr is the base type for all types that represent
// a symbolic math expression (and know their type)
// DERIV = derived type (see CRTP) [not "deriviative"]
//
// the derived class must implement
// * dosubst (on both assignexpr and typelessassign)
// * doderiv (for any is_mathsym expression)
template<typename DERIV,typename RANGE>
struct mathexpr {
	typedef RANGE range;

	constexpr const DERIV &dclassref() const
		{ return static_cast<const DERIV &>(*this); }
	DERIV *dclassref()
		{ return static_cast<DERIV &>(*this); }

	constexpr auto val() const { return dclassref().val(); }

	template<typename V, typename E>
	constexpr auto operator[](const assignexpr<V,E> &a) const
		{ return dclassref().dosubst(a); }

	template<typename A1, typename A2>
	constexpr auto operator[](const assignpair<A1,A2> &a) const {
		return (*this)[a.first][a.second];
	}

	auto operator[](const typelessassign &a) const
		{ return dclassref().dosubst(a); }

	constexpr int precedence() const { return 0; }
	
	// derivative 
	template<typename E,
		typename std::enable_if<is_mathsym<E>::value>::type *EN=nullptr>
	constexpr auto d(const E &x) const
		{ return dclassref().doderiv(x); }

};


// this is the base type for pseudo-expressions that are only
// known at runtime.  This base doesn't even know its type
// (but abspseudoexpr below does).
// Why "pseudo-"???  Well, this is not actually a mathexpr
// Rather, a pointer to it will be wrapped in a mathexpr
// (see pseudoexpr below for the derived pseudo-expr
//  and rtmathexpr for the wrapper into a mathexpr)
struct typelesspseudoexpr {
	typedef void rtrange;

	virtual ~typelesspseudoexpr() = default;
	virtual typelesspseudoexpr *clone() const = 0;
	virtual void print(std::ostream &os) const = 0;

	template<typename R>
	const abspseudoexpr<R> &withtype() const {
		// exception thrown if not of this type
		return dynamic_cast<const abspseudoexpr<R> &>(*this);
	}

	template<typename E>
	const E &as() const {
		// exception thrown if not of this type
		return dynamic_cast<const pseudoexpr<E> &>(*this).impl;
	}

	virtual int precedence() const { return 0; }

	virtual typelesspseudoptr doderiv(const typelesspseudoptr &e) const = 0;
};


// abstract compile-type expression that knows its type
template<typename RANGE>
struct abspseudoexpr : typelesspseudoexpr {
	typedef RANGE range;
	typedef RANGE rtrange;

	virtual ~abspseudoexpr<RANGE>() = default;

	virtual abspseudoexpr *clone() const = 0;

	virtual RANGE val() const = 0;
	virtual pseudoptr<RANGE> subst(const typelessassign &a) const = 0;

	template<typename V, typename E>
	pseudoptr<RANGE> subst(const assignexpr<V,E> &a)
		{ return this->subst(typelessassign{a}); }

	template<typename E>
	constexpr
	pseudoptr<typename derivativetype<RANGE,E>::type>
	doderiv(const E &e) const
		{ return this->doderiv(typelesspseudoptr(topseudoptr(e))); }

	virtual void print(std::ostream &os) const = 0;

};


// a concrete pseudo expression.  This has virtual methods and
// just wraps a mathexpr (type IT)
// it is *not* a mathexpr.  A pointer to it must be wrapped... see below
//
// NOTE: many of the methods have same or similar names to those
// in a real mathexpr.  However, they are different.  Note that they
// return pseudoptr types, and *NOT* mathexpr!
// These get wrapped correctly by rtmathexpr (see below)
template<typename IT>
struct pseudoexpr : public abspseudoexpr<typename IT::range> {
	typedef typename IT::range range;
	IT impl;
	template<typename T>
	constexpr pseudoexpr(T &&t) : impl(std::forward<T>(t)) {}

	virtual pseudoexpr<IT> *clone() const
		{ return new pseudoexpr<IT>(*this); }
	
	virtual range val() const
		{ return impl.val(); }

	virtual pseudoptr<typename IT::range> subst(const typelessassign &a) const
		{ return topseudoptr(impl[a]); }

	virtual int precedence() const
		{ return impl.precedence(); }

	virtual void print(std::ostream &os) const
		{ impl.print(os); }

	virtual typelesspseudoptr doderiv(const typelesspseudoptr &e) const 
		{ return mydoderiv<IT>(e); }

	template<typename T>
	constexpr
	typename std::enable_if<has_deriv<T>::value,typelesspseudoptr>::type
	mydoderiv(const typelesspseudoptr &e) const
		{ return topseudoptr(impl.doderiv(e)); }

	template<typename T>
	constexpr
	typename std::enable_if<!has_deriv<T>::value,typelesspseudoptr>::type
	mydoderiv(const typelesspseudoptr &e) const
		{ throw std::logic_error("derivative does not exist for this type"); }
};


// conversions from different expressions, pseudo-expressions
//  and ptrs to pseduo-expression into pseudoptr (ptr to pseudo-expression)
template<typename E>
pseudoptr<typename E::range> topseudoptr(const E &e) {
	return pseudoptr<typename E::range>{new pseudoexpr<E>(e)};
}
template<typename E>
pseudoptr<typename E::range> topseudoptr(E &&e) {
	return pseudoptr<typename E::range>{new pseudoexpr<E>(std::move(e))};
}
template<typename R>
pseudoptr<R> topseudoptr(const abspseudoexpr<R> &e) {
	return pseudoptr<R>{e.clone()};
}
template<typename R>
pseudoptr<R> topseudoptr(abspseudoexpr<R> &&e) {
	return pseudoptr<R>{e.clone()}; // better way?  I haven't found one
}
template<typename R>
pseudoptr<R> topseudoptr(const rtmathexpr<R> &e) {
	return e.impl;
}
template<typename R>
pseudoptr<R> topseudoptr(rtmathexpr<R> &&e) {
	return std::move(e.impl);
}
template<typename R>
pseudoptr<R> topseudoptr(const pseudoptr<R> &e) {
	return e;
}
template<typename R>
pseudoptr<R> topseudoptr(pseudoptr<R> &&e) {
	return std::move(e);
}



		
// symbolic math expression that knows its type, but nothing else
// it is fully known at "rt" (runtime)
// implemented as a pointer to a pseudo-expression, which uses dynamic dispatch
template<typename RANGE>
struct rtmathexpr : mathexpr<rtmathexpr<RANGE>,RANGE> {
	pseudoptr<RANGE> impl;

	template<typename T>
	rtmathexpr(T &&t) : impl{topseudoptr(std::forward<T>(t))} {}

	constexpr auto val() const { return impl->val(); }

	template<typename E1, typename E2>
	constexpr auto dosubst(const assignexpr<E1,E2> &a) const
		{ return rtmathexpr<RANGE>{impl->subst(a)}; }

	constexpr int precedence() const { return impl->precedence(); }

	void print(std::ostream &os) const { impl->print(os); }

	template<typename E>
	constexpr auto doderiv(const E &x) const 
		{ return rtmathexpr<typename derivativetype<RANGE,E>::type>
				{impl->doderiv(x)}; }
};

// assign expression (which consists just of a pair of symbolic expressions:
template<typename LHS, typename RHS>
struct assignexpr {
	static_assert(std::is_same<typename getrange<LHS>::type, typename getrange<RHS>::type>::value,"assignment doesn't preserve type");

	typedef typename getrange<LHS>::type range;

	typename getrange<LHS>::B lhs;
	typename getrange<RHS>::B rhs;
	template<typename T1, typename T2>
	constexpr assignexpr(T1 &&l, T2 &&r) : lhs(std::forward<T1>(l)),
			rhs(std::forward<T2>(r)) {}

	void print(std::ostream &os) const {
		lhs.print(os); os << " <- "; rhs.print(os);
	}
};

// here is the same thing, but when the type of the expressions involved
// is not known at compile time -- this is treated slightly separately
// than other compile-time deductions because others require just
// double-dispatch, but this would require triple dispatch, so we just
// implement the specifics necessary)
struct typelessassign {
	typelesspseudoptr lhs,rhs;
	template<typename V, typename E>
	typelessassign(const assignexpr<V,E> &a) :
			lhs(topseudoptr(a.lhs)) , rhs(topseudoptr(a.rhs)) {}

	
	void print(std::ostream &os) const {
		lhs->print(os); os << " <- "; rhs->print(os);
	}
};

// use == to build assignexpr:
template<typename E1, typename E2, 
		typename std::enable_if<is_mathsym<E1>::value && is_mathsym<E2>::value>::type *E=nullptr>
constexpr auto operator==(E1 &&e1, E2 &&e2) {
	return assignexpr<
	typename std::remove_cv<typename std::remove_reference<E1>::type>::type,
	typename std::remove_cv<typename std::remove_reference<E2>::type>::type>
		{std::forward<E1>(e1),std::forward<E2>(e2)};
}

// use == to build assignexpr (when rhs is a constant)
template<typename E1, typename std::enable_if<is_mathsym<E1>::value>::type *E=nullptr>
constexpr assignexpr<typename std::remove_cv<typename std::remove_reference<E1>::type>::type,rtconstsym<typename getrange<E1>::type>>
operator==(E1 &&e1, const typename getrange<E1>::type &e2) {
	return assignexpr<
	typename std::remove_cv<typename std::remove_reference<E1>::type>::type,
	rtconstsym<typename getrange<E1>::type>>
		{std::forward<E1>(e1),rtconstsym<typename getrange<E1>::type>{e2}};
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

// a constant
template<typename DERIV, typename RANGE>
struct constsym : public mathexpr<DERIV,RANGE> {
	void print(std::ostream &os) const { os << this->dclassref().val(); }

	template<typename V, typename E>
	constexpr DERIV dosubst(const assignexpr<V,E> &) const
		{ return this->dclassref(); }

	constexpr DERIV dosubst(const typelessassign &) const
		{ return this->dclassref(); }

	template<typename E, typename derivativetype<RANGE,E>::type *EN=nullptr>
	constexpr
	ctconstsymzero<typename derivativetype<RANGE,E>::type>
	doderiv(const E &) const { return {}; };
};

// special case of "zero"
template<typename RANGE>
struct ctconstsymzero : public constsym<ctconstsymzero<RANGE>,RANGE> {
	constexpr auto val() const { return typebasics<RANGE>::zero(); }
};

// special case of "one"
template<typename RANGE>
struct ctconstsymidentity : public constsym<ctconstsymidentity<RANGE>,RANGE> {
	constexpr auto val() const { return typebasics<RANGE>::identity(); }
};

// at compile time:
template<typename RANGE, RANGE value>
struct ctconstsym : public constsym<ctconstsym<RANGE,value>,RANGE> {
	static_assert(std::is_integral<RANGE>::value,"compile-time constants only valid for integral types -- at least until C++17");
	constexpr auto val() const { return value; }
};

// at runtime:
template<typename RANGE>
struct rtconstsym : public constsym<rtconstsym<RANGE>,RANGE> {
	constexpr rtconstsym(const RANGE &r) : k(r) {}
	constexpr rtconstsym(RANGE &&r) : k(std::move(r)) {}
	constexpr auto val() const { return k; }

	RANGE k;
};

// a symbol (like "x"), still abstract as there are a few ways of
// specifying the name of the symbol
template<typename DERIV, typename RANGE>
struct sym : public mathexpr<DERIV,RANGE> {

	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const {
		return this->dclassref().samesym(s);
	}

	bool samesym(const typelesspseudoptr &s) const {
		return this->dclassref().samesym(s);
	}

	constexpr RANGE val() const {
		return false ? RANGE{0} : throw std::logic_error("symbol has no value");
	}

	constexpr rtmathexpr<RANGE> dosubst(const typelessassign &a) const {
		return samesym(a.lhs)
			? rtmathexpr<RANGE>(a.rhs->withtype<RANGE>())
			: rtmathexpr<RANGE>(this->dclassref());
	}

	template<typename E>
	constexpr
	rtmathexpr<typename derivativetype<RANGE,E>::type>
	doderiv(const E &e) const { 
		typedef typename derivativetype<RANGE,E>::type R;
		return samesym(e) ? rtmathexpr<R>(ctconstsymidentity<R>())
						: rtmathexpr<R>(ctconstsymzero<R>());
	}
};

// code to convert a variadic template of characters into a string:
template<char...Ns> struct chartostr;
template<char N0, char...Ns>
struct chartostr<N0,Ns...> {
	std::string exec() const {
		return chartostr<Ns...>().exec()+N0;
	}
};
template<>
struct chartostr<> {
	std::string exec() const {
		return std::string{};
	}
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
	
	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const
		{ return false; }

	constexpr bool samesym(const sym<mytype,RANGE> &s) const
		{ return true; }

	bool samesym(const typelesspseudoptr &s) const {
		try {
			const mytype &l = s->as<mytype>();
			return true;
		} catch(const std::bad_cast &) {
			return false;
		}
	}

	using sym<staticsym<RANGE,N...>,RANGE>::dosubst;

	template<typename V, typename E>
	constexpr mytype dosubst(const assignexpr<V,E> &) const
		{ return *this; }

	template<typename E>
	constexpr E dosubst(const assignexpr<mytype,E> &a) const
		{ return a.rhs; }

	template<typename E>
	constexpr auto doderiv(const E &) const
		{ return ctconstsymzero<RANGE>{}; }

	template<typename E>
	constexpr auto doderiv(const mytype &) const
		{ return ctconstsymidentity<RANGE>{}; }


	constexpr RANGE val() const {
		return false ? typebasics<RANGE>::zero() : throw std::logic_error(std::string("symbol ")+chartostr<N...>().exec()+" is unassigned");
	}

	void print(std::ostream &os) const { os << chartostr<N...>().exec(); }

};


// symbol whose name is set at compile time
// for instance: dynsym<double> blah("x2");
// probably better written as dynsym<double> x2("x2");
//  (to avoid confusion)
template<typename RANGE>
struct dynsym : public sym<dynsym<RANGE>,RANGE> { 
	const char *n;
	constexpr dynsym(const char *name) : n(name) {}

	typedef dynsym<RANGE> mytype;
	
	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const
		{ return false; }

	constexpr bool samesym(const mytype &s) const
		{ return n==s.n; }

	bool samesym(const typelesspseudoptr &s) const {
		try {
			const mytype &l = s->as<mytype>();
			return !strcmp(l.n,n);
		} catch(const std::bad_cast &) {
			return false;
		}
	}

	using sym<dynsym<RANGE>,RANGE>::dosubst;

	template<typename V, typename E,
		typename std::enable_if<!std::is_same<V,mytype>::value>::type *EN=nullptr>
	constexpr mytype dosubst(const assignexpr<V,E> &) const
		{ return *this; }

	template<typename E>
	constexpr rtmathexpr<RANGE> dosubst
			(const assignexpr<mytype,mathexpr<E,RANGE>> &a) const {
		return (!strcmp(a.lhs.n,n)) ? rtmathexpr<RANGE>(a.rhs)
				: rtmathexpr<RANGE>(*this);
	}

	constexpr RANGE val() const {
		return false ? RANGE{0} : throw std::logic_error(std::string("symbol ")+n+" is unassigned");
	}

	void print(std::ostream &os) const { os << n; }
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
template<typename O, typename... Fs, std::size_t... I>
constexpr auto doval_help(const O &o, const std::tuple<Fs...> &fs, 
			std::index_sequence<I...>) {
	return o((std::get<I>(fs)).val()...);
}
template<typename O, typename... Fs>
constexpr auto doval(const O &o, const std::tuple<Fs...> &fs) {
	return doval_help(o,fs,std::index_sequence_for<Fs...>{});
}

// same for invoking assign:
template<typename O, typename A, typename... Fs, std::size_t... I>
constexpr auto doassign_help(const A &a, const O &o, const std::tuple<Fs...> &fs, 
			std::index_sequence<I...>) {
	return o((std::get<I>(fs))[a]...);
}
template<typename O, typename A, typename... Fs>
constexpr auto doassign(const A &a, const O &o, const std::tuple<Fs...> &fs) {
	return doassign_help(a,o,fs,std::index_sequence_for<Fs...>{});
}

// same for invoking doderiv:
template<typename O, typename E, typename... Fs, std::size_t... I>
constexpr auto doopderiv_help(const E &e, const O &o, const std::tuple<Fs...> &fs, 
			std::index_sequence<I...>) {
	return opinfo<O,Fs...>::doderiv(e,o,std::get<I>(fs)...);
}
template<typename O, typename E, typename... Fs>
constexpr auto doopderiv(const E &e, const O &o, const std::tuple<Fs...> &fs) {
	return doopderiv_help(e,o,fs,std::index_sequence_for<Fs...>{});
}


// quick check to see if a type T has a method named "print"
// that returns void and takes a stream *and* an integer
// based on stackoverflow Q#87372, answer 16824239 (but simpler for this case)
template<typename T>
struct print_takes_prec {
private:
	template<typename S>
	static constexpr auto check(S*)
		-> typename std::enable_if<
				std::is_same<decltype(std::declval<S>().print(
					std::declval<std::ostream &>(),
					std::declval<int>())),void>::value,
				std::true_type>::type {
			return std::true_type{}; }

	template<typename>
	static constexpr std::false_type check(...) { return std::false_type{}; }

	typedef decltype(check<T>(nullptr)) type;
public:
	static constexpr bool value = type::value;
};

// An expression representing an operation (OP) on types F1 through Fn
template<typename OP, typename F1, typename... Fs>
struct symop : public mathexpr<symop<OP,F1,Fs...>, decltype(doval(std::declval<OP>(),std::declval<std::tuple<F1,Fs...>>()))>{
	typedef std::tuple<F1,Fs...> TT;
	typedef mathexpr<symop<OP,F1,Fs...>, decltype(doval(std::declval<OP>(),std::declval<std::tuple<F1,Fs...>>()))> baseT;

	OP op;
	TT fs;

	constexpr symop(const F1 &f1, const Fs &...fs) : fs(f1,fs...) {}
	constexpr symop(const OP &o, const F1 &f1, const Fs &...fs) : op(o), fs(f1,fs...) {}

	constexpr typename baseT::range val() const {
		return doval(op,fs);
	}

	template<typename E1, typename E2>
	constexpr auto dosubst(const assignexpr<E1,E2> &a) const {
		return doassign(a,op,fs);
	}

	constexpr int precedence() const
		{ return opinfo<OP,F1,Fs...>::precedence; }

	template<std::size_t I,
		typename std::enable_if<print_takes_prec<typename std::tuple_element<I,TT>::type>::value>::type *EN=nullptr>
	void printsubtree(std::ostream &os,
			int myprec=opinfo<OP,F1,Fs...>::precedence) const {
		std::get<I>(fs).print(os,myprec);
	}

	template<std::size_t I,
		typename std::enable_if<!print_takes_prec<typename std::tuple_element<I,TT>::type>::value>::type *EN=nullptr>
	void printsubtree(std::ostream &os,
			int myprec=opinfo<OP,F1,Fs...>::precedence) const {
		std::get<I>(fs).print(os);
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
		
	void print(std::ostream &os,int parprec=1000) const {
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

	template<typename E>
	constexpr auto doderiv(const E &e) const
		{ return doopderiv(e,op,fs); }
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
constexpr auto operator+(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<symplus>(std::forward<F1>(f1),
			rtconstsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator+(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symplus>(rtconstsym<typename std::remove_reference<F1>::type::range>(k),
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
constexpr auto operator-(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<symminus>(std::forward<F1>(f1),
			rtconstsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator-(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symminus>(rtconstsym<typename std::remove_reference<F1>::type::range>(k),
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
constexpr auto operator*(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<symmultiplies>(std::forward<F1>(f1),
			rtconstsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator*(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symmultiplies>(rtconstsym<typename std::remove_reference<F1>::type::range>(k),
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
constexpr auto operator/(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<symdivides>(std::forward<F1>(f1),
			rtconstsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator/(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symdivides>(rtconstsym<typename std::remove_reference<F1>::type::range>(k),
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
constexpr auto operator%(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<symmodulus>(std::forward<F1>(f1),
			rtconstsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator%(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symmodulus>(rtconstsym<typename std::remove_reference<F1>::type::range>(k),
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

// derivative rules & type information

template<typename T>
struct typebasics<T,
	typename std::enable_if<std::is_arithmetic<T>::value>::type> {
	static constexpr T identity() { return {1}; }
	static constexpr T zero() { return {0}; }

	enum { simplifyadd0 = 1 };
	enum { simplifymult1 = 1 };
};

template<typename T>
struct derivativetype<T,T,
		typename std::enable_if<std::is_floating_point<T>::value>::type> {
	typedef T type;
};

template<typename T>
struct derivativetype<T,T,
		typename std::enable_if<std::is_integral<T>::value>::type> {
	typedef T type; // maybe??
};

template<typename T1,typename E>
struct derivativetype<T1,E,typename makevoid<typename E::range>::type> {
	typedef typename derivativetype<T1,typename E::range>::type type;
};

template<typename T1,typename T2>
struct derivativetype<T1,pseudoptr<T2>,void> {
	typedef typename derivativetype<T1,T2>::type type;
};

// simplify:

template<typename E>
struct simpstruct {
	constexpr static E exec(const E &e) { return e; }
};

template<typename E>
constexpr auto simplify(const E &e) { return simpstruct<E>::exec(e); }

template<typename OP, typename... Es, std::size_t... I>
constexpr auto simplify_help(const std::tuple<Es...> &tup, std::index_sequence<I...>) {
	return doop<OP>(simplify(std::get<I>(tup))...);
}

template<typename OP, typename E1, typename... Es>
struct simpstruct<symop<OP,E1,Es...>> {
	constexpr static auto
	exec(const symop<OP,E1,Es...> &e)
		{ return simplify_help<OP>(e.fs,std::index_sequence_for<E1,Es...>{}); }
};

template<typename T, typename E>
struct simpstruct<symop<symmultiplies,E,ctconstsymzero<T>>> {
	constexpr static auto
	exec(const symop<symmultiplies,E,ctconstsymzero<T>> &)
		{ return ctconstsymzero<T>{}; }
};

template<typename T, typename E>
struct simpstruct<symop<symmultiplies,ctconstsymzero<T>,E>> {
	constexpr static auto
	exec(const symop<symmultiplies,ctconstsymzero<T>,E> &)
		{ return ctconstsymzero<T>{}; }
};

/*
template<typename T1,typename T2>
constexpr auto simplify(const symop<symmultiplies,T1,T2> &)
	{ return ctconstsymzero<typename T1::range>{}; }
*/

#endif
