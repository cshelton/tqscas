#include "symmath.h"
//#include "symmathcalc.h"
#include <functional>

#include <iostream>
#include <string>

using namespace std;

template<typename A, typename B>
void foo(const assignpair<A,B> &) {
	cout << typeid(A).name() << endl;
	cout << typeid(B).name() << endl;
}

template<typename E>
void showsimp(const E &e) {
	auto f = simplify(e);
	e.print(cout); cout << " ====> "; f.print(cout); cout << endl;
}

template<typename E>
void showsimprt(const E &e) {
	auto f = simplify(e);
	e.print(cout); cout << " =rt=> "; f.print(cout); cout << endl;
}

int main(int argc, char **argv) {
	constexpr staticsym<double,'x'> x;
	constexpr staticsym<double,'y'> y;
	constexpr staticsym<double,'z'> z;

	constexpr auto a1 = (x==3.0);
	constexpr auto a2 = (y==1.0);
	constexpr auto f1 = x+x*x+y;
	
	constexpr auto e1 = f1[a1];
	constexpr auto a3 = (z==2.0);
	constexpr auto e2 = e1[a2];
	constexpr auto e3 = e2[a3];
	constexpr double v = e3.val();

	constexpr auto aa0 = (a1 & a2);

	constexpr auto f1a = f1[aa0];
	constexpr auto d = f1a.val();
	cout << d << endl;

	constexpr auto f2 = z*y + x*y + x*z;
	constexpr auto v2 = (f2*f1)[x==3 & y==7 & z==10].val();
	cout << v2 << endl;

	constexpr auto f3 = x+y;
	constexpr auto f4 = y+z*z;
	constexpr auto f5 = f3[x == f4];
	cout << f5[z==5 & y==2].val() << endl;

	staticsym<double,'X'> x2;
	staticsym<double,'Y'> y2;
	auto f = x2+y2;
	auto a = ((x2==3.0) & (y2==1.0));
	cout << f[a].val() << endl;

	dynsym<std::string> s1("s");
	dynsym<std::string> s2("s");
	dynsym<std::string> s3("t");

	auto fs = s1+s2+s3;
	fs.print(cout); cout << endl;
	auto fs2 = fs[s1=="s1 " & s3=="t1"];
	fs2.print(cout); cout << endl;
	auto vs = fs[s1=="s1 " & s3=="t1"].val();

	cout << vs << endl;

	f1.print(cout); cout << endl;
	constexpr auto g1 = (x/(y/y)+x)*y;
	g1.print(cout); cout << endl;
	constexpr auto g2 = -x+ (5-x)+x+(x*4);
	g2.print(cout); cout << endl;
	
	auto b1 = x==f1;
	b1.print(cout); cout << endl;
	auto b2 = b1 & y==g1;
	b2.print(cout); cout << endl;

	x.d(y).print(cout); cout << endl;
	x.d(x).print(cout); cout << endl;
	(x*x).d(x).print(cout); cout << endl;
	(3*x).d(y).print(cout); cout << endl;
	constexpr auto h = 3*x + x*x*4*x/y;
	h.d(y).print(cout); cout << endl;
	constexpr auto hdx = h.d(x);
	showsimp(hdx);
	cout << "--------" << endl;
	constexpr auto tdx = (3*x).d(x);
	showsimp(tdx);
	auto ff = ctconstsymzero<double>{} * x;
	showsimp(ff);
	showsimp(x);
	showsimp(ctconstsymidentity<double>{}*x);
	showsimp(3*x+ctconstsymidentity<double>{}*x);
	showsimp(3*x+1*x);
	showsimp(x+0);
	showsimprt(x+0);
	auto i1 = ctconstsym<int,1>{};
	auto i0 = ctconstsym<int,0>{};
	showsimp(x+i1);
	showsimp(x+i0);
	showsimp(x*i1);
	showsimp(i0*i0);
	cout << typeid(i0*i0).name() << endl;
	showsimp(x*i1+i0*i0+i0*i1+x*(i1+i0)+i0*x);
	showsimprt(x*i1+i0*i0+i0*i1+x*(i1+i0)+i0*x);
	typedef simplifykeeponly<symmultiplies,decltype(x),decltype(i1)> t1;
	typedef simplifykeeponly<symmultiplies,decltype(i1),decltype(x)> t2;
	cout << t1::isadd << ' ' << t1::ismult << ' ' << t1::E1is0 << ' ' << t1::E1is1 << ' ' << t1::E2is0 << ' ' << t1::E2is1 << ' ' << t1::value << endl;
	cout << t2::isadd << ' ' << t2::ismult << ' ' << t2::E1is0 << ' ' << t2::E1is1 << ' ' << t2::E2is0 << ' ' << t2::E2is1 << ' ' << t2::value << endl;
	cout << symtypepairinfo<decltype(x)::range,decltype(i0)::range>::simplifymult0 << iszero<decltype(x)>::value << iszero<decltype(i0)>::value << endl;

}
