#ifndef MATHFN_H
#define MATHFN_H

#include <utility>
#include <type_traits>
#include <cmath>
#include <tuple>

/*
template<typename DOMAIN, typename RANGE>
struct mathfn {
	typedef DOMAIN domain;
	typedef RANGE range;

	range operator()(domain)
}
*/

struct anytype{};

template<typename T>
struct makevoid {
	typedef void type;
};

template<typename T, typename D=void, typename R=void>
struct is_mathfn {
	enum {value = 0 };
};

template<typename T>
struct is_mathfn<T,typename makevoid<typename std::remove_reference<T>::type::domain>::type,
			typename makevoid<typename std::remove_reference<T>::type::range>::type> {
	enum {value = 1 };
};

template<typename RANGE>
struct constfn {
	typedef anytype domain;
	typedef RANGE range;

	range k;

	constexpr constfn(const range &r) : k(r) {}
	constexpr constfn(range &&r) : k(std::move(r)) {}

	template<typename X>
	constexpr range operator()(const X &) const { return k; }
};

template<typename RANGE>
struct identityfn {
	typedef RANGE domain;
	typedef RANGE range;

	constexpr range operator()(const domain &x) const { return x; }
};

/* based on stackoverflow 7858817, answer by Walter */

template<typename O, typename D, typename... Fs, std::size_t... I>
constexpr auto call_help(const O &o, const std::tuple<Fs...> &fs, const D &x,
			std::index_sequence<I...>) {
	return o((std::get<I>(fs))(x)...);
}

template<typename O, typename D, typename... Fs>
constexpr auto call(const O &o, const std::tuple<Fs...> &fs, const D &x) {
	return call_help(o,fs,x,std::index_sequence_for<Fs...>{});
}

template<typename T1, typename T2>
struct mycommontype {
	typedef typename std::common_type<T1,T2>::type type;
};

template<typename T>
struct mycommontype<anytype,T> {
	typedef T type;
};
template<typename T>
struct mycommontype<T,anytype> {
	typedef T type;
};
template<>
struct mycommontype<anytype,anytype> {
	typedef anytype type;
};


template<typename F1, typename... Fs>
struct mergedomains {
	typedef typename mycommontype<typename F1::domain,
				typename mergedomains<Fs...>::type>::type type;
};

template<typename F1>
struct mergedomains<F1> {
	typedef typename F1::domain type;
};

template<typename OP, typename F1, typename... Fs>
struct applyop {
	typedef std::tuple<F1,Fs...> TT;

	typedef typename mergedomains<F1,Fs...>::type domain;
	typedef decltype(call(std::declval<OP>(),std::declval<TT>(),std::declval<domain>())) range;

	OP op;
	TT fs;

	constexpr applyop(const F1 &f1, const Fs &...fs) : fs(f1,fs...) {}
	constexpr applyop(const OP &o, const F1 &f1, const Fs &...fs) : op(o), fs(f1,fs...) {}

	template<typename X=domain>
	constexpr typename std::enable_if<!std::is_same<X,anytype>::value,range>::type
	operator()(const domain &x) const {
		return call(op,fs,x);
	}

	template<typename D,typename X=domain>
	constexpr typename std::enable_if<std::is_same<X,anytype>::value,range>::type
	operator()(const D &x) const {
		return call(op,fs,x);
	}
		
};

// the ones in std:: (like std::plus) do not have constexpr operator()
// (they are supposed to in C++14, but clang and g++ do not yet have this)

struct fnplus {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)+std::forward<T2>(t2))){
		return std::forward<T1>(t1)+std::forward<T2>(t2);
	}
};
struct fnminus {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)-std::forward<T2>(t2))){
		return std::forward<T1>(t1)-std::forward<T2>(t2);
	}
};
struct fnmultiplies {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)*std::forward<T2>(t2))){
		return std::forward<T1>(t1)*std::forward<T2>(t2);
	}
};
struct fndivides {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)/std::forward<T2>(t2))){
		return std::forward<T1>(t1)/std::forward<T2>(t2);
	}
};
struct fnmodulus {
	template<typename T1, typename T2>
	constexpr auto operator()(T1 &&t1, T2 &&t2) const
			noexcept(noexcept(std::forward<T1>(t1)%std::forward<T2>(t2))){
		return std::forward<T1>(t1)%std::forward<T2>(t2);
	}
};
struct fnnegate {
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
	typename std::enable_if<is_mathfn<F1>::value
				&& is_mathfn<F2>::value>::type *E=nullptr>
constexpr auto operator+(F1 &&f1, F2 &&f2) {
	return doop<fnplus>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator+(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<fnplus>(std::forward<F1>(f1),
			constfn<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator+(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<fnplus>(constfn<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

template<typename F1, typename F2,
	typename std::enable_if<is_mathfn<F1>::value
				&& is_mathfn<F2>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1, F2 &&f2) {
	return doop<fnminus>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<fnminus>(std::forward<F1>(f1),
			constfn<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator-(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<fnminus>(constfn<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

template<typename F1, typename F2,
	typename std::enable_if<is_mathfn<F1>::value
				&& is_mathfn<F2>::value>::type *E=nullptr>
constexpr auto operator*(F1 &&f1, F2 &&f2) {
	return doop<fnmultiplies>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator*(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<fnmultiplies>(std::forward<F1>(f1),
			constfn<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator*(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<fnmultiplies>(constfn<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

template<typename F1, typename F2,
	typename std::enable_if<is_mathfn<F1>::value
				&& is_mathfn<F2>::value>::type *E=nullptr>
constexpr auto operator/(F1 &&f1, F2 &&f2) {
	return doop<fndivides>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator/(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<fndivides>(std::forward<F1>(f1),
			constfn<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator/(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<fndivides>(constfn<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

template<typename F1, typename F2,
	typename std::enable_if<is_mathfn<F1>::value
				&& is_mathfn<F2>::value>::type *E=nullptr>
constexpr auto operator%(F1 &&f1, F2 &&f2) {
	return doop<fnmodulus>(std::forward<F1>(f1),std::forward<F2>(f2));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator%(F1 &&f1, const typename std::remove_reference<F1>::type::range &k) {
	return doop<fnmodulus>(std::forward<F1>(f1),
			constfn<typename std::remove_reference<F1>::type::range>(k));
}
template<typename F1,
	typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator%(const typename std::remove_reference<F1>::type::range &k, F1 &&f1) {
	return doop<fnmodulus>(constfn<typename std::remove_reference<F1>::type::range>(k),
			std::forward<F1>(f1));
}

template<typename F1,
			typename std::enable_if<is_mathfn<F1>::value>::type *E=nullptr>
constexpr auto operator-(F1 &&f1) {
	return doop<fnnegate>(std::forward<F1>(f1));
}



#endif
