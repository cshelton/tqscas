#include "util.hpp"
#include <iostream>
#include <numeric>
#include <algorithm>

using namespace std;

template<typename P>
void show(P &p, int n, int m) {
	do {
		for(int i=0;i<n;i++)
			cout << p.x[i] << ' ';
		cout << "|";
		for(int i=n;i<m;i++)
			cout << ' ' << p.x[i];
		cout << endl;
	} while(p.nextpick());
}

template<typename F>
void tryrestr(F f) {
	cout << "-----------------------" << endl;
	vector<char> y(6);
	iota(y.begin(),y.end(),'A');
	auto p2 = picker(y,3,std::move(f));
	show(p2,3,6);
}

int main(int argc, char **argv) {
	int m = argc<2 ? 6 : atoi(argv[1]), n = argc<3 ? 2 : atoi(argv[2]);

	vector<int> x(m);
	iota(x.begin(),x.end(),0);
	pickn<int> p(x,n);
	show(p,n,m);

	//-----

	vector<char> y(6);
	iota(y.begin(),y.end(),'A');
	pickn<char> pp(y,3);
	show(pp,3,6);

	tryrestr([](char a, char b) {
			return true;
		});

	tryrestr([](char a, char b) {
			static const char l[] = {'B','C','D','E'};
			return find(begin(l),end(l),a)==end(l)
				|| find(begin(l),end(l),b)==end(l);
		});
	tryrestr([](char a, char b) {
			static const char l[] = {'B','D','E'};
			return find(begin(l),end(l),a)==end(l)
				|| find(begin(l),end(l),b)==end(l);
		});
}
