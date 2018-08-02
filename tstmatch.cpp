#include "exprbasicops.hpp"
#include "exprmatch.hpp"
#include "exprsugar.hpp"
#include <iostream>

using namespace std;

template<typename E, typename P>
void checkit(const E &e, const P &p) {
	auto m = match(e,p);
	if (m) {
		cout << to_string(e) << " matches with " << to_string(p) << ": " << endl;
		for(auto &mm : *m)
			cout << "\t" << mm.first << ": " << to_string(mm.second) << endl;
	} else {
		cout << to_string(e) << " does NOT match with " << to_string(p) << ": " << endl;
	}
}

int main(int argc, char **argv) {
	auto x = newvar<double>("x"), y = newvar<double>("y"), z = newvar<double>("z");
	auto e1 = x+z;
	auto e2 = z+x;
	auto e3 = 3*x + pow(x,y) + log(e1+5*y);

	auto m1 = x+z;
	auto m2 = 3*x + pow(x,y) + log(e1+5*y);
	auto m3 = E1_ + E2_;
	checkit(e1,m1);
	checkit(e2,m1);
	checkit(e3,m1);
	checkit(e1,m2);
	checkit(e2,m2);
	checkit(e3,m2);
	checkit(e1,m3);
	checkit(e2,m3);
	checkit(e3,m3);

}
