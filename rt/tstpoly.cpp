#include "poly.hpp"
#include <iostream>

using namespace std;

void tryit(const expr &e, const expr &x) {
	cout << "=====" << endl;
	cout << tostring(e) << " on " << tostring(x) << endl;
	auto p = topoly(e,x);
	if (p) cout << " => " << tostring(*p) << endl;
	else cout << " N/A" << endl;
}
	

int main(int argc, char **argv) {

	auto x = newvar<scalarreal>("x");
	auto y = newvar<scalarreal>("y");
	auto z = newvar<scalarreal>("z");

	auto e1 = pow(x,2)+x*y+z+3;
	auto e2 = x*(x+y+z)*z*(x+3)/2*(z+2)/3*z;
	auto e3 = x*(x+y)+exp(z);
	auto e4 = x*(x+y)+exp(z)+pow(x,y);

	tryit(e1,x);
	tryit(e1,y);
	tryit(e1,z);
	tryit(e2,x);
	tryit(e2,y);
	tryit(e2,z);
	tryit(e3,x);
	tryit(e3,y);
	tryit(e3,z);
	//tryit(e3,exp(z)); // not currently allowed -- do subst first
	tryit(e4,x);
	tryit(e4,y);
	tryit(e4,z);
}
