#include <iostream>
#include <variant>
#include <string>
#include "typetostr.hpp"
#include "typestuff.hpp"

using namespace std;

template<typename T>
struct uninittype {
	using basetype = T;
};

template<typename U>
using tobase_t = typename decay_t<U>::basetype;

struct OP1 {};

struct OP2 {};

auto evalop(const OP1 &, int a, int b) { return a+b; }
auto evalop(const OP1 &, int a, long b) { return a+b; }
auto evalop(const OP1 &, long a, long b) { return a+b; }
auto evalop(const OP1 &, long a, int b) { return a+b; }

auto evalop(const OP2 &, double a, double b) { return a+b; }
auto evalop(const OP2 &, double a, int b) { return a+b; }
auto evalop(const OP2 &, int a, double b) { return a+b; }
auto evalop(const OP2 &, double a, long b) { return a+b; }
auto evalop(const OP2 &, long a, double b) { return a+b; }

template<typename OP, typename T1, typename T2>
monostate evalop(const OP &, T1 &&, T2 &&) { return {}; }

template<typename OP, typename... Ts>
variant<uninittype<Ts>...> evaloptype(const OP &op,
		variant<uninittype<Ts>...> a,
		variant<uninittype<Ts>...> b) {
	return visit([&op](auto &&aa, auto &&bb) -> variant<uninittype<Ts>...> {
			return uninittype<decltype(evalop(op,
					declval<tobase_t<decltype(aa)>>(),
					declval<tobase_t<decltype(bb)>>()
				))>{};},a,b);
}

template<typename ...Ts>
auto varname(const variant<Ts...> &v) {
	//return visit([](auto &&x) { return typeid(tobase_t<decltype(x)>()).name(); },v);
	return visit([](auto &&x)
			{ return typetostr<tobase_t<decltype(x)>()>(); }
		,v);
}

int main(int argc, char **argv) {
	variant<uninittype<monostate>,uninittype<int>,uninittype<long>,uninittype<double>> x,y,z;

	x = uninittype<int>();
	y = uninittype<long>();
	z = uninittype<double>();

	cout << varname(evaloptype(OP1{},x,x)) << endl;
	cout << varname(evaloptype(OP1{},x,y)) << endl;
	cout << varname(evaloptype(OP1{},y,x)) << endl;
	cout << varname(evaloptype(OP1{},y,y)) << endl;
	cout << varname(evaloptype(OP2{},y,z)) << endl;
	cout << varname(evaloptype(OP2{},z,z)) << endl;

	cout << varname(evaloptype(OP1{},y,z)) << endl;

	variant<monostate,int,long,double> a;
	innerwrap_t<uninittype,decltype(a)> b;
	cout << typetostr<decltype(a)>() << endl;
	cout << typetostr<decltype(b)>() << endl;
	cout << typetostr<decltype(x)>() << endl;

}


	
	
		
