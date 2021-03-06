#include "scalarrewrite.hpp"
#include <iostream>

using namespace std;

void checkrewrite(const expr &e, const expr &newe, const expr &v) {
	auto newset = scalarruleset;
	newset.setvar(getvar(v));
	expr re = newset.rewrite(e);
	if (re.sameas(newe)) {
		cout << "pass: ";
		cout << tostring(e) << " =?=> " << tostring(newe) << endl;
	} else {
		cout << "FAIL: ";
		cout << tostring(e) << " =?=> " << tostring(newe) << endl;
		cout << "\toutput = " << tostring(re) << endl;
		cout << "\ttarget = " << tostring(newe) << endl;
	}
}

void checkrewrite(const expr &e, const expr &newe) {
	expr re = scalarruleset.rewrite(e);
	if (re.sameas(newe)) {
		cout << "pass: ";
		cout << tostring(e) << " =?=> " << tostring(newe) << endl;
	} else {
		cout << "FAIL: ";
		cout << tostring(e) << " =?=> " << tostring(newe) << endl;
		cout << "\toutput = " << tostring(re) << endl;
		cout << "\ttarget = " << tostring(newe) << endl;
	}
}

expr F(int n, int d) {
	return scalar(scalarreal{n,d});
}

int main(int argc, char **argv) {
	expr x = newvar<scalarreal>("x");
	expr y = newvar<scalarreal>("y");
	expr z = newvar<scalarreal>("z");


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
	checkrewrite(log(exp(x+1-x)),scalar(1));
	checkrewrite(log(x/x),scalar(0));
	checkrewrite(log(ifthenelse(x,x,x*x)),ifthenelse(x,log(x),2*log(x)));

	checkrewrite(x*y,x*y,y);
	checkrewrite(x*y,y*x,x);

	checkrewrite(deriv(2*x+y,x),scalar(2));
	checkrewrite(deriv(x*x,x),2*x);
	checkrewrite(deriv(log(x*y),x),pow(x,-1));
	checkrewrite(deriv(log(x+y*x),x),y*pow(x+x*y,-1)+pow(x+x*y,-1));

	checkrewrite(abs(x*x),pow(x,2));
	checkrewrite(abs(-x*x),pow(x,2));
	checkrewrite(abs(log(1+x*x)),log(1+pow(x,2)));
	checkrewrite(abs(log(0.5+x*x)),abs(log(0.5+pow(x,2))));

	checkrewrite(sum(x,x,1,10),scalar(55));
	checkrewrite(sum(sum(x*y,x,1,10),y,0,2),scalar(55*3));
	checkrewrite(sum(y,x,1,10),10*y);
	checkrewrite(sum(y,x,y,y+6),7*y);
	checkrewrite(sum(x*y,x,1,10),55*y);
	checkrewrite(sum(x*y,y,1,10),55*x);
	checkrewrite(sum(abs(x),x,1,z)+sum(abs(y),y,1,z),
			2*sum(abs(x),x,1,z));
	checkrewrite(sum(x,x,0,y),ifthenelse(y,0,F(1,2)*y+F(1,2)*pow(y,2)));
	checkrewrite(sum(x,x,1,y),ifthenelse(-1+y,0,0.5*y+0.5*pow(y,2)));
	checkrewrite(sum(x,x,2,y),ifthenelse(-2+y,0,-1+0.5*y+0.5*pow(y,2)));
	checkrewrite(sum(x,x,3,y),ifthenelse(-3+y,0,-3+0.5*y+0.5*pow(y,2)));
	checkrewrite(sum((x+3),x,0,y-3),ifthenelse(-3+y,0,-3+0.5*y+0.5*pow(y,2)));
	checkrewrite(sum(x*x,x,0,y),ifthenelse(y,0,F(1,6)*y + F(1,2)*pow(y,2) + F(1,3)*pow(y,3)));
	checkrewrite(sum(x*x,x,4,y),ifthenelse(-4+y,0,scalar(-14)+F(1,6)*y + F(1,2)*pow(y,2) + F(1,3)*pow(y,3)));
// below doesn't work out this way currently
	//checkrewrite(sum((x+4)*(x+4),x,0,y-4),ifthenelse(-4+y,0,scalar(-14)+F(1,6)*y + F(1,2)*pow(y,2) + F(1,3)*pow(y,3)));
	checkrewrite(sum(x,x,y+2,y),scalar(0));
	checkrewrite(sum(x*x,x,y+2,y),scalar(0));

	checkrewrite(deriv(integrate(y*y,y,0,1),x),scalar(0));

// w/ table integration
	checkrewrite(deriv(integrate(y*x,y,0,1),x),F(1,2));
	checkrewrite(deriv(integrate(y*x,y,x,1),x),F(1,2)+F(-3,2)*pow(x,2));

	checkrewrite(integrate(y+x,y,x,1),F(1,2)+x+F(-3,2)*pow(x,2));
	checkrewrite(integrate(y*x,y,x,1),F(1,2)*x+F(-1,2)*pow(x,3));
	checkrewrite(integrate(y*z,y,x,1),F(-1,2)*z*pow(x,2) + F(1,2)*z);
	checkrewrite(integrate(y,y,0,1),F(1,2));
	
	checkrewrite(integrate(y*y*y/10 + y*y/x,y,x,z),
		F(1,3)*pow(x,-1)*pow(z,3)
		+ F(-1,3)*pow(x,2)
		+ F(-1,40)*pow(x,4)
		+ F(1,40)*pow(z,4)
		);
	checkrewrite(integrate(1/x,x,1,y), log(abs(y)));
	checkrewrite(integrate(1/(x+3),x,-2,y), log(abs(3+y)));
	checkrewrite(integrate(1/(3*x+3)/(3*x+3),x,0,y),
		F(1,9) + F(-1,3)*pow(3+3*y,-1));
	checkrewrite(integrate(1/(3*x+3)/(3*x+3),x,2,y),
		F(1,27) + F(-1,3)*pow(3+3*y,-1));

	expr lda = exp(-y*x)+1;
	expr tpt = scalar(3.0);
	expr l3 = substitute(lda,x,tpt);
	expr lt = integrate(exp(-y*x)+1,x,0.0,5.0);
	expr llh = scalarruleset.rewrite(log(l3*exp(-lt)));
	cout << tostring(llh) << endl << endl;
	expr dllh = scalarruleset.rewrite(deriv(llh,y,z));
	cout << tostring(dllh) << endl << endl;
	checkrewrite(llh,x);
	
}

/* w/o table integration
	checkrewrite(deriv(integrate(y*x,y,0,1),x),integrate(y,y,0,1));
	checkrewrite(deriv(integrate(y*x,y,x,1),x),-1*pow(x,2)+integrate(y,y,x,1));

	checkrewrite(integrate(y+x,y,x,1),integrate(x,y,x,1)+integrate(y,y,x,1));
	checkrewrite(integrate(y*x,y,x,1),x*integrate(y,y,x,1));
	checkrewrite(integrate(y*z,y,x,1),z*integrate(y,y,x,1));
	checkrewrite(integrate(y,y,0,1),integrate(y,y,0,1));
*/
