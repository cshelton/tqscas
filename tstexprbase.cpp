#include "exprbase.hpp"
#include <iostream>
#include <string>

using namespace std;


struct myop {};

int ndigits(int x) {
	if (x==0) return 1;
	int ret = 0;
	while ((x/=10) != 0) ret++;
	return ret;
}

int raise(int x, int i) {
	for(;i>0;i--) x*= 10;
	return x;
}

int evalop(const myop &, int x, int y) {
	return raise(x,ndigits(y))+y;
}

std::string symbol(const myop &) {
	return "$";
}

int precedence(const myop &) {
	return 3;
}

template<typename... Ts>
ostream operator<<(ostream &os, const variant<Ts...> &v) {
	return visit([&os](auto &&x) { return os << x; }, v);
}

int main(int argc, char **argv) {
	auto i = newvar<int>("i"), j = newvar<int>("j");

	auto k1 = newconst(1), k2 = newconst(2);

	auto e1 = buildexpr(myop{},i,j);
	auto e2 = buildexpr(myop{},k1,k2);
	auto e3 = buildexpr(myop{},e2,e2);
	auto e4 = buildexpr(myop{},e3,e2);

	cout << eval(e2) << ' ' << eval(e3) << ' '
		<< eval(e4) << endl;
}

