#include "poly.hpp"
#include <iostream>

using namespace std;

void testtopoly(const expr &e, const expr &x) {
	cout << "=====" << endl;
	cout << tostring(e) << " on " << tostring(x) << endl;
	auto p = topoly(e,x);
	if (p) cout << " => " << tostring(*p) << endl;
	else cout << " N/A" << endl;
}

void testdivide(const expr &v, const expr &n, const expr &d) {
	cout << tostring(n) << " / " << tostring(d) << " = " << std::endl;
	auto res = polydivide(*topoly(n,v),*topoly(d,v));
	cout << "\t" << tostring(res.first)
		<< " + " << tostring(res.second) << "/" << tostring(d) << endl;
	res = polypseudodivide(*topoly(n,v),*topoly(d,v));
	cout << "\t" << tostring(res.first)
		<< " + " << tostring(res.second) << "/" << tostring(d) << endl;
}

void testgcd(const expr &v, const expr &a, const expr &b) {
	cout << tostring(a) << " & " << tostring(b) << std::endl;
	auto res = polygcd(*topoly(a,v),*topoly(b,v));
	cout << "\t" << tostring(res) << endl;
}
	

int main(int argc, char **argv) {

	auto x = newvar<scalarreal>("x");
	auto y = newvar<scalarreal>("y");
	auto z = newvar<scalarreal>("z");

	auto e1 = pow(x,2)+x*y+z+3;
	auto e2 = x*(x+y+z)*z*(x+3)/2*(z+2)/3*z;
	auto e3 = x*(x+y)+exp(z);
	auto e4 = x*(x+y)+exp(z)+pow(x,y);

	testtopoly(e1,x);
	testtopoly(e1,y);
	testtopoly(e1,z);
	testtopoly(e2,x);
	testtopoly(e2,y);
	testtopoly(e2,z);
	testtopoly(e3,x);
	testtopoly(e3,y);
	testtopoly(e3,z);
	//testtopoly(e3,exp(z)); // not currently allowed -- do subst first
	testtopoly(e4,x);
	testtopoly(e4,y);
	testtopoly(e4,z);

	testdivide(x,3*pow(x,3) + x*x + x + 5, 5*x*x - 3*x + 1);
	testdivide(x,x*x*x*3 + x*x - 4, x-1);
	testdivide(x,x*x*x*3 + x - 4, x-1);
	testdivide(x,x*x*x*2 + x - 4, x-1);
	testdivide(x,x*x*x*2 -5*x*x + x - 4, x*x-1);

	testdivide(x,x*x*x*(2+y) -5*x*x - z*y*x*x + x - 4 + z, x*x-z);

	testgcd(x,pow(x,4)-2*pow(x,3) - 6*pow(x,2)+12*x+15,pow(x,3)+pow(x,2)-4*x-4);
	testgcd(x,pow(x,2)-1,pow(x,2)-x);
	testgcd(x,pow(x,2)*y-y,pow(x,2)-x);
	testgcd(x,pow(x,2)-x,pow(x,2)*y-y);
	testgcd(z,x*pow(z,3) + (x*x+3*x)*z + 3*x*z*x + z + x,
	          -pow(z,4)*x -3*pow(z,3)*x - pow(z,2) + 3*x*y*z*z + pow(x*z,2)
		      +(y+3*x*x)*z + x);
}
