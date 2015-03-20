#include <utility>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <type_traits>

using namespace std;

int global=4;

template<typename T>
struct A {
	template<typename U=T>
	typename std::enable_if<std::is_same<U,int>::value,T &>::type get() const { return global; }
	template<typename U=T>
	typename std::enable_if<!std::is_same<U,int>::value,bool>::type get() const { return false; }

};

int main(int argc, char **argv) {
	A<int> a;
	cout << a.get() << endl;
	A<void> b;
	cout << b.get() << endl;
	
}
