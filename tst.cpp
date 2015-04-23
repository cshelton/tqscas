#include "symmath.h"
//#include "symmathcalc.h"
#include <functional>

#include <iostream>

using namespace std;


int main(int argc, char **argv) {
	constexpr staticsym<double,'x'> x;
	constexpr staticsym<double,'y'> y;
	constexpr staticsym<double,'z'> z;

	constexpr auto f1 = x+x*x+y;
	
	constexpr auto a1 = (x==3.0);
	constexpr auto e1 = f1[a1];
	constexpr auto a2 = (y==1.0);
	constexpr auto a3 = (z==2.0);
	constexpr auto e2 = e1[a2];
	constexpr auto e3 = e2[a3];
	constexpr double v = e3.val();

	constexpr auto aa0 = (a1 & a2);

	cout << f1[aa0].val() << endl;

	constexpr auto f2 = z*y + x*y + x*z;
	constexpr auto v2 = (f2*f1)[x==3 & y==7 & z==10].val();
	cout << v2 << endl;

/*
	staticsym<double,'x'> x;
	staticsym<double,'y'> y;
	auto f = x+y;
	auto a = ((x==3.0) & (y==1.0));
	cout << f[a].val() << endl;
*/
	

}

