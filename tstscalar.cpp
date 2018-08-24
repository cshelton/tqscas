#include "exprscalarops.hpp"
#include "exprsugar.hpp"

#include <iostream>

using namespace std;

ostream &operator<<(ostream &os, const std::monostate &) {
	return os << "monostate";
}

template<typename T, typename... Ts>
ostream &operator<<(ostream &os, const variant<T, Ts...> &v) {
	return visit([&os](auto &&x) -> ostream & { return os << x; }, v);
}


template<typename E, typename V>
void checkderiv(const E &e, const V &v, const vector<double> &vs) {
	cout << "f = " << to_string(e) << endl;
	for(auto &vv : vs) {
		auto val = newconst(vv);
		auto de = deriv(v,e,val);
		auto ee = evalat(v,e,val);
		cout << "@ " << to_string(v) << " = " << to_string(val) << endl;
		cout << "f =  " << eval(ee) << endl;
		cout << "f' = " << eval(de) << endl;
	}
}
	

int main(int argc, char **argv) {
	auto x = newvar<double>("x");

	auto e1 = x*x*x+4.0*x*x+x/x;
	checkderiv(e1,x,vector<double>{-1.0,0.0,1.0,40.0});
	checkderiv(x*0.5*x*x-2.0*x/(x-4.0),x,vector<double>{-4,-1,0,1,3,4,5});
	//checkderiv(x*0.5*x*x-2.0*x/(x-4.0),x,vector<double>{0});
}
	
