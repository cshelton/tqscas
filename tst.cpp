#include "mathfn.h"
#include "parsefn.h"

#include <iostream>

using namespace std;

int main(int argc, char **argv) {
	//auto f = 1 + 5.22_fn + -1;
	//auto f2 = 6_fn;
	//auto f3 = 7_fn;

	//int ans = ((f+f2)|f3)(3);
	auto f = x_fn<double>() * 3;
	//auto v = f(4);
	//cout << v << endl;
	
	//cout << std::is_void<typename decltype(f)::domain>::value << endl;

	auto g = x_fn<double>();
	auto gg = g^g;
	for(int i=0;i<5;i++)
		cout << i << ": " << gg(i) << endl;
}
