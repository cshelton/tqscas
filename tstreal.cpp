#include "scalarbase.hpp"
#include <iostream>

using namespace std;

int main(int argc, char **argv) {

	scalarreal s0(0),s1(1),s2(2),s3(3),s4(4);

	auto s5 = s0/s1;
	auto s6 = s0+s1+s2+s3/s4;
	auto s7 = pow(s2,s3);
	auto s8 = pow(s2/s3,-s4);
	auto s9 = pow(s1/s4,-s3);

	cout << tostring(s0) << endl;
	cout << tostring(s1) << endl;
	cout << tostring(s2) << endl;
	cout << tostring(s3) << endl;
	cout << tostring(s4) << endl;

	cout << tostring(s5) << endl;
	cout << tostring(s6) << endl;
	cout << tostring(s7) << endl;
	cout << tostring(s8) << endl;
	cout << tostring(s9) << endl;

	scalarreal s10{scalarreal::eulerconst()}, s11{scalarreal::piconst()};

	cout << tostring(s10) << endl;
	cout << tostring(s11) << endl;

	cout << tostring(pow(s11,scalarreal{2})) << endl;
	cout << tostring(pow(s11,scalarreal{0})) << endl;

	cout << endl;

	cout << "round" << tostring((scalarreal{1,4})) << " to " << tostring((scalarreal{1,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{3,4})) << " to " << tostring((scalarreal{3,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{4,4})) << " to " << tostring((scalarreal{4,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{5,4})) << " to " << tostring((scalarreal{5,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{6,4})) << " to " << tostring((scalarreal{6,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{7,4})) << " to " << tostring((scalarreal{7,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{-1,4})) << " to " << tostring((scalarreal{-1,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{-3,4})) << " to " << tostring((scalarreal{-3,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{-4,4})) << " to " << tostring((scalarreal{-4,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{-5,4})) << " to " << tostring((scalarreal{-5,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{-6,4})) << " to " << tostring((scalarreal{-6,4}).round()) << endl;
	cout << "round" << tostring((scalarreal{-7,4})) << " to " << tostring((scalarreal{-7,4}).round()) << endl;

	cout << "floor" << tostring((scalarreal{1,4})) << " to " << tostring((scalarreal{1,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{3,4})) << " to " << tostring((scalarreal{3,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{4,4})) << " to " << tostring((scalarreal{4,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{5,4})) << " to " << tostring((scalarreal{5,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{6,4})) << " to " << tostring((scalarreal{6,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{7,4})) << " to " << tostring((scalarreal{7,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{-1,4})) << " to " << tostring((scalarreal{-1,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{-3,4})) << " to " << tostring((scalarreal{-3,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{-4,4})) << " to " << tostring((scalarreal{-4,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{-5,4})) << " to " << tostring((scalarreal{-5,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{-6,4})) << " to " << tostring((scalarreal{-6,4}).floor()) << endl;
	cout << "floor" << tostring((scalarreal{-7,4})) << " to " << tostring((scalarreal{-7,4}).floor()) << endl;

	cout << "ceil" << tostring((scalarreal{1,4})) << " to " << tostring((scalarreal{1,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{3,4})) << " to " << tostring((scalarreal{3,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{4,4})) << " to " << tostring((scalarreal{4,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{5,4})) << " to " << tostring((scalarreal{5,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{6,4})) << " to " << tostring((scalarreal{6,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{7,4})) << " to " << tostring((scalarreal{7,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{-1,4})) << " to " << tostring((scalarreal{-1,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{-3,4})) << " to " << tostring((scalarreal{-3,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{-4,4})) << " to " << tostring((scalarreal{-4,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{-5,4})) << " to " << tostring((scalarreal{-5,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{-6,4})) << " to " << tostring((scalarreal{-6,4}).ceil()) << endl;
	cout << "ceil" << tostring((scalarreal{-7,4})) << " to " << tostring((scalarreal{-7,4}).ceil()) << endl;
}
