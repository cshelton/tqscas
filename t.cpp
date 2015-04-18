#include <utility>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <type_traits>

using namespace std;

int global=4;

template<typename T>
struct A {
	template<typename U=T>
	typename std::enable_if<std::is_same<U,int>::value,T &>::type get() const { return global; }
	template<typename U=T>
	typename std::enable_if<!std::is_same<U,int>::value,bool>::type get() const { return false; }

};

template<typename T>
struct B {
	template<typename S>
	int foo(const S &) const { return 4; }

	template<typename S>
	int foo(const A<S> &) const { return 1; }
};

	

int main(int argc, char **argv) {
	A<int> a;
/*
	cout << a.get() << endl;
	A<void> b;
	cout << b.get() << endl;
*/

	B<void> c;
	cout << c.foo(a) << ' ' << c.foo(2) << ' ' << c.foo(3.4) << endl;
	
}
