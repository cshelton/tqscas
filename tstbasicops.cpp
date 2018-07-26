#include "exprsugar.hpp"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {
	auto x = newvar<int>("x"), y = newvar<int>("y");

	auto e1 = x+y;
	cout << draw(e1) << endl;
	auto e2 = e1+3;
	cout << draw(e2) << endl;
	auto e3 = -9+e2;
	cout << draw(e3) << endl;
	auto e4 = -8*x*-9+y/4;
	cout << draw(e4) << endl;
	auto e5 = -e4;
	cout << draw(e5) << endl;
	auto e6 = pow(3,2) + pow(x,log(y+3));
	cout << draw(e6) << endl;
	
}
