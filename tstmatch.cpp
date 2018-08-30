#include "basicops.hpp"
#include "match.hpp"
#include "sugar.hpp"
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

	cout << "-------" << endl;

	auto m4 = buildexpr(matchcommop<addop>{},x,z);
	auto m5 = L(11,buildexpr(matchcommop<addop>{},x,E4_));
	checkit(e1,m4);
	checkit(e2,m4);
	checkit(e3,m4);
	checkit(e1,m5);
	checkit(e2,m5);
	checkit(e3,m5);

	auto e4 = buildexpr(binarychainop<addop>{},newconst(4),x,newconst(8),z*5);
	auto e5 = buildexpr(binarychainop<addop>{},x,newconst(4),newconst(8));
	auto e6 = buildexpr(binarychainop<addop>{},newconst(4),x);
	auto m6 = buildexpr(matchcommop<binarychainop<addop>>{},x,newconst(4),newconst(8),z*5);
	auto m7 = buildexpr(matchcommop<binarychainop<addop>>{},x,newconst(4));
	auto m8 = buildexpr(matchremainderop<binarychainop<addop>>{},newconst(4),x);
	auto m9 = buildexpr(matchremainderop<binarychainop<addop>>{},newconst(4),x,E3_);
	auto m10 = buildexpr(matchcommop<binarychainop<addop>,true>{},x,newconst(4),E5_);

	cout << "-------" << endl;

	checkit(e1,m6);
	checkit(e4,m6);
	checkit(e5,m6);
	checkit(e6,m6);
	checkit(e1,m7);
	checkit(e4,m7);
	checkit(e5,m7);
	checkit(e6,m7);
	checkit(e1,m8);
	checkit(e4,m8);
	checkit(e5,m8);
	checkit(e6,m8);
	checkit(e1,m9);
	checkit(e4,m9);
	checkit(e5,m9);
	checkit(e6,m9);
	checkit(e1,m10);
	checkit(e4,m10);
	checkit(e5,m10);
	checkit(e6,m10);

}
