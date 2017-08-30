#include "scalarrange.hpp"
#include <iostream>

using namespace std;
using rng = scalarset<double>;
using p = limpt<double>;

void print(const rng &s) {
	for(auto i : s.x) {
		cout
			<< (i.first.left ? "" : "!")
			<< (i.first.closed ? '[' : '(')
			<< i.first << ','
			<< i.second
			<< (i.second.closed ? ']' : ')')
			<< (!i.second.left ? "" : "!")
			<< ' ';
	}
	cout << endl;
}

int main(int argc, char **argv) {


	rng a;
	a.x.emplace(p{-1,true},p{1,true});
	a.x.emplace(p{2,false},p{5,true});
	a.x.emplace(p{7,false},p{15,false});
	a.x.emplace(p{-10,true},p{-5,false});

	print(a);

	rng b;
	b.x.emplace(p{-8,true},p{-7,false});
	b.x.emplace(p{-6,false},p{0,true});
	b.x.emplace(p{35,true},std::numeric_limits<double>::infinity());

	print(b);

	print(a-b);
	print(a+b);

	rng c(-8.0);

	print(a-c);
}
	
