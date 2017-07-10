#ifndef MATHFN_H
#define MATHFN_H

#include <utility>
#include <type_traits>
#include <cmath>


/*  concept:
template<typename X, typename Y>
class mathfn {
public:
	Y operator()(const domain &x) const;
};
*/

// from http://flamingdangerzone.com/cxx11/2012/06/01/almost-static-if.html
namespace detail{ enum class enabler{}; }
template<typename C>
using EnableIf = typename std::enable_if<C::value,detail::enabler>::type;
template<typename C>
using DisableIf = typename std::enable_if<!C::value,detail::enabler>::type;

// from http://stackoverflow.com/questions/11813940/possible-to-use-type-traits-sfinae-to-find-if-a-class-defines-a-member-type

template<typename T>
struct makevoid {
	typedef void type;
};

template<typename T, typename D = void, typename R = void>
struct is_mathfn {
	enum { value = 0 };
};

template<typename T>
struct is_mathfn<T,typename makevoid<typename std::remove_reference<T>::type::domain>::type, typename makevoid<typename std::remove_reference<T>::type::range>::type> {
	enum {value = 1 };
};

template<typename F1, typename F2>
struct are_mathfn {
	enum {value = is_mathfn<F1>::value && is_mathfn<F2>::value};
};

template<typename Y>
class constfn {
public:
	using domain=void;
	using range=Y;
	constexpr constfn(const range &yy) : y(yy) {}
	constexpr constfn(range &&yy) : y(std::move(yy)) {}

	template<typename X>
	constexpr range operator()(const X &x) { return y; }
protected:
	range y;
};

constexpr constfn<unsigned long long int> operator"" _fn(unsigned long long int x) { return constfn<unsigned long long int>(x); }
constexpr constfn<long double> operator"" _fn(long double x) { return constfn<long double>(x); }

template<typename X>
class x_fn {
public:
	using domain=X;
	using range=X;
	constexpr range operator()(const domain &x) { return x; }
};

template<typename F1, typename F2, typename E=void>
struct combdomain {
	typedef typename std::common_type<typename F1::domain,typename F2::domain>::type type;
};

template<typename F1, typename F2>
struct combdomain<F1,F2,
		typename std::enable_if<!std::is_void<typename F1::domain>::value
						&& !std::is_void<typename F2::domain>::value>::type> {
	typedef typename std::common_type<typename F1::domain,typename F2::domain>::type type;
};

template<typename F1, typename F2>
struct combdomain<F1,F2,
		typename std::enable_if<std::is_void<typename F1::domain>::value>::type> {
	typedef typename F2::domain type;
};

template<typename F1, typename F2>
struct combdomain<F1,F2,
		typename std::enable_if<std::is_void<typename F2::domain>::value && !std::is_void<typename F1::domain>::value>::type> {
	typedef typename F1::domain type;
};


template<typename F1, typename F2> 
class addfn {
public:
	using domain = typename combdomain<F1,F2>::type;
	using range = decltype(std::declval<typename F1::range>()+
	                std::declval<typename F2::range>());
	template<typename FF1, typename FF2>
	constexpr addfn(FF1 &&ff1, FF2 &&ff2)
		: f1(std::forward<FF1>(ff1)), f2(std::forward<FF2>(ff2)) {}

	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return f1(x)+f2(x); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x) { return f1(x)+f2(x); }
protected:
	F1 f1;
	F2 f2;
};

template<typename F1, typename F2, EnableIf<are_mathfn<F1,F2>>...>
addfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
operator+(F1 &&f1, F2 &&f2) {
	return addfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
		(std::forward<F1>(f1), std::forward<F2>(f2));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
addfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator+(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return addfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(std::forward<F1>(f1), constfn<typename std::remove_reference<F1>::type::range>(k));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
addfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator+(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return addfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(constfn<typename std::remove_reference<F1>::type::range>(k), std::forward<F1>(f1));
}

template<typename F1, typename F2>
class multfn {
public:
	using domain = typename combdomain<F1,F2>::type;
	using range = decltype(std::declval<typename F1::range>()*
	                std::declval<typename F2::range>());
	template<typename FF1, typename FF2>
	constexpr multfn(FF1 &&ff1, FF2 &&ff2)
		: f1(std::forward<FF1>(ff1)), f2(std::forward<FF2>(ff2)) {}


	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return f1(x)*f2(x); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x) { return f1(x)*f2(x); }
protected:
	F1 f1;
	F2 f2;
};

template<typename F1, typename F2, EnableIf<are_mathfn<F1,F2>>...>
multfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
operator*(F1 &&f1, F2 &&f2) {
	return multfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
		(std::forward<F1>(f1), std::forward<F2>(f2));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
multfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator*(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return multfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(std::forward<F1>(f1), constfn<typename std::remove_reference<F1>::type::range>(k));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
multfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator*(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return multfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(constfn<typename std::remove_reference<F1>::type::range>(k), std::forward<F1>(f1));
}

template<typename T, typename E=void>
struct make_any_signed {
	typedef T type;
};

template<typename T> 
struct make_any_signed<T,typename std::enable_if<std::is_unsigned<T>::value>::type> {
	typedef typename std::make_signed<T>::type type;
};

template<typename F1, typename F2> 
class subfn {
public:
	using domain = typename combdomain<F1,F2>::type;
	using range = typename make_any_signed<decltype(std::declval<typename F1::range>()-std::declval<typename F2::range>())>::type;
	template<typename FF1, typename FF2>
	constexpr subfn(FF1 &&ff1, FF2 &&ff2)
		: f1(std::forward<FF1>(ff1)), f2(std::forward<FF2>(ff2)) {}

	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return f1(x)-f2(x); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x) { return f1(x)-f2(x); }
protected:
	F1 f1;
	F2 f2;
};


template<typename F>
class unarysubfn {
public:
	using domain = typename F::domain;
	using range = typename make_any_signed<typename F::range>::type;
	template<typename FF>
	constexpr unarysubfn(FF &&ff)
		: f(std::forward<FF>(ff)) {}

	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return -f(x); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x) { return -f(x); }
protected:
	F f;
};

template<typename F>
unarysubfn<typename std::remove_reference<F>::type>
operator-(F &&f) {
	return unarysubfn<typename std::remove_reference<F>::type>
		(std::forward<F>(f));
}

template<typename F1, typename F2, EnableIf<are_mathfn<F1,F2>>...>
subfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
operator-(F1 &&f1, F2 &&f2) {
	return subfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
		(std::forward<F1>(f1), std::forward<F2>(f2));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
subfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator-(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return subfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(std::forward<F1>(f1), constfn<typename std::remove_reference<F1>::type::range>(k));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
subfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator-(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return subfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(constfn<typename std::remove_reference<F1>::type::range>(k), std::forward<F1>(f1));
}

template<typename F1, typename F2> 
class divfn {
public:
	using domain = typename combdomain<F1,F2>::type;
	using range = decltype(std::declval<typename F1::range>()/
	                std::declval<typename F2::range>());
	template<typename FF1, typename FF2>
	constexpr divfn(FF1 &&ff1, FF2 &&ff2)
		: f1(std::forward<FF1>(ff1)), f2(std::forward<FF2>(ff2)) {}

	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return f1(x)/f2(x); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x) { return f1(x)/f2(x); }
protected:
	F1 f1;
	F2 f2;
};

template<typename F1, typename F2, EnableIf<are_mathfn<F1,F2>>...>
divfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
operator/(F1 &&f1, F2 &&f2) {
	return divfn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
		(std::forward<F1>(f1), std::forward<F2>(f2));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
divfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator/(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return divfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(std::forward<F1>(f1), constfn<typename std::remove_reference<F1>::type::range>(k));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
divfn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator/(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return divfn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(constfn<typename std::remove_reference<F1>::type::range>(k), std::forward<F1>(f1));
}

template<typename F1, typename F2> 
class raisefn {
public:
	using domain = typename combdomain<F1,F2>::type;
	using range = decltype(pow(std::declval<typename F1::range>(),
	                std::declval<typename F2::range>()));
	template<typename FF1, typename FF2>
	constexpr raisefn(FF1 &&ff1, FF2 &&ff2)
		: f1(std::forward<FF1>(ff1)), f2(std::forward<FF2>(ff2)) {}

	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return pow(f1(x),f2(x)); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x) { return pow(f1(x),f2(x)); }
protected:
	F1 f1;
	F2 f2;
};

template<typename F1, typename F2, EnableIf<are_mathfn<F1,F2>>...>
raisefn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
operator^(F1 &&f1, F2 &&f2) {
	return raisefn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
		(std::forward<F1>(f1), std::forward<F2>(f2));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
raisefn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator^(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return raisefn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(std::forward<F1>(f1), constfn<typename std::remove_reference<F1>::type::range>(k));
}

template<typename F1, EnableIf<is_mathfn<F1>>...>
raisefn<typename std::remove_reference<F1>::type, constfn<typename std::remove_reference<F1>::type::range>>
operator^(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return raisefn<typename std::remove_reference<F1>::type,
				constfn<typename std::remove_reference<F1>::type::range>>
		(constfn<typename std::remove_reference<F1>::type::range>(k), std::forward<F1>(f1));
}

template<typename F1, typename F2>
class composefn {
public:
	using domain = typename F2::domain;
	using range = typename F1::range;
	template<typename FF1, typename FF2>
	constexpr composefn(FF1 &&ff1, FF2 &&ff2)
		: f1(std::forward<FF1>(ff1)), f2(std::forward<FF2>(ff2)) {}

	template<typename D=domain>
	constexpr range operator()
		(const typename std::enable_if<!std::is_void<D>::value,D>::type &x)
		{ return f1(f2(x)); }

	template<typename X, typename D=domain>
	constexpr
		typename std::enable_if<std::is_void<D>::value,range>::type
		operator()(const X &x)
		{ return f1(f2(x)); }
protected:
	F1 f1;
	F2 f2;
};

template<typename F1, typename F2>
composefn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
operator%(F1 &&f1, F2 &&f2) {
	return composefn<typename std::remove_reference<F1>::type,
			typename std::remove_reference<F2>::type>
		(std::forward<F1>(f1), std::forward<F2>(f2));
}

#endif
