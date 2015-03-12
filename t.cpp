#include <utility>
#include <stdexcept>
#include <iostream>

using namespace std;

constexpr int fac(int i) {
	return i<0 ? throw std::logic_error("i must be non-negative") : (i==0 ? 1 : i*fac(i-1));
}

int main(int argc, char **argv) {
	constexpr int out = fac(4);
	cout << out << endl;
}
