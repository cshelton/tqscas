#include <iostream>

using namespace std;

template<typename T>
struct A {
	T t;
	constexpr A() {};
	constexpr A(T tt) : t(move(tt)) {}
};


template<typename T>
constexpr A<T> makeit(T f) {
	return {move(f)};
}

template<typename T>
constexpr A<T> a0 = makeit((T)65);

int main(int argc, char **argv) {

	cout << a0<int>.t << endl;
	cout << a0<char>.t << endl;
}


