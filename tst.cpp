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

	constexpr dynsym<std::string> s1("s");
	constexpr dynsym<std::string> s2("s");
	constexpr dynsym<std::string> s3("t");

	constexpr auto fs = s1+s2+s3;
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
	hdx.print(cout); cout << endl;
	cout << "simplified: "; simplify(hdx).print(cout); cout << endl;
	cout << "--------" << endl;
	constexpr auto tdx = (3*x).d(x);
	tdx.print(cout); cout << endl;
	constexpr auto shdx = simplify(tdx);
	cout << "simplified: "; shdx.print(cout); cout << endl;
	auto ff = ctconstsymzero<double>{} * x;
	ff.print(cout); cout << endl;
	cout << "simplified: "; simplify(ff).print(cout); cout << endl;
	cout << "simplified: "; simplify(x).print(cout); cout << endl;
	cout << "simplified: "; simplify(ctconstsymidentity<double>{}*x).print(cout); cout << endl;
	cout << "simplified: "; simplify(3*x+ctconstsymidentity<double>{}*x).print(cout); cout << endl;
	cout << "simplified: "; simplify(3*x+1*x).print(cout); cout << endl;
	//cout << typeid(derivativetype<double,decltype(x)>::type) << endl;
	//cout << typeid(derivativetype<double,double>::type).name() << endl;
	//cout << typeid(derivativetype<double,mathexpr<decltype(x),double>>::type).name() << endl;
	

}
