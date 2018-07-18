#include "typestuff.hpp"
#include <string>
#include <vector>
#include <iostream>

using namespace std;

template<template<typename> typename T, typename V1, typename V2>
void check(const V1 &v1, const V2 &v2) {
	cout << sametypes(v1,v2) << ' ' << sametypeswrap<T>(v1,v2);
	cout << ' ' << sametypes(v2,v1) << ' ' << sametypeswrap<T>(v2,v1) << endl;
}

template<typename T, bool B>
struct XX {
};

template<typename T>
using XXtrue = XX<T,true>;
template<typename T>
using XXfalse = XX<T,false>;

int main(int argc, char **argv) {
	variant<int,char,string> v1a{'a'},v1b{3},v1c{'!'},v1d{4},v1e{'r'},v1f{'s'};
	variant<double,vector<int>,vector<char>,char,XX<char,true>,XX<char,false>> v2a{2.3},v2b{'b'},v2c{'x'},v2d{vector<int>(2)},v2e{XX<char,true>{}},v2f{XX<char,false>{}};

	check<vector>(v1a,v2a);
	check<vector>(v1b,v2b);
	check<vector>(v1c,v2c);
	check<vector>(v1d,v2d);
	check<XXtrue>(v1e,v2e);
	check<XXfalse>(v1e,v2e);
	check<XXtrue>(v1f,v2f);
	check<XXfalse>(v1f,v2f);
}

