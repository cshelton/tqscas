#ifndef SYMMATH_H
#define SYMMATH_H

#include <utility>
#include <type_traits>
#include <cmath>
#include <tuple>
#include <stdexcept>

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

template<typename LHS, typename RHS>
struct assignexpr {
	static_assert(std::is_same<typename LHS::range, typename RHS::range>::value,"assignment doesn't preserve type");

	typedef typename LHS::range range;

	LHS lhs;
	RHS rhs;
	template<typename T1, typename T2>
	constexpr assignexpr(T1 &&l, T2 &&r) : lhs(std::forward<T1>(l)),
			rhs(std::forward<T2>(r)) {}
};

struct typelessassign;


template<typename DERIV,typename RANGE>
struct mathexpr {
	typedef RANGE range;

	const DERIV *deriv() const { return (const DERIV *)(this); }
	DERIV *deriv() { return (const DERIV *)(this); }

	constexpr auto val() const { return deriv()->val(); }

	template<typename V, typename E>
	constexpr auto operator[](const assignexpr<V,E> &a) const { return deriv()[a]; }

	auto operator[](const typelessassign &a) const { return deriv()[a]; }
};

struct typelessabsexpr {
	virtual ~typelessabsexpr() = 0;
	virtual typelessabsctexpr *clone() const = 0;

	template<typename R>
	const absctexpr<R> &withtype() const {
		// exception thrown if not of this type
		return *(dynamic_cast<const absctexpr<R> *>(*this));
	}

	template<typename E>
	const E &as() const {
		// exception thrown if not of this type
		return (dynamic_cast<const ctexprimpl<E> *)(*this)->impl;
	}
};

template<RANGE>
using absctptr<RANGE> = std::value_ptr<absctexpr<RANGE>>;

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

template<typename RANGE>
struct ctmathexpr : mathexpr<ctmathexpr<RANGE>,RANGE>;

template<typename IT>
struct ctexprimpl : public absctexpr<typename IT::range>;

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
	typedef IT::range range;
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
	ctmathexpr(const absctptr<RANGE> &i) impl(i) {}

	constexpr auto val() const { return impl->val(); }

	template<typename A>
	constexpr auto operator[](const A &a) const
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
	range k;

	constexpr constsym(const range &r) : k(r) {}
	constexpr constsym(range &&r) : k(std::move(r)) {}

	constexpr auto val() const { return k; }

	template<typename V, typename E>
	constexpr constsym<RANGE> operator[](const assignexpr<V,E> &) const
		{ return *this; }

	constsym<RANGE> operator[](const typelessassign &) const
		{ return *this; }
};

template<typename DERIV, typename RANGE> {
struct sym : public mathexpr<DERIV,RANGE> {

	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const {
		return deriv()->samesym(s);
	}

};

template<typename RANGE,char ...N>
struct staticsym : public sym<staticsym<RANGE,N...>,RANGE> {

	typedef staticsym<RANGE,N...> mytype;
	
	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const
		{ return false; }

	constexpr bool samesym(const sym<mytype,RANGE> &s) const
		{ return true; }

	template<typename V, typename E>
	constexpr mytype operator[](const assignexpr<V,E> &) const {
		{ return *this; }

	template<typename E>
	constexpr E operator[](const assignexpr<mytype,E> &a) const
		{ return a.rhs; }

	ctmathexpr<RANGE> operator[](const typelessassign &a) const {
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
	constexpr sym(const char *name) : n(name) {}

	typedef dynsym<RANGE> mytype;
	
	template<typename D, typename R>
	constexpr bool samesym(const sym<D,R> &s) const
		{ return false; }

	constexpr bool samesym(const mytype &s) const
		{ return n==s.deriv()->n; }
		
	template<typename V, typename E>
	constexpr mytype operator[](const assignexpr<V,E> &) const {
		{ return *this; }

	template<typename E>
	constexpr ctmathexpr<RANGE> operator[]
			(const assignexpr<mytype>,mathexpr<E,RANGE>> &a) const {
		return (!strcmp(a.lhs.n,n)) ? ctmathexpr<RANGE>(a.rhs)
				: ctmathexpr<RANGE>(*this);
	}

	ctmathexpr<RANGE> operator[](const typelessassign &a) const {
		try {
			const mytype &l = a.lhs->as<mytype>();
			if (!strcmp(l.n,n))
				return ctmathexpr<RANGE>(a.rhs->withtype<RANGE>());
			else return ctmathexpr<RANGE>(*this);
		} catch(const std::bad_cast &)
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
struct symop : public mathexpr<symop<OP,F1,Fs...>,
		decltype(doval(std::declval<OP>(),std::declval<TT>()))>{
	typedef std::tuple<F1,Fs...> TT;

	OP op;
	TT fs;

	constexpr symop(const F1 &f1, const Fs &...fs) : fs(f1,fs...) {}
	constexpr symop(const OP &o, const F1 &f1, const Fs &...fs) : op(o), fs(f1,fs...) {}

	constexpr range val() const {
		return doval(op,fs);
	}

	template<typename A>
	constexpr auto operator[](const A &a) {
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
	return applyop<OP,typename std::remove_reference<Fs>::type...>
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
