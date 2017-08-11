#include "scalarrewrite.hpp"
#include <iostream>

using namespace std;

void checkrewrite(const expr &e, const expr &newe) {
	expr re = rewrite(e,scalarruleset);
	if (re==newe) {
		cout << "pass: ";
		cout << tostring(e) << " =?=> " << tostring(newe) << endl;
	} else {
		cout << "FAIL: ";
		cout << tostring(e) << " =?=> " << tostring(newe) << endl;
		cout << "\toutput = " << tostring(re) << endl;
		cout << "\ttarget = " << tostring(newe) << endl;
	}
}

int main(int argc, char **argv) {
	expr x = newvar<double>("x");
	expr y = newvar<double>("y");
	expr z = newvar<double>("z");

	checkrewrite(x+x,2*x);
	checkrewrite(x+x+x,3*x);
	checkrewrite(x+(x+x)+(x+y),y+4*x);
	checkrewrite(x*x,pow(x,2));
	checkrewrite(x*x*x,pow(x,3));
	checkrewrite(x*(x*x)*(x*y),y*pow(x,4));
	checkrewrite(pow(x,5)*(x*pow(x,2))*(x*y),y*pow(x,9));
	checkrewrite(log(x*y*(z+4*y)), log(x) + log(y) + log(z+4*y));
	checkrewrite(pow(pow(x,z),y),pow(x,y*z));
	checkrewrite(pow(pow(2,x),3),pow(8,x));
	checkrewrite(log(exp(x)),x);
	checkrewrite(log(exp(x+1-x)),newconst(1.0));
	checkrewrite(log(x/x),newconst(0.0));
}
