#ifndef SCALARRANGE_H
#define SCALARRANGE_H

#include <vector>
#include <utility>
#include <limits>
#include <cmath>
#include <set>
#include <tuple>

template<typename T>
struct limpt {
	T pt;
	bool left, closed;

	limpt() {}
	limpt(const T &v) : pt(v), closed(std::isfinite(v)), left(true) {}
	limpt(const T &v, bool l) : pt(v), left(l), closed(std::isfinite(v)) {}
	limpt(const T &v, bool l, bool c) : pt(v), left(l), closed(c) {}
	limpt(const limpt &) = default;
	limpt(limpt &&) = default;
	limpt &operator=(const limpt &) = default;
	limpt &operator=(limpt &&) = default;

	limpt(const limpt &p, bool l) : pt(p.pt), closed(p.closed), left(l) {}
	limpt(limpt &&p, bool l) : pt(std::move(p.pt)),
			closed(p.closed), left(l) {}

	operator T() const {
		return pt;
	}

	limpt operator+(const limpt &p) const {
		limpt ret(*this);
		ret.pt += p;
		ret.closed &= p.closed;
		ret.left &= p.left; // just to make symmetric
		return ret;
	}

	limpt operator*(const limpt &p) const {
		limpt ret(*this);
		ret.pt *= p;
		ret.closed &= p.closed;
		ret.left &= p.left; // just to make symmetric
		return ret;
	}

	limpt operator-() const {
		limpt ret(*this);
		ret.pt = -ret.pt;
		ret.left = !ret.left;
		return ret;
	}

	limpt flip() const {
		limpt ret(*this);
		ret.left = !ret.left;
		ret.closed = !ret.closed;
		return ret;
	}

	int cmp2(const limpt &l) const {
		int mt = closed ? 0 : (left ? 1 : -1);
		int ml = l.closed ? 0 : (l.left ? 1 : -1);
		return mt==ml ? 0 : (mt<ml ? -1 : 1);
	}

	int cmp(const limpt &l) const {
		if (pt==l.pt) return cmp2(l);
		return pt<l.pt ? -1 : 1;
	}

	bool operator<(const limpt &l) const {
		return cmp(l)<0;
	}
	bool operator<=(const limpt &l) const {
		return cmp(l)<=0;
	}
	bool operator>(const limpt &l) const {
		return cmp(l)>0;
	}
	bool operator>=(const limpt &l) const {
		return cmp(l)>=0;
	}
	bool operator==(const limpt &l) const {
		return cmp(l)==0;
	}
	bool operator!=(const limpt &l) const {
		return cmp(l)!=0;
	}
	bool operator<(const T &l) const {
		return cmp(limpt{l})<0;
	}
	bool operator<=(const T &l) const {
		return cmp(limpt{l})<=0;
	}
	bool operator>(const T &l) const {
		return cmp(limpt{l})>0;
	}
	bool operator>=(const T &l) const {
		return cmp(limpt{l})>=0;
	}
	bool operator==(const T &l) const {
		return cmp(limpt{l})==0;
	}
	bool operator!=(const T &l) const {
		return cmp(limpt{l})!=0;
	}
};
template<typename T>
	bool operator<(const T &l, const limpt<T> &l2)  {
		return limpt<T>{l}.cmp(l2)<0;
	}
template<typename T>
	bool operator<=(const T &l, const limpt<T> &l2)  {
		return limpt<T>{l}.cmp(l2)<=0;
	}
template<typename T>
	bool operator>(const T &l, const limpt<T> &l2)  {
		return limpt<T>{l}.cmp(l2)>0;
	}
template<typename T>
	bool operator>=(const T &l, const limpt<T> &l2)  {
		return limpt<T>{l}.cmp(l2)>=0;
	}
template<typename T>
	bool operator==(const T &l, const limpt<T> &l2)  {
		return limpt<T>{l}.cmp(l2)==0;
	}
template<typename T>
	bool operator!=(const T &l, const limpt<T> &l2)  {
		return limpt<T>{l}.cmp(l2)!=0;
	}

template<typename T>
limpt<T> pow(const limpt<T> &b, const limpt<T> &e) {
	auto retpt = pow(b.pt,e.pt);
	return limpt<T>{retpt,b.closed && e.closed};
}

template<typename T>
struct range : std::pair<limpt<T>,limpt<T>> {
	template<typename R1,typename R2>
	range(R1 l, R2 r)
		: std::pair<limpt<T>,limpt<T>>(std::piecewise_construct,
				std::forward_as_tuple(l<r ? limpt<T>(l) : limpt<T>(r),true),
				std::forward_as_tuple(l<r ? limpt<T>(r) : limpt<T>(l),false))
			{}

	range() {}

	bool overlap(const range &r) const {
		return !(r.second<this->first || this->second<r.first);
	}

	// returns number of pieces.  If -1, *this remains the same
	// (and nothing is placed in r1 or r2)
	int subtract(const range &r, range &r1, range &r2) const {
		if (r.first<=this->first) {
			if (r.second<this->first) return -1;
			if (r.second>=this->second) return 0;
			r1.second = this->second;
			r1.first = r.second.flip();
			return 1;
		} else if (r.first<=this->second) {
			r1.first = this->first;
			r1.second = r.first.flip();
			if (r.second>=this->second) {
				return 1;
			} else {
				r2.second = this->second;
				r2.first = r.second.flip();
				return 2;
			}
		} else return -1;
	}


};

template<typename T>
struct scalarset {
	std::set<range<T>> x;

	scalarset() {}

	scalarset(T pt) : x{range<T>{pt,pt}} {}
	scalarset(T pt1, T pt2) : x{range<T>{pt1,pt2}} {}

	void compress() {
		for(auto i=x.begin();i!=x.end();++i) {
			auto j=i;
			for(++j;j!=x.end();++j) {
				if (j->first>i->second) break;
				else if (j->second>i->second) {
					auto r = *i;
					r.second = j->second;
					if (i==x.begin()) {
						x.erase(i);
						x.emplace(r);
						i = x.begin();
					} else {
						auto i0 = i;
						--i0;
						x.erase(i);
						x.emplace(r);
						i = i0;
						++i;
					}
				}
				x.erase(j);
				j = i;
			}
		}
	}

	template<typename F> // F takes two ranges and returns a range
	scalarset combine(const scalarset &s, F fn) const {
		scalarset ret;
		for(auto &a : x)
			for(auto &b : s.x)
				ret.x.emplace(fn(a,b));
		ret.compress();
		return ret;
	}

	template<typename F> // F takes two ranges and returns a scalarset
	scalarset combinemult(const scalarset &s, F fn) const {
		scalarset ret;
		for(auto &a : x)
			for(auto &b : s.x)
				for(auto &c : fn(a,b).x)
					ret.x.emplace(c);
		ret.compress();
		return ret;
	}

	template<typename F> // F takes two ranges and returns a range
	scalarset modify(F fn) const {
		scalarset ret;
		for(auto &a : x)
			ret.x.emplace(fn(a));
		ret.compress();
		return ret;
	}

};

template<typename T>
scalarset<T> operator+(const scalarset<T> &a, const scalarset<T> &b) {
	scalarset<T> ret(a);
	for(auto r : b.x)
		ret.x.emplace(r);
	ret.compress();
	return ret;
}

template<typename T>
scalarset<T> operator-(const scalarset<T> &a, const scalarset<T> &b) {
	scalarset<T> ret(a);
	range<T> r1,r2;
	int n;
	for(auto i = ret.x.begin();i!=ret.x.end();++i)
		for(auto j = b.x.begin();j!=b.x.end();++j)
			if(j->first>i->second) break;
			else if((n = i->subtract(*j,r1,r2))!=-1) {
				if (i==ret.x.begin()) {
					ret.x.erase(i);
					ret.x.emplace(r1);
					i = ret.x.begin();
				} else {
					auto i0 = i;
					--i0;
					ret.x.erase(i);
					ret.x.emplace(r1);
					i = i0;
					++i;
				}
				if (n==2) ret.x.emplace(r2);
			}
	return ret;
}

#endif
