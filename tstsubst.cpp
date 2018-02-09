//#include "scalarrewrite.hpp"
#include <iostream>
#include "scalar.hpp"

using namespace std;

expr F(int n, int d) {
	return scalar(scalarreal{n,d});
}

int main(int argc, char **argv) {
	expr x = newvar<scalarreal>("x");
	expr y = newvar<scalarreal>("y");
	expr z = newvar<scalarreal>("z");
	expr k = F(4.0,1.0);

	auto rewriterules = scalarruleset;

	cout << tostring(x + y + z | x<<k & y<<F(1,2)) << endl;
	cout << tostring(exp(x*x) + exp(y + z) | x<<k & y<<F(1,2)) << endl;
	cout << tostring(rewriterules.rewrite(exp(x*x) + exp(y + z) | x<<k & y<<F(1,2))) << endl;
}
