#include "symmath.h"
//#include "symmathcalc.h"
#include <functional>

#include <iostream>
#include <string>

using namespace std;

template<typename A, typename B>
void foo(const assignpair<A,B> &) {
	cout << typeid(A).name() << endl;
	cout << typeid(B).name() << endl;
}


int main(int argc, char **argv) {
	constexpr staticsym<double,'x'> x;
	constexpr staticsym<double,'y'> y;
	constexpr staticsym<double,'z'> z;

	auto f = x*x;
	auto df = f.d(x);
	f.print(cout); cout << "=>"; df.print(cout); cout << endl;
	auto g = x+0;
	auto sg = rtsimplify(g);
	sg.print(cout); cout << endl;
}
