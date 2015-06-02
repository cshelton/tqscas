#include "commonunion.h"

#include <iostream>

using namespace std;

struct A {
	constexpr A(int cc) : c(cc) {}
	constexpr int foo(int a, int b) const {
		return a+b+c;
	}

	int c;
};

struct B {
	constexpr int foo(int a, int b) const {
		return a*b;
	}
};

struct C {
	constexpr C(int cc=1, int dd=1,int ee=0) : c(cc), d(dd), e(ee) {}
	constexpr int foo(int a, int b) const {
		return a*c/b*d+e;
	}

	int c,d,e;
};

int main(int argc, char **argv) {
	constexpr A aa{-3};
	constexpr commonunion<A> a(aa);
	constexpr auto u = a.foo(5,2);
	cout << u << endl;

	constexpr commonunion<A,B> ab(aa);
	constexpr auto v = ab.foo(5,2);
	cout << v << endl;

	constexpr B bb{};
	constexpr commonunion<A,B> ba(bb);
	constexpr auto w = ba.foo(5,2);
	cout << w << endl;

	constexpr commonunion<A,B,C> abc(C{});
	constexpr auto x = abc.foo(5,2);
	cout << x << endl;

	constexpr commonunion<C,B,A> cba(aa);
	constexpr auto y = cba.foo(5,2);
	cout << y << endl;

	constexpr commonunion<C,B,A> cba2(C{});
	constexpr auto p = cba2.foo(5,2);
	cout << p << endl;

	constexpr commonunion<commonunion<A,B>,C> abc2(C{});
	constexpr auto z = abc2.foo(5,2);
	cout << z << endl;

	
	cout << sizeof(A) << ' ' << sizeof(B) << ' ' << sizeof(C) << ' ' << sizeof(ba) << ' ' << sizeof(ab) << ' ' << sizeof(abc) << ' ' << sizeof(abc2) << ' ' << sizeof(commonunion<C,A>) << endl;

	cout << sameuniontype1<commonunion<>,typename std::decay<decltype(abc)>::type>::value << endl;
	cout << sameuniontype1<typename std::decay<decltype(abc)>::type,typename std::decay<decltype(cba)>::type>::value << endl;

	cout << sameuniontype<typename std::decay<decltype(abc)>::type,
				typename std::decay<decltype(abc)>::type>::value << endl;
	cout << sameuniontype<typename std::decay<decltype(abc2)>::type,
			typename std::decay<decltype(cba)>::type>::value << endl;
}
