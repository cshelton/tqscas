#include "varset.h"
#include <iostream>
#include <string>

using namespace std;

void foo() {}
template<typename T, typename E=void>
struct A;

template<typename T>
struct A<T, decltype(declval<T>()*declval<T>(),foo())> {
};

int main(int argc, char **argv) {
	constexpr varset<double> v1("x");
	constexpr varset<double,double,double> v("a","x","other");

	constexpr double d = getarg<double>("other",v,3,4.0,3.2);
	constexpr double d2 = getarg<int>("x",v1,3.4);

	cout << d << ' ' << d2 << endl;

	constexpr varset<double,int,double> v3("x","y","z");
	constexpr varsetval<double,int,double> vv3(v3,4.2,2,1.3);

	constexpr d3 = getarg<int>("y",vv3);

	cout << d3 << endl;
}
