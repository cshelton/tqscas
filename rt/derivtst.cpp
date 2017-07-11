#include "expr.hpp"
#include <string>
#include "scalar.hpp"
#include "exprtostr.hpp"

using namespace std;

using E = expr;

void showderiv(const expr &e,const expr &x) {
	cout << "--------------------" << endl;
	cout << tostring(e) << ": " << endl;
	cout << draw(e) << " with respect to " << tostring(x) << endl;
	auto ee = deriv(e,x);
	cout << tostring(ee) << ": " << endl;
	cout << draw(ee) << endl;
}

int main(int argc, char **argv) {
	E x(newvar<double>("x"));
	E y(newvar<double>("y"));
	E z(newvar<double>("z"));

	showderiv((x+x)*(x+x)-y,x);
	showderiv((x+x)*(x+x)-y*pow(x,y),x);
	showderiv((x+x)*(x+x)-y*pow(x,y),y);

	showderiv(log(x*pow(y,x)),x);
	showderiv(log(x*pow(y,x)),y);

	showderiv(exp(x*y),x);

	showderiv(exp(2.0*x)/x, x);
	showderiv(pow(y,2.0*x)/x, x);

	showderiv(log(exp(2.0*x)/x), x);
}
