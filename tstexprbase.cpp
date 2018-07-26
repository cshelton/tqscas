#include "exprbase.hpp"
#include <iostream>
#include <string>
#include "typetostr.hpp"
#include <type_traits>

using namespace std;


struct myop {};

template<typename T>
T ndigits(T x) {
	T ret = 1;
	while ((x/=10) != 0) ret++;
	return ret;
}

template<typename T, typename S>
auto raise(T x, S i) {
	for(;i>0;i--) x*= 10;
	return x;
}

template<typename T, typename S>
auto intconcat(T x, S y) {
	return raise(x,ndigits(y))+y;
}

template<typename T, typename S,
	std::enable_if_t<std::is_integral_v<T> && std::is_integral_v<S>,int> =0>
auto evalop(const myop &, T x, S y) {
	return intconcat(intconcat(9,intconcat(x,y)),9);
}

std::string symbol(const myop &) {
	return "$";
}

int precedence(const myop &) {
	return 3;
}

struct myop2 {};

template<typename T, typename S,
	int_t<decltype(std::declval<T>()+std::declval<S>())> =0>
auto evalop(const myop2 &, T x, S y) {
	return x+y;
}

std::string symbol(const myop2 &) {
	return "+";
}

int precedence(const myop2 &) {
	return 2;
}



ostream &operator<<(ostream &os, const std::monostate &) {
	return os << "monostate";
}

template<typename T, typename... Ts>
ostream &operator<<(ostream &os, const variant<T, Ts...> &v) {
	return visit([&os](auto &&x) -> ostream & { return os << x; }, v);
}

int main(int argc, char **argv) {
	auto i = newvar<long long>("i"), j = newvar<long long>("j");

	auto k1 = newconst(1ll), k2 = newconst(2ll);

	auto e1 = buildexpr(myop{},i,j);
	auto e2 = buildexpr(myop{},k1,k2);
	auto e3 = buildexpr(myop{},e2,e2);
	auto e4 = buildexpr(myop{},e3,e2);
	auto e5 = buildexpr(myop{},e2,e3);

	auto v2 = eval(e2);
	cout << v2 << endl;
	cout << eval(e3) << endl;
	cout << eval(e4) << endl;
	cout << eval(e5) << endl;

	auto ii = newvar<int>("ii"), ji = newvar<int>("ji");
	auto ki1 = newconst(1), ki2 = newconst(2);

	auto e6 = buildexpr(myop{},ki1,ki2);
	auto e7 = buildexpr(myop{},k1,k2);
	auto e8 = buildexpr(myop{},e6,e7);
	auto e9 = buildexpr(myop{},e8,ki2);
	cout << eval(e6) << endl;
	cout << typetostr<decltype(e6)>() << endl;
	cout << eval(e7) << endl;
	cout << typetostr<decltype(e7)>() << endl;
	cout << eval(e8) << endl;
	cout << typetostr<decltype(e8)>() << endl;
	cout << eval(e9) << endl;
	cout << typetostr<decltype(e9)>() << endl;


	auto e10 = buildexpr(myop2{},e7,e6);
	cout << eval(e10) << endl;
	cout << typetostr<decltype(e10)>() << endl;

/*
	cout << exprsame(e10,e10) << ' ' << exprsame(e3,e10) << ' ' << exprsame(e1,e1) << endl;

	cout << exprsame(buildexpr(myop{},i,j),buildexpr(myop{},i,j)) << endl;
	cout << exprsame(buildexpr(myop{},i,j),buildexpr(myop2{},i,j)) << endl;
	cout << exprsame(buildexpr(myop{},i,buildexpr(myop{},j,k1)),
				buildexpr(myop{},i,k1)) << endl;
	cout << exprsame(buildexpr(myop{},i,buildexpr(myop{},j,k1)),
				buildexpr(myop{},i,buildexpr(myop{},j,k1))) << endl;
	cout << exprsame(buildexpr(myop{},i,buildexpr(myop{},j,k2)),
				buildexpr(myop{},i,buildexpr(myop{},j,k1))) << endl;
	cout << exprsame(buildexpr(myop{},i,buildexpr(myop{},j,k2)),
				buildexpr(myop{},ii,buildexpr(myop{},ji,ki2))) << endl;
*/



	
}

