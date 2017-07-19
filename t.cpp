#include "symmath.h"
#include <iostream>


using namespace std;

int main(int argc, char **argv) {
	constexpr auto v1 = 1_c;
	constexpr auto v10 = 10_c;
	constexpr auto v11 = staticop<plusop,decltype(v1),decltype(v10)>{};
	cout << v1 << endl;
	cout << v10 << endl;
	cout << v11 << endl;
}
	
