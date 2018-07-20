#include<variant>
#include<string>
#include<iostream>

using namespace std;

template<typename T>
struct A {
	A(){}
	A(T t) { x = t; }
	T x;
};

template<typename T>
struct B {
	B(){}
	B(T t) { x = t; }
	T x;
};

struct C {
	template<typename T>
	void operator()(const A<T> &x) {
		cout << "A:" << x.x;
	}
	template<typename T>
	void operator()(const B<T> &x) {
		cout << "B:" << x.x;
	}
	template<typename T>
	void operator()(const T &x) {
		cout << x;
	}
};

template<typename E>
void doit(const E &e) {
	visit(C{},e);
	cout << endl;
}

int main(int argc, char **argv) {
	variant<A<int>,A<std::string>,A<double>,B<int>,B<char>,int> x,y,z;

	x = A<double>(3.4);
	y = B<int>(2);
	z = B<int>(3);

	doit(x);
	doit(y);
	doit(z);

	x = 4;


	doit(x);
	doit(y);
	doit(z);
}

