#include <typeinfo>
#include <iostream>
#include <variant>
#include <tuple>
#include <vector>
#include "typestuff.hpp"
#include <string>
#include <cmath>

struct A {};
struct B {};
struct C {};

template<typename T, typename... Ts>
bool allsame(T, Ts &&...) {
	return (... && std::is_same_v<T,Ts>);
}

template<typename T, typename... Ts>
bool allsameb(T a, Ts &&... aa) {
	return (... && (a==aa));
}

bool mynot(bool b) { return !b; }

template<typename... Ts>
bool test(Ts &&...ts) {
	//return allsameb(1,ts...);
	return allsameb(1,mynot(ts)...);
}

auto to_string(const A &a) {
	return std::string("A");
}
auto to_string(const B &b) {
	return std::string("B");
}
auto to_string(const C &c) {
	return std::string("C");
}

template<typename T, typename = void>
struct hasstdtostring : std::false_type {};

template<typename T>
struct hasstdtostring<T,std::void_t<
		decltype(std::to_string(std::declval<T>()))>> : std::true_type {};

template<typename T>
inline constexpr bool hasstdtostring_v = hasstdtostring<T>::value;

template<typename T>
std::string tostring(const T &x) {
	if constexpr (hasstdtostring_v<T>) return std::to_string(x);
	else return to_string(x);
}

void checkrem(int a, int b) {
	std::cout << a << '%' << b << '=' << (a%b) << std::endl;
	std::cout << a << 'r' << b << '=' << std::remainder(a,b) << std::endl;
}

int main(int argc, char **argv) {
	using namespace std;
	cout << is_same_v<A,B> << ' ' << is_same_v<B,C> << endl;
	cout << allsame(A{},A{},A{}) << ' ' << allsame(A{},A{},A{},B{}) << endl;
	cout << allsameb(1,1,1,1) << ' ' << allsameb(1,1,2,1) << endl;

	cout << "----" << endl;
	cout << test(1,1,1,1) << ' ' << test(1,1,0,1) <<  ' ' << test(0,0,0,0) << endl;

	cout << istmpl_v<std::vector,std::vector<int>> << endl;
	cout << istmpl_v<std::variant,std::variant<int,bool,double>> << endl;

	cout << tostring(3) << ' ' << tostring(5.4) << ' ' << tostring(C{}) << ' ' << tostring(A{}) << endl;

	checkrem(5,3);
	checkrem(5,-3);
	checkrem(-5,3);
	checkrem(-5,-3);
	checkrem(6,3);
	checkrem(6,-3);
	checkrem(-6,3);
	checkrem(-6,-3);
	checkrem(7,3);
	checkrem(7,-3);
	checkrem(-7,3);
	checkrem(-7,-3);

}
