#include <iostream>
#include <string>

std::string to_string(const std::string &s) { return std::string("\"")+s+"\""; }

#include "sugar.hpp"
#include "typetostr.hpp"

using namespace std;

ostream &operator<<(ostream &os, const std::monostate &) {
     return os << "monostate";
}

template<typename T, typename... Ts>
ostream &operator<<(ostream &os, const variant<T, Ts...> &v) {
     return visit([&os](auto &&x) -> ostream & { return os << x; }, v);
}

template<typename T, typename... Ts>
ostream &writetype(ostream &os, const variant<T, Ts...> &v) {
	return visit([&os](auto &&x) -> ostream & {
			return os << typetostr<std::decay_t<decltype(x)>>(); 
		}, v);
}


int main(int argc, char **argv) {
	auto x = newvar<int>("x"), y = newvar<int>("y");

	auto e1 = x+y;
	cout << draw(e1) << endl;
	cout << typetostr<decltype(e1)>() << endl;
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
	cout << typetostr<decltype(e6)>() << endl;

	auto e7 = pow(newconst(3),newconst(2)) + log(newconst(4.3));
	cout << draw(e7) << endl;
	cout << typetostr<decltype(evalop<defaulttraits>(logop{},2.0))>() << endl;
	cout << typetostr<decltype(evalop<defaulttraits>(logop{},std::string{}))>() << endl;
	cout << eval(e7) << endl;
	writetype(cout,evaltype(e7)) << endl;
	writetype(cout,evaltype(e4)) << endl;

	cout << "-----------" << endl;

	auto s = newvar<string>("s");

	auto ee1 = newconst(std::string("hello")) + std::string(" there");
	auto ee2 = s+std::string(" there");
	cout << draw(ee1) << endl;
	cout << eval(ee1) << endl;
	writetype(cout,evaltype(ee1)) << endl;
	cout << draw(ee2) << endl;
	cout << eval(ee2) << endl;
	writetype(cout,evaltype(ee2)) << endl;

	cout << "-----------" << endl;

	auto f1 = evalat(x,e4,newconst(2));
	cout << draw(f1) << endl;
	auto f2 = evalat(y,f1,newconst(7));
	cout << draw(f2) << endl;
	cout << eval(f2) << endl;

	auto f3 = evalat(y,f1,newconst(7.0));
		// should fail, because 7.0 is not an int!
	cout << draw(f3) << endl;
	cout << eval(f3) << endl;

	auto f4 = buildexpr(evalatop{},y,f1,newconst(7.0)); // can force it!
	cout << draw(f4) << endl;
	cout << eval(f4) << endl; // should fail

	
}
