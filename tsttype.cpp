#include "typestuff.hpp"
#include <iostream>
#include <vector>
#include <utility>
#include <string>
#include <iomanip>
#include "typetostr.hpp"

using namespace std;

struct opplus {};

struct opminus {};

struct opmult {};

struct opdiv {};

struct invalidopexception {};

template<typename T1, typename T2>
auto apply(const opplus &o, T1 x, T2 y) { return x+y; }
template<typename T1, typename T2>
auto apply(const opminus &o, T1 x, T2 y) { return x-y; }
template<typename T1, typename T2>
auto apply(const opmult &o, T1 x, T2 y) { return x*y; }
/*
template<typename T1, typename T2>
auto apply(const opdiv &o, T1 x, T2 y) { return x/y; }
*/
auto apply(const opdiv &o, int x, int y) { return x/y; }

template<typename O, typename T1, typename T2>
auto apply(const O &, T1, T2) -> T1 { throw invalidopexception(); }

template<typename A, typename B>
using vpair = pair<typename unpackto<variant,A>::type,typename unpackto<variant,B>::type>;

int main(int argc, char **argv) {
	constexpr variant<int,char,long> v1(2),v2('c');

	constexpr bool isint = istype<int>(v1);

	cout << istype<int>(v1) << ' ' << istype<char>(v1) << ' ' << istype<long>(v1) << endl;
	cout << istype<int>(v2) << ' ' << istype<char>(v2) << ' ' << istype<long>(v2) << endl;

	using vt = variant<double,int>;

	vector<vt> x{3.5,2,3};
	vector<vt> y{1.5,0,2};
	vector<variant<opplus,opminus,opmult,opdiv>> o{opplus{},opminus{},opdiv{}};

	for(int i=0;i<x.size();i++)
		visit([](auto o, auto x, auto y){ cout << apply(o,x,y) << endl; },
				o[i],x[i],y[i]);

	vpair<tuple<int,long,double>,tuple<int,char,float>> vp;
	vp.first = 2;
	vp.second = 'c';
	visit([](auto a, auto b) { cout << a << " & " << b << endl; },vp.first,vp.second);

	typedef variant<std::monostate> vvoid;
	vvoid vv;

	using variant1 = variant<int,char>;
	using variant2 = variant<double,string>;

	using variant12 = variantunion_t<variant1,variant2>;

	vector<variant12> v12s;
	v12s.emplace_back(1);
	v12s.emplace_back(3.4);
	v12s.emplace_back("hello"s);

	cout << typetostr<variant12>() << endl;

	using variant3 = variant<int,string,float>;

	using variant112 = variantunion_t<variant1,variant12>;
	cout << typetostr<variant112>() << endl;

	using variant1123 = variantunion_t<variant112,variant3>;
	cout << typetostr<variant1123>() << endl;

	for(auto &x : v12s) 
		visit([](auto a) { cout << a << endl; },x);

	int i = gettype<int>(v12s[0]);
	cout << i << endl;
	double d = gettype<double>(v12s[1]);
	cout << d << endl;
	cout << "got here" << endl;
	double dfail = gettype<double>(v12s[0]);
	cout << "should not got here" << endl;
	cout << dfail << endl;

}

