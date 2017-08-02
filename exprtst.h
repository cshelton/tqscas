#ifndef EXPRTST_H
#define EXPRTST_H

#include "exprcall.h"

struct isconstT {
	template<typename T>
	constexpr bool evalconst(T &&) { return true; }
	template<typename T, typename NAME>
	constexpr bool evalsym(NAME &&) { return false; }
	template<typename OP, typename... ARGS>
	constexpr bool evalop(OP &&op, ARGS &&...args) {
		return callonargs(isconstT{},std::logical_and{},true,std::forward<ARGS>(args)...);
	}
};
template<typename E>
constexpr bool isconst(E &&e) {
	return callonexpr(isconstT{},std::forward<E>(e));
}
	
struct issymT {
	template<typename T>
	constexpr bool evalconst(T &&) { return false; }
	template<typename T, typename NAME>
	constexpr bool evalsym(NAME &&) { return true; }
	template<typename OP, typename... ARGS>
	constexpr bool evalop(OP &&op, ARGS &&...args) { return false; }
};
template<typename E>
constexpr bool issym(E &&e) {
	return callonexpr(issymT{},std::forward<E>(e));
}

template<typename T, typename NAME>
struct samenameT {
	samenameT() : name() {}
	samenameT(const NAME &n) : name(n) {}
	const NAME &name;

	template<typename... ARGS>
	constexpr bool evalconst(ARGS &&...) { return false; }
	template<typename T2, typename NAME2>
	constexpr bool evalsym(NAME2 &&name2) {
		return std::is_same<T,T2>::value
			&& cmpnames(name,std::forward<NAME2>(name2))==0;
	}
	template<typename OP, typename... ARGS>
	constexpr bool evalop(OP &&op, ARGS &&...args) { return false; }
};
template<typename E, typename T, typename NAME>
constexpr bool issamesym(E &&e, const expr_sym_ct<T,NAME> &) {
	return callonexpr(issamesymT<T,NAME>{}, std::forward<E>(e));
}
template<typename E, typename T>
constexpr bool issamesym(E &&e, const expr_sym_rt<T> &e) {
	return callonexpr(
		issamesymT<T,typename std::decay<decltype(e.name)>::type>{e.name},
		std::forward<E>(e));
}


#endif
