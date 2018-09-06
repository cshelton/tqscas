#ifndef UTIL_H
#define UTIL_H

#include <vector>
#include <numeric>
#include <iostream>

template<typename T>
struct pickn {
	std::vector<T> x;
	int n;

	std::vector<int> c;

	pickn(std::vector<T> items, int num)
		: x(std::move(items)), n(num), c(num,0) { }

	bool nextpick() {
		for(int i=n-1;i>=0;--i) {
			if (c[i]==x.size()-i-1) {
				c[i] = 0;
				for(int j=i;j<x.size()-1;j++)
					std::swap(x[j],x[j+1]);
			} else {
				c[i]++;
				std::swap(x[i],x[i+c[i]]);
				return true;
			}
		}
		return false;
	}
};

template<typename T, typename F>
struct picknrestr {
	std::vector<T> x;
	int n;

	std::vector<int> c;
	std::vector<int> a;
	F canswap;

	picknrestr(std::vector<T> items, int num, F f)
		: x(std::move(items)), n(num), c(num,0), a(x.size()),
			canswap(std::move(f)) {
		std::iota(a.begin(),a.end(),0);
	}

	bool isvalid(int i, int j) {
		for(;i<j;i++)
			if (a[j]>a[i] && !canswap(x[i],x[j])) return false;
		return true;
	}

	void swapem(int i, int j) {
		std::swap(x[i],x[j]);
		std::swap(a[i],a[j]);
	}

	bool nextpick() {
		for(int i=n-1;i>=0;--i) {
			if (c[i]==x.size()-i-1) {
				c[i] = 0;
				for(int j=i;j<x.size()-1;j++)
					swapem(j,j+1); // put back in order
			} else {
				c[i]++;
				if (isvalid(i,i+c[i])) {
					swapem(i,i+c[i]);
					return true;
				} else {
					swapem(i,i+c[i]);
					++i; // do same lvl again
				}
			}
		}
		return false;
	}
};

template<typename T, typename F>
auto picker(std::vector<T> items, int n, F f) {
	return picknrestr<T,F>(std::move(items),n,std::move(f));
}



#endif
