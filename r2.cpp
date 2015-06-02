#include "commonunion.h"

#include <iostream>
#include <stdexcept>

template<typename B>
struct mybase {
	int foobase() {
		return static_cast<B &>(*this).foo(3,4);
	}
	void check() const { std::cout << "check" << std::endl; }
};

DEFUNION(foobarunion,: public mybase<foobarunion<Ts...>>,foo,bar)

using namespace std;

struct A {
	A(int cc) : c(cc) {
		cout << "A constructed from int" <<endl;
	}

	A(const A &aa) : c(aa.c) {
		cout << "A constructed from A" <<endl;
	}

	A(A &&aa) : c(aa.c) {
		cout << "A move-constructed from A" <<endl;
	}

	~A() { cout << "A destroyed\n"; }

	A &operator=(const A &a) {
		c = a.c;
		cout << "A assigned\n";
		return *this;
	}

	A &operator=(A &&a) {
		c = a.c;
		cout << "A move-assigned\n";
		return *this;
	}

	int foo(int a, int b) {
		return a+b+(c++);
	}

	int foo(int a, int b) const {
		return a+b+(c);
	}

	std::string bar(const std::string &s1) const {
		return s1+"A:"+std::to_string(c);
	}

	int c;
};

struct B {
	B() { cout <<  "B constructed" << endl; }
	B(const B &b) {
		cout << "B constructed from B" <<endl;
	}

	B(B &&b)  {
		cout << "B move-constructed from B" <<endl;
	}

	~B() { cout << "B destroyed\n"; }

	B &operator=(const B &b) {
		cout << "B assigned\n";
		return *this;
	}

	B &operator=(B &&b) {
		cout << "B move-assigned\n";
		return *this;
	}

	int foo(int a, int b) const {
		return a*b;
	}

	std::string bar(const std::string &s1) const {
		return s1+"B:";
	}
};

struct C {
	C() : c(1), d(1), e(0) {
		cout << "C constructed from 0 ints\n"; }
	C(int cc, int dd,int ee) : c(cc), d(dd), e(ee) {
		cout << "C constructed from 3 ints\n"; }
	C(const C &cc) :c(cc.c), d(cc.d), e(cc.d) {
		cout << "C constructed from C" <<endl;
	}

	C(C &&cc) : c(cc.c), d(cc.d), e(cc.e) {
		cout << "C move-constructed from C" <<endl;
	}

	~C() { cout << "C destroyed\n"; }

	C &operator=(const C &cc) {
		c = cc.c; d = cc.d; e = cc.e;
		cout << "C assigned\n";
		return *this;
	}

	C &operator=(C &&cc) {
		c = cc.c; d = cc.d; e = cc.e;
		cout << "C move-assigned\n";
		return *this;
	}
	int foo(int a, int b) {
		c++; d++; e++;
		return a*c/b*d+e;
	}

	std::string bar(const std::string &s1) const {
		return s1+"C:"+std::to_string(c)+" "+std::to_string(d)+" "+std::to_string(e);
	}

	int c,d,e;
};

int main(int argc, char **argv) {
	A aa{-3};
	foobarunion<A> a(aa);
	auto u = a.foo(5,2);
	cout << u << endl;

	foobarunion<A,B> ab(aa);
	auto v = ab.foo(5,2);
	cout << v << endl;

	B bb{};
	foobarunion<A,B> ba(bb);
	auto w = ba.foo(5,2);
	cout << w << endl;

	foobarunion<A,B,C> abc(C{});
	auto x = abc.foo(5,2);
	cout << x << endl;

	foobarunion<C,B,A> cba(aa);
	auto y = cba.foo(5,2);
	cout << y << endl;

	foobarunion<C,B,A> cba2(C{});
	auto p = cba2.foo(5,2);
	cout << p << endl;
	cba2 = aa;

	foobarunion<foobarunion<A,B>,C> abc2(C{});
	auto z = abc2.foo(5,2);
	cout << z << endl;

	abc2.check();
	cout << abc2.foobase() << endl;;
	
	cout << sizeof(A) << ' ' << sizeof(B) << ' ' << sizeof(C) << ' ' << sizeof(ba) << ' ' << sizeof(ab) << ' ' << sizeof(abc) << ' ' << sizeof(abc2) << ' ' << sizeof(foobarunion<C,A>) << endl;

	cout << endl;

	cout << ab.bar(std::string("A:")) << endl;
	cout << ba.bar(std::string("B:")) << endl;
	cout << abc.bar(std::string("C:")) << endl;
	cout << cba.bar(std::string("A:")) << endl;
	cout << cba2.bar(std::string("C:")) << endl;
	cout << abc2.bar(std::string("C:")) << endl;

}
