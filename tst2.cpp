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

	constexpr auto ftest = rtconstsym<double>(3);
	constexpr auto vtest = ftest.val();
	cout << vtest << endl;

	constexpr auto f = x*x;
	auto df = f.d(x);
	f.print(cout); cout << "=>"; df.print(cout); cout << endl;
	auto g = x+0;
	auto sg = rtsimplify(g);
	sg.print(cout); cout << endl;
	rtconstsym<double> dconst(3.0);
	cout << is_constsym<decltype(dconst)>::value << endl;
	cout << is_mathsym<decltype(dconst)>::value << endl;
	cout << is_mathsymnew<decltype(dconst)>::value << endl;
	cout << is_mathsym<decltype(x)>::value << endl;
	cout << is_mathsymnew<decltype(x)>::value << endl;
}
