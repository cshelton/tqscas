#include "varset.h"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char **argv) {
	constexpr varset<double,string,double> v("a","x","other");

	cout << getarg<double>("b",v,3,"hello",3.2,4.2) << endl;
}
