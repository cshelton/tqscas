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

	constexpr inst<decltype(v1)> x1(4.4);
	cout << getarg("x",v1,x1) << endl;

	cout << "--------------" << endl;

	constexpr varpair<var<double>,varpair<var<int>,var<double>>> v2("a","b","c");
	constexpr int i2 = v2.getindex("b");
	constexpr int i3 = v2.getindex<int>("a");
	cout << i2 << endl; 
	cout << i3 << endl; 

	constexpr inst<decltype(v2)> x2(2.3,1,-5.6);

	//cout << is_same<int,typename decltype(x2)::commontype>::value << endl;
	//cout << is_same<notype,typename decltype(x2)::commontype>::value << endl;
	//cout << is_same<double,typename decltype(x2)::commontype>::value << endl;
	constexpr double a2 = getarg("c",v2,x2);
	cout << a2 << endl;

	cout << "--------------" << endl;
	
	constexpr varpair<var<double>,varpair<var<A>,var<double>>> v3("a","b","c");

	constexpr inst<decltype(v3)> x3(2.3,A{1},-5.6);
	constexpr double a3 = getarg<double>("a",v3,x3);
	cout << a3 << endl;

	cout << "--------------" << endl;

	varpair<multvar<char>,multvar<A>> v4(multvar<char>{"one","two"},multvar<A>{"alpha","beta"});

	auto aa1 = {'A','z'};
	auto aa2 = {A{3},A{2}};
	inst<decltype(v4)> x4(aa1,aa2);
	
	cout << getarg<char>("one",v4,x4) << endl;
	//char cc = getarg<char>("alpha",v4,x4);
	//cout << cc << endl;
	//cout << x4[v4["two"]] << endl;
	//
	cout << "---------------" << endl;

	varpair<multvar<double>,varpair<multvar<double>,var<double>>> v5
			(multvar<double>{"mercury","venus","earth","mars"},
				multvar<double>{"jupiter","saturn","uranus","neptune"},"pluto");

	auto bb1 = {3.4, 4.5, 5.6, 6.7};
	auto bb2 = {10.1, 11.1, 12.1, 13.1};
	inst<decltype(v5)> x5(bb1,bb2,0.5);

	cout << std::is_same<typename decltype(v5)::commontype,double>::value << endl;	
	cout << std::is_same<typename deduce_ret<anytype,decltype(v5)>::type,double>::value << endl;	
	cout << x5[v5["earth"]] << endl;

	cout << "---------------" << endl;

	constexpr inst<unnamedvar<const char *>> x6("hello there");
	constexpr const char *ss = x6;
	cout << ss << endl;

	constexpr inst<unnamedvar<double>> x7(5.6);
	cout << -x7*2.0 << endl;

	cout << "---------------" << endl;
	constexpr assign<double> x8(4.44);
	
	constexpr double a8 = x8;
	cout << a8 << endl;
}
