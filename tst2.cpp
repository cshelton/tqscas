#include <iostream>
#include <tuple>

using namespace std;

int add(int a) { return a; }
int add(int a1, int a2) { return a1+a2; }
int add(int a1, int a2, int a3) { return a1+a2+a3; }
int add(int a1, int a2, int a3, int a4) { return a1+a2+a3+a4; }

int square(int a) { return a*a; }

template<typename... T>
int addsquares(T... as) {
	return add(square(as)...);
}

int main(int argc, char **argv) {
	cout << addsquares(3,4,5) << endl;
}
