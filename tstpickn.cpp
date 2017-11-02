#include "util.hpp"
#include <iostream>
#include <numeric>

using namespace std;

int main(int argc, char **argv) {
	int m = argc<2 ? 6 : atoi(argv[1]), n = argc<3 ? 2 : atoi(argv[2]);

	vector<int> x(m);
	iota(x.begin(),x.end(),0);
	pickn<int> p(x,n);
	do {
		for(int i=0;i<n;i++)
			cout << p.x[i] << ' ';
		cout << "|";
		for(int i=n;i<m;i++)
			cout << ' ' << p.x[i];
		cout << endl;
	} while(p.nextpick());
}
