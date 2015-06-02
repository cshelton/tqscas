#ifndef CTSTRING_H
#define CTSTRING_H

#include <string>
#include <cstring>

template<char... N>
struct ctstring {
};

template<char... N> struct ctstringcmp;

template<typename A, typename B>
struct ctstringcmp2;

template<char N0, char... N, char M0, char... M>
struct ctstringcmp2<ctstring<N0,N...>,ctstring<M0,M...>> {
	constexpr static int cmp() {
		return N0==M0 ?  ctstringcmp2<ctstring<N...>,ctstring<M...>>::cmp()
				: N0-M0;
	}
};
template<char M0, char... M>
struct ctstringcmp2<ctstring<>,ctstring<M0,M...>> {
	constexpr static int cmp() {
		return -M0;
	}
};
template<char N0, char... N>
struct ctstringcmp2<ctstring<N0,N...>,ctstring<>> {
	constexpr static int cmp() {
		return N0;
	}
};
template<>
struct ctstringcmp2<ctstring<>,ctstring<>> {
	constexpr static int cmp() {
		return 0;
	}
};


template<char N0, char...N>
struct ctstringcmp<N0,N...> {
	constexpr static int cmp(const char *s) {
		return (*s) ?
				(N0==*s ? ctstringcmp<N...>::cmp(s+1) : N0-*s) : N0;
	}
	constexpr static int cmp(const std::string &s) {
		return cmp(s.c_str());
	}
	template<char... M>
	constexpr static int cmp(const ctstring<M...> &s) {
		return ctstringcmp2<ctstring<N0,N...>,ctstring<M...>>::cmp();
	}
};
template<>
struct ctstringcmp<> {
	constexpr static int cmp(const char *s) {
		return (*s) ? -(*s) : 0;
	}
	static int cmp(const std::string &s) {
		return cmp(s.c_str());
	}
	template<char... M>
	constexpr static int cmp(const ctstring<M...> &s) {
		return ctstringcmp2<ctstring<>,ctstring<M...>>::cmp();
	}
};
		
		

std::string nametostring(const std::string &s) { return s; }
std::string nametostring(std::string &&s) { return std::move(s); }

std::string nametostring(const char *s) { return std::string(s); }

template<char... N>
std::string nametostring(const ctstring<N...> &s) {
	static const char n[] = {N...,0};
	return n;
}

int cmpnames(const std::string &s1, const std::string &s2) {
	return strcmp(s1.c_str(),s2.c_str());
}
int cmpnames(const std::string &s1, const char *s2) {
	return strcmp(s1.c_str(),s2);
}
int cmpnames(const char *s1, const std::string &s2) {
	return strcmp(s1,s2.c_str());
}
int cmpnames(const char *s1, const char *s2) {
	return strcmp(s1,s2);
}
template<typename T, char... N>
int cmpnames(const T &s1, const ctstring<N...> &s2) {
	return -ctstringcmp<N...>::cmp(s1);
}
template<typename T, char... N>
int cmpnames(const ctstring<N...> &s1, const T &s2) {
	return ctstringcmp<N...>::cmp(s2);
}
template<char... N, char... M>
int cmpnames(const ctstring<N...> &s1, const ctstring<M...> &s2) {
	return ctstringcmp<N...>::cmp(s2);
}

#endif
