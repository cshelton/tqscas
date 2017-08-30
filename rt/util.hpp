#ifndef UTIL_H
#define UTIL_H

#include <vector>

template<typename T>
struct pickn {
	std::vector<T> x;
	int n;

	std::vector<int> c;

	pickn(std::vector<T> items, int num) : x(std::move(items)), n(num), c(num,0) { }

	bool nextpick() {
		if (c.back()==x.size()-n) c.back() = 0;
		else {
			std::swap(x[n-1],x[n+c.back()]);
			c.back()++;
			return true;
		}

		for(int i=c.size()-2;i>=0;i--) {
			if (c[i]==x.size()-i-1) c[i]=0;
			else {
				c[i]++;
				std::swap(x[i],x[i+1]);
				return true;
			}
		}
		return false;
	}
};





#endif
