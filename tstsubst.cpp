#include "scalarrewrite.hpp"
#include <iostream>

using namespace std;

expr F(int n, int d) {
	return scalar(scalarreal{n,d});
}

int main(int argc, char **argv) {
	expr x = newvar<scalarreal>("x");
	expr y = newvar<scalarreal>("y");
	expr z = newvar<scalarreal>("z");
	expr k = newconst(4.0);

	cout << tostring(x + y + z | x<<k & y<<F(1,2)) << endl;

}
