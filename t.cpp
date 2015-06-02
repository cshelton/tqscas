#include <utility>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <type_traits>
#include <typeindex>

using namespace std;

struct B {
	float f;
};

struct A {
	int i;
};

struct C {
	int i;
};

template<typename T1, typename T2, typename En=void> struct sort2;

template<typename T1, typename T2>
struct sort2<T1,T2,typename std::enable_if<std::type_index(typeid(T1)) <
					std::type_index(typeid(T2))>::type> {
	typedef std::pair<T1,T2> type;
};

template<typename T1, typename T2>
struct sort2<T1,T2,typename std::enable_if<std::type_index(typeid(T2)) <
					std::type_index(typeid(T1))>::type> {
	typedef std::pair<T2,T1> type;
};

int main(int argc, char **argv) {
/*
	std::cout << std::type_index(typeid(int)).hash_code() << std::endl;
	std::cout << std::type_index(typeid(float)).hash_code() << std::endl;
	std::cout << std::type_index(typeid(A)).hash_code() << std::endl;
	std::cout << std::type_index(typeid(B)).hash_code() << std::endl;
	std::cout << std::type_index(typeid(C)).hash_code() << std::endl;
*/
	std::cout << (std::type_index(typeid(A)) < std::type_index(typeid(B))) << std::endl;
	std::cout << (std::type_index(typeid(B)) < std::type_index(typeid(A))) << std::endl;
	typename sort2<A,B>::type ab;
	//std::cout << typeid(typename sort2<A,B>::type).name() << std::endl;
	//std::cout << typeid(typename sort2<B,A>::type).name() << std::endl;
}
