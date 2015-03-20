#include <iostream>
#include <string>
#include "varset.h"
#include <utility>

using namespace std;

struct A {
	constexpr explicit A(int ii) : i(ii) {}

	int i;

	constexpr const int &get() const { return i; }
	int &get() { return i; }

};

ostream &operator<<(ostream &os, const A &a) {
	return os << a.i;
}

template<typename T>
struct B {
	B(int ii) : i(ii) {}

	template<typename S, typename std::enable_if<!std::is_same<T,S>::value>::type *E=nullptr>
	S retas(const S &s) { return i; }

	//const T &retas(const T &s) { return i; }
	
	template<typename S, typename std::enable_if<std::is_same<T,S>::value>::type *E=nullptr>
	const S &retas(const S &s) { return i; }

	int i;
};
/*
int main(int argc, char **argv) {
	B<int> b(65);
	
	cout << b.retas(1) << endl;
	cout << b.retas('a') << endl;
}
*/
/*
int main(int argc, char **argv) {
	constexpr varvalpair<varval<double>,varvalpair<varval<int>,varval<double>>> x2(2.3,1,-5.6);

	cout << allsame<decltype(x2)>::value << endl;
}
*/

int main(int argc, char **argv) {
	constexpr var<double> v1("x");
	constexpr int i1 = v1.getindex<string>("x");

	string s;
	s = 4.5;

	const A a(4);
	cout << a.get() << endl;

	cout << s << endl;
	cout << i1 << endl;
	cout << std::is_assignable<string&,double>::value << endl;
	cout << endl;

	constexpr varval<double> x1(4.4);
	cout << getarg("x",v1,x1) << endl;

	cout << "--------------" << endl;

	constexpr varpair<var<double>,varpair<var<int>,var<double>>> v2("a","b","c");
	constexpr int i2 = v2.getindex("b");
	constexpr int i3 = v2.getindex<int>("a");
	cout << i2 << endl; 
	cout << i3 << endl; 

	constexpr varvalpair<varval<double>,varvalpair<varval<int>,varval<double>>> x2(2.3,1,-5.6);

	//cout << is_same<int,typename decltype(x2)::commontype>::value << endl;
	//cout << is_same<notype,typename decltype(x2)::commontype>::value << endl;
	//cout << is_same<double,typename decltype(x2)::commontype>::value << endl;
	constexpr double a2 = getarg("c",v2,x2);
	cout << a2 << endl;

	cout << "--------------" << endl;
	
	constexpr varpair<var<double>,varpair<var<A>,var<double>>> v3("a","b","c");

	constexpr varvalpair<varval<double>,varvalpair<varval<A>,varval<double>>> x3(2.3,A{1},-5.6);
	constexpr double a3 = getarg<double>("a",v3,x3);
	cout << a3 << endl;
	
}
