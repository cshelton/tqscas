#include <iostream>
#include "typeset.h"
#include "typetostr.h"

using namespace std;

int main(int argc, char **argv) {
	using A=typeset<int>;
	using B=typeset<int,char>;
	using C=typeset<int,char,double>;

	cout << typesethas<A,int>::value << ' ' << typesethas<A,int>::only << endl;
	cout << typesethas<A,char>::value << ' ' << typesethas<A,char>::only << endl;
	cout << typesethas<A,double>::value << ' ' << typesethas<A,double>::only << endl;
	cout << typesethas<A,A>::value << ' ' << typesethas<A,A>::only << endl;
	cout << typesethas<B,int>::value << ' ' << typesethas<B,int>::only << endl;
	cout << typesethas<B,char>::value << ' ' << typesethas<B,char>::only << endl;
	cout << typesethas<B,double>::value << ' ' << typesethas<B,double>::only << endl;
	cout << typesethas<B,A>::value << ' ' << typesethas<B,A>::only << endl;
	cout << typesethas<C,int>::value << ' ' << typesethas<C,int>::only << endl;
	cout << typesethas<C,char>::value << ' ' << typesethas<C,char>::only << endl;
	cout << typesethas<C,double>::value << ' ' << typesethas<C,double>::only << endl;
	cout << typesethas<C,A>::value << ' ' << typesethas<C,A>::only << endl;

	cout << endl;
	cout << typetostr<typename commontype<A>::type>() << endl;
	cout << typetostr<typename commontype<B>::type>() << endl;
	cout << typetostr<typename commontype<C>::type>() << endl;
}
