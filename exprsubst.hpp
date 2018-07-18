#ifndef EXPRSUBST_HPP
#define EXPRSUBST_HPP

#include "exprbase.hpp"
#include "exprmatch.hpp"
#include <map>
#include <iterator>

// Assumes that the type of the expression remains unchanged
template<typename E>
E substitute(const E &e, const E &x, const E &v) {
	return e.map([x,v](const E &ex) {
			if (ex.sameas(x)) return std::optional<E>{v};
			return std::optional<E>{};
			});
}

// x -> v
template<typename T, typename E>
E substitute(const E &e, const E &x, const T &v) {
	return substitute(e,x,newconst(v));
}

// x -> v
template<typename E>
struct subst {
	subst(E xx, E vv) : x(std::move(xx)), v(std::move(vv)) {}
	E x,v;
};

template<typename E>
E substitute(const E &e, const subst<E> &st) {
	return substitute(e,st.x,st.v);
}

template<typename E>
E substitute(const E &e, const std::vector<subst<E>> &st) {
	return e.map([&st](const E &ex) {
			for(auto &s : st)
				if (ex.sameas(s.x)) return std::optional<E>{s.v};
			return std::optional<E>{};
			});
};

template<typename E>
E operator|(E e, std::vector<subst<E>> st) {
	return substitute(std::move(e),std::move(st));
}

template<typename E>
std::vector<subst<E>> operator<<(E xx, E vv) {
	return {1,subst{std::move(xx),std::move(vv)}};
}

template<typename E>
std::vector<subst<E>> operator&(std::vector<subst<E>> v1,
							std::vector<subst<E>> v2) {
	std::vector<subst<E>> ret(std::move(v1));
	ret.insert(ret.end(),std::make_move_iterator(v2.begin()),
			std::make_move_iterator(v2.end()));
	return ret;
}

//------------------

struct placeholder {
	int num;
};

template<typename E>
constexpr E P(int i) { return E{placeholder{i}}; }


template<typename E>
constexpr E P0_ = P<E>(0);
template<typename E>
constexpr E P1_ = P<E>(1);
template<typename E>
constexpr E P2_ = P<E>(2);
template<typename E>
constexpr E P3_ = P<E>(3);
template<typename E>
constexpr E P4_ = P<E>(4);
template<typename E>
constexpr E P5_ = P<E>(5);
template<typename E>
constexpr E P6_ = P<E>(6);
template<typename E>
constexpr E P7_ = P<E>(7);
template<typename E>
constexpr E P8_ = P<E>(8);
template<typename E>
constexpr E P9_ = P<E>(9);

template<typename E>
bool isplaceholder(const E &e) {
	return (e.isleaf() && e.asleaf().type()==typeid(placeholder));
}

template<typename E>
E substitute(const E &e, const exprmap<E> st) {
	return e.map([st](const E &ex) {
			if (isplaceholder(ex)) {
				int n = MYany_cast<placeholder>(ex.asleaf()).num;
				auto l = st.find(n);
				if (l!=st.end()) return std::optional<E>{l->second};
			}
			return std::optional<E>{};
		});
}

//--------------------

template<typename E>
E replacelocal(const E &e) {
	return e.map([](const E &ex) {
			if (!isop<scopeinfo>(ex) ||
			    isplaceholder(ex.children()[0]))
				return std::optional<E>{};
			return std::optional<E>{in_place,substitute(ex,ex.children()[0],
					newvar(getvartype(ex.children()[0]));
					)};
		});
}

#endif
