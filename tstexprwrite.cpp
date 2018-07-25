#include "exprbase.hpp"
#include <iostream>
#include <string>
#include "typetostr.hpp"
#include "exprtostr.hpp"
#include "exprbaseops.hpp"

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

template<typename T, typename S>
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

template<typename T, typename S>
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

template<typename E>
void dumpexpr(const E &e) {
	cout << typetostr<decay_t<decltype(e)>>() << endl;
	cout << to_string(e) << endl;
	cout << eval(e) << endl;
	cout << draw(e) << endl;
}

int main(int argc, char **argv) {
	auto i = newvar<long long>("i"), j = newvar<long long>("j");

	auto k1 = newconst(1ll), k2 = newconst(2ll);

	auto e1 = buildexpr(myop{},i,j);
	auto e2 = buildexpr(myop{},k1,k2);
	auto e3 = buildexpr(myop{},e2,e2);
	auto e4 = buildexpr(myop{},e3,e2);
	auto e5 = buildexpr(myop{},e2,e3);

	dumpexpr(e1);
	dumpexpr(e2);
	dumpexpr(e3);
	dumpexpr(e4);
	dumpexpr(e5);

	auto ii = newvar<int>("ii"), ji = newvar<int>("ji");
	auto ki1 = newconst(1), ki2 = newconst(2);

	auto e6 = buildexpr(myop{},ki1,ki2);
	auto e7 = buildexpr(myop{},k1,k2);
	auto e8 = buildexpr(myop{},e6,e7);
	auto e9 = buildexpr(myop{},e8,ki2);
	auto e10 = buildexpr(myop2{},e7,e6);
	dumpexpr(e6);
	dumpexpr(e7);
	dumpexpr(e8);
	dumpexpr(e9);
	dumpexpr(e10);

	auto e11 = buildexpr(binarychainop<myop,false>{},e6,e7,e6,e10);
	dumpexpr(e11);
	auto e12 = buildexpr(binarychainop<myop,true>{},e6,e7,e6,e10);
	dumpexpr(e12);


	
}

