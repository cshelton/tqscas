#include <iostream>

std::string to_string(char c) {
	return std::string(1,c);
}

std::string to_string(int x) {
	return std::to_string(x); 
}

#include "subst.hpp"
#include "sugar.hpp"
#include "typetostr.hpp"
#include "exprtostr.hpp"

using namespace std;

int main(int argc, char **argv) {
	auto x = newvar<double>("x");
	auto y = newvar<double>("y");
	auto z = newvar<double>("z");
	auto k = newconst(3);
	auto k2 = newconst(3.1);

	auto e1 = x+y*z+k*z;

	cout << to_string(e1) << endl;
	auto e2 = substitute(e1,z,k2);
	cout << to_string(e2) << endl;
	auto e3 = substitute(e1,z,k);
	cout << to_string(e3) << endl;

	auto kc = newconst('b');
	auto e4 = substitute(e1,z,kc);
	cout << to_string(e4) << endl;
	cout << typetostr<decltype(e4)>() << endl;
	cout << to_string(kc) << endl;
	cout << hasmytostring_v<char> << endl;
	cout << to_string('b') << endl;

	cout << to_string(e4 | y << k) << endl;
	cout << to_string(e4 | y << k & (3*kc) << z) << endl;

	auto e5 = (k+y*P1_)/(P2_+5+P3_);
	cout << to_string(e5) << endl;
	using E = exprunion_t<decltype(e1),exprunion_t<decltype(e2),exprunion_t<decltype(e3),decltype(e4)>>>;
	exprmap<E> ss;
	ss.emplace(1,upgradeexpr<E>(e3));
	ss.emplace(3,upgradeexpr<E>(kc));
	cout << to_string(substitute(e5,ss)) << endl;
}
