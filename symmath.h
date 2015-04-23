#ifndef SYMMATH_H
#define SYMMATH_H

#include <utility>
#include <type_traits>
#include <cmath>
#include <stdexcept>
#include "value_ptr.h"

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

template<typename T>
struct getrange {
	typedef typename std::remove_cv<typename std::remove_reference<T>::type>::type B;
	typedef typename B::range type;
};

template<typename LHS, typename RHS>
struct assignexpr {
	static_assert(std::is_same<typename getrange<LHS>::type, typename getrange<RHS>::type>::value,"assignment doesn't preserve type");

	typedef typename getrange<LHS>::type range;

	typename getrange<LHS>::B lhs;
	typename getrange<RHS>::B rhs;
	template<typename T1, typename T2>
	constexpr assignexpr(T1 &&l, T2 &&r) : lhs(std::forward<T1>(l)),
			rhs(std::forward<T2>(r)) {}
};

template<typename E1, typename E2, 
		typename std::enable_if<is_mathsym<E1>::value && is_mathsym<E2>::value>::type *E=nullptr>
constexpr assignexpr<E1,E2> operator==(E1 &&e1, E2 &&e2) {
	return assignexpr<
	typename std::remove_cv<typename std::remove_reference<E1>::type>::type,
	typename std::remove_cv<typename std::remove_reference<E2>::type>::type>
		{std::forward<E1>(e1),std::forward<E2>(e2)};
}

template<typename RANGE>
struct constsym;

template<typename E1, typename std::enable_if<is_mathsym<E1>::value>::type *E=nullptr>
constexpr assignexpr<typename std::remove_cv<typename std::remove_reference<E1>::type>::type,constsym<typename getrange<E1>::type>>
operator==(E1 &&e1, const typename getrange<E1>::type &e2) {
	return assignexpr<
	typename std::remove_cv<typename std::remove_reference<E1>::type>::type,
	constsym<typename getrange<E1>::type>>
		{std::forward<E1>(e1),constsym<typename getrange<E1>::type>{e2}};
}

template<typename... As>
using assigntup = std::tuple<As...>;


// std::tie should be constexpr.  However, in clang it is not yet
// (and g++ is even worse on constexpr methods for tuples)
// (this one operates a little bit differently, so that it works with below)
// Cannot replace directly with std::tie at this point!
template<typename... Ts>
constexpr auto consttie(Ts &&...ts) noexcept {
	return std::tuple<typename std::remove_reference<typename std::remove_cv<Ts>::type>::type &...>(ts...);
}

template<typename E1, typename E2, typename As>
constexpr auto
operator&(const assignexpr<E1,E2> &a1, As &&as) {
	return std::tuple_cat(consttie(a1),std::forward<As>(as));
}

template<typename E1, typename E2, typename As>
constexpr auto
operator&(assignexpr<E1,E2> &&a1, As &&as) {
	return std::tuple_cat(consttie(std::move(a1)),std::forward<As>(as));
}

template<typename E1, typename E2, typename As>
constexpr auto
operator&(As &&as, const assignexpr<E1,E2> &a1) {
	return std::tuple_cat(std::forward<As>(as),consttie(a1));
}

template<typename E1, typename E2, typename As>
constexpr auto
operator&(As &&as, assignexpr<E1,E2> &&a1) {
	return std::tuple_cat(std::forward<As>(as),consttie(std::move(a1)));
}

template<typename E1, typename E2, typename E3, typename E4>
constexpr assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>
operator&(const assignexpr<E1,E2> &a1, const assignexpr<E3,E4> &a2) {
	return assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>{a1,a2};
}
template<typename E1, typename E2, typename E3, typename E4>
constexpr assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>
operator&(assignexpr<E1,E2> &&a1, const assignexpr<E3,E4> &a2) {
	return assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>{std::move(a1),a2};
}
template<typename E1, typename E2, typename E3, typename E4>
constexpr assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>
operator&(const assignexpr<E1,E2> &a1, assignexpr<E3,E4> &&a2) {
	return assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>{a1,std::move(a2)};
}
template<typename E1, typename E2, typename E3, typename E4>
constexpr assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>
operator&(assignexpr<E1,E2> &&a1, assignexpr<E3,E4> &&a2) {
	return assigntup<assignexpr<E1,E2>,assignexpr<E3,E4>>
			{std::move(a1),std::move(a2)};
}

struct typelessassign;

template<std::size_t I> // I is number *left*
struct applyassigntup {
	template<typename E, typename... As>
	constexpr auto exec(const E &e, const assigntup<As...> &a) {
		return applyassigntup<I-1>{}.exec
			(e[std::get<std::tuple_size<assigntup<As...>>::value-(I+1)>(a)],a);
	}
};

template<>
struct applyassigntup<0> {
	template<typename E, typename... As>
	constexpr auto exec(const E &e, const assigntup<As...> &a) {
		return e[std::get<std::tuple_size<assigntup<As...>>::value-1>(a)];
	}
};
		
template<typename DERIV,typename RANGE>
struct mathexpr {
	typedef RANGE range;

	constexpr const DERIV *deriv() const { return (const DERIV *)(this); }
	DERIV *deriv() { return (const DERIV *)(this); }

	constexpr auto val() const { return deriv()->val(); }

	template<typename V, typename E>
	constexpr auto operator[](const assignexpr<V,E> &a) const
		{ return deriv()->dosubst(a); }

	template<typename... As>
	constexpr auto operator[](const assigntup<As...> &as) const {
		return applyassigntup<std::tuple_size<assigntup<As...>>::value-1>()
					.exec(*this,as);
	}

	auto operator[](const typelessassign &a) const
		{ return deriv()->dosubst(a); }
};

template<typename RANGE> struct absctexpr;

template<typename RANGE> struct ctmathexpr;

template<typename IT> struct ctexprimpl;

struct typelessabsexpr {
	virtual ~typelessabsexpr() = 0;
	virtual typelessabsexpr *clone() const = 0;

	template<typename R>
	const absctexpr<R> &withtype() const {
		// exception thrown if not of this type
		return *(dynamic_cast<const absctexpr<R> *>(*this));
	}

	template<typename E>
	const E &as() const {
		// exception thrown if not of this type
		return dynamic_cast<const ctexprimpl<E> *>(*this)->impl;
	}
};

template<typename RANGE>
using absctptr = value_ptr<absctexpr<RANGE>>;

template<typename RANGE>
struct absctexpr : typelessabsexpr {
	virtual ~absctexpr<RANGE>() = default;

	virtual absctexpr *clone() const = 0;

	virtual RANGE val() const = 0;
	virtual absctexpr<RANGE> *subst(const typelessassign &a) const = 0;

	template<typename V, typename E>
	absctptr<RANGE> subst(const assignexpr<V,E> &a) {
		return this->subst(typelessassign{a});
	}
};


template<typename E>
absctptr<typename E::range> toabsct(const E &e) {
	return new ctexprimpl<E>(e);
}
template<typename E>
absctptr<typename E::range> toabsct(E &&e) {
	return new ctexprimpl<E>(std::move(e));
}

template<typename R>
absctptr<R> toabsct(const ctmathexpr<R> &e) {
	return e.impl;
}

template<typename R>
absctptr<R> toabsct(ctmathexpr<R> &&e) {
	return std::move(e.impl);
}

template<typename IT>
struct ctexprimpl : public absctexpr<typename IT::range> {
	typedef typename IT::range range;
	IT impl;
	template<typename T>
	constexpr ctexprimpl(T &&t) : impl(std::forward<T>(t)) {}

	virtual ctexprimpl<IT> *clone() const { return new ctexprimpl<IT>(*this); }
	
	virtual range val() const { return impl.val(); }

	virtual absctptr<range> subst(const typelessassign &a) const {
		return toct(impl[a]);
	}
};

		
template<typename RANGE>
struct ctmathexpr : mathexpr<ctmathexpr<RANGE>,RANGE> {
	absctptr<RANGE> impl;

	template<typename T>
	ctmathexpr(const T &t) : impl{toabsct(t)} {}
	ctmathexpr(const absctptr<RANGE> &i) : impl(i) {}

	constexpr auto val() const { return impl->val(); }

	template<typename E1, typename E2>
	constexpr auto dosubst(const assignexpr<E1,E2> &a) const
		{ return impl->subst(a); }
};

struct typelessassign {
	value_ptr<typelessabsexpr> lhs,rhs;
	template<typename V, typename E>
	typelessassign(const assignexpr<V,E> &a) :
			lhs(toabsct(a.lhs)) , rhs(toabsct(a.rhs)) {}

		
};


//-----------------------------------------------------------

template<typename RANGE>
struct constsym : public mathexpr<constsym<RANGE>,RANGE> {
	RANGE k;

	constexpr constsym(const RANGE &r) : k(r) {}
	constexpr constsym(RANGE &&r) : k(std::move(r)) {}

	constexpr auto val() const { return k; }

	template<typename V, typename E>
	constexpr constsym<RANGE> dosubst(const assignexpr<V,E> &) const
		{ return *this; }

	constsym<RANGE> dosubst(const typelessassign &) const
		{ return *this; }
};

template<typename DERIV, typename RANGE>
struct sym : public mathexpr<DERIV,RANGE> {

	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const {
		return this->deriv()->samesym(s);
	}

	constexpr RANGE val() const {
		return false ? RANGE{0} : throw std::logic_error("symbol has no value");
	}

};

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

	template<typename V, typename E>
	constexpr mytype dosubst(const assignexpr<V,E> &) const
		{ return *this; }

	template<typename E>
	constexpr E dosubst(const assignexpr<mytype,E> &a) const
		{ return a.rhs; }

	ctmathexpr<RANGE> dosubst(const typelessassign &a) const {
		try {
			const mytype &l = a.lhs->as<mytype>(); // as a check...
			return ctmathexpr<RANGE>(a.rhs->withtype<RANGE>());
		} catch(const std::bad_cast &) {
			return ctmathexpr<RANGE>(*this);
		}
	}

};

template<typename RANGE>
struct dynsym : public sym<dynsym<RANGE>,RANGE> { 
	const char *n;
	constexpr dynsym(const char *name) : n(name) {}

	typedef dynsym<RANGE> mytype;
	
	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const
		{ return false; }

	constexpr bool samesym(const mytype &s) const
		{ return n==s.deriv()->n; }
		
	template<typename V, typename E>
	constexpr mytype dosubst(const assignexpr<V,E> &) const
		{ return *this; }

	template<typename E>
	constexpr ctmathexpr<RANGE> dosubst
			(const assignexpr<mytype,mathexpr<E,RANGE>> &a) const {
		return (!strcmp(a.lhs.n,n)) ? ctmathexpr<RANGE>(a.rhs)
				: ctmathexpr<RANGE>(*this);
	}

	ctmathexpr<RANGE> dosubst(const typelessassign &a) const {
		try {
			const mytype &l = a.lhs->as<mytype>();
			if (!strcmp(l.n,n))
				return ctmathexpr<RANGE>(a.rhs->withtype<RANGE>());
			else return ctmathexpr<RANGE>(*this);
		} catch(const std::bad_cast &) {
			return ctmathexpr<RANGE>(*this);
		}
	}
};


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

template<typename O, typename A, typename... Fs, std::size_t... I>
constexpr auto doassign_help(const A &a, const O &o, const std::tuple<Fs...> &fs, 
			std::index_sequence<I...>) {
	return o((std::get<I>(fs))[a]...);
}

template<typename O, typename A, typename... Fs>
constexpr auto doassign(const A &a, const O &o, const std::tuple<Fs...> &fs) {
	return doassign_help(a,o,fs,std::index_sequence_for<Fs...>{});
}


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
};

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
struct symnegate {
	template<typename T>
	constexpr auto operator()(T &&t) const
			noexcept(noexcept(-std::forward<T>(t))) {
		return -std::forward<T>(t);
	}
};

template<typename OP, typename... Fs>
constexpr auto doop(Fs &&...fs) {
	return symop<OP,typename std::remove_reference<Fs>::type...>
				(std::forward<Fs>(fs)...);
}

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
			constsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator+(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symplus>(constsym<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

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
			constsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator-(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symminus>(constsym<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

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
			constsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator*(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symmultiplies>(constsym<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

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
			constsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator/(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symdivides>(constsym<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

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
			constsym<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator%(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<symmodulus>(constsym<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

template<typename F1,
			typename std::enable_if<is_mathsym<F1>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1) {
	return doop<symnegate>(std::forward<F1>(f1));
}

#endif
