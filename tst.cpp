#include "mathfn.h"
#include "mathfncalc.h"
#include <functional>

#include <iostream>

using namespace std;


int main(int argc, char **argv) {
	constexpr auto x = identityfn<double>{};
	constexpr auto f1 = -(3.5*(x*x+4));
	constexpr auto f2 = f1+x+3;
	//constexpr auto f2 = x+3;

	constexpr auto f3 = derivative(f2);
	constexpr auto f4 = integral(f2);
	constexpr auto f5 = f4 - f4(0);

	
	constexpr auto v0 = f2(1);
	constexpr auto v1 = f3(1);
	constexpr auto v2 = f4(1);
	constexpr auto v3 = f5(1);

	cout << v0 << endl;
	cout << v1 << endl;
	cout << v2 << endl;
	cout << v3 << endl;

	
}

