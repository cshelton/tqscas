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

	checkrewrite(0.5*x*x*x + 0.25*x*x, 0.25*pow(x,2) + 0.5*pow(x,3));
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
	checkrewrite(log(exp(x+1-x)),scalar(1.0));
	checkrewrite(log(x/x),scalar(0.0));
	//checkrewrite(abs(x+y),cond(x+y,-1*x+-1*y,x+y));
	checkrewrite(log(cond(x,x,x*x)),cond(x,log(x),2*log(x)));
	//checkrewrite(abs(2*x+2*y)+abs(x+y),???);
	checkrewrite(deriv(2*x+y,x),scalar(2.0));
	checkrewrite(deriv(x*x,x),2*x);
	checkrewrite(deriv(log(x*y),x),pow(x,-1));
	checkrewrite(deriv(log(x+y*x),x),(1+y)*pow(x+x*y,-1));
// STUCK HERE... (probably in range propation) TODO
	checkrewrite(abs(x*x),pow(x,2));
	checkrewrite(abs(-x*x),pow(x,2));
	checkrewrite(abs(log(1.0+x*x)),log(1.0+pow(x,2)));
	checkrewrite(abs(log(0.5+x*x)),abs(log(0.5+pow(x,2))));
	checkrewrite(sum(x,x,1.0,10.0),scalar(55.0));
	checkrewrite(sum(sum(x*y,x,1.0,10.0),y,0.0,2.0),scalar(55.0*3.0));
	checkrewrite(sum(y,x,1.0,10.0),10.0*y);
	checkrewrite(sum(y,x,y,y+6),7.0*y);
	checkrewrite(sum(x*y,x,1.0,10.0),55.0*y);
	checkrewrite(sum(x*y,y,1.0,10.0),55.0*x);
	checkrewrite(sum(abs(x),x,1.0,z)+sum(abs(y),y,1.0,z),
			2.0*sum(abs(x),x,1.0,z));
	checkrewrite(sum(x*x,x,0.0,y),scalarreal{1,6}*y + scalarreal{1,2}*pow(y,2) + scalarreal{1,3}*pow(y,3));
	checkrewrite(sum(x,x,0.0,y),scalarreal{1,2}*y+scalarreal{1,2}*pow(y,2));
/*
	checkrewrite(sum(x,x,1.0,y),0.5*y+0.5*pow(y,2.0));
	checkrewrite(sum(x,x,2.0,y),-1.0+0.5*y+0.5*pow(y,2.0));
	checkrewrite(sum(x,x,3.0,y),-3.0+0.5*y+0.5*pow(y,2.0));
*/
}
