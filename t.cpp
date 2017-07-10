#include <utility>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <type_traits>

using namespace std;

template<char... N>
struct name {
	constexpr const char *value() const {
		return (const char[]){N...,0};
	}
};

int main(int argc, char **argv) {
	cout << name<'a','b','!'>().value() << endl;
	name<'a','b','!'> n;
	cout << n.value() << endl;
	cout << ((const char[]){'a','b','!',0}) << endl;
	
}
