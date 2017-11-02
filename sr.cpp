#include "exprrewrite.hpp"
#include "exprtostr.hpp"
#include "scalarbase.hpp"
#include "scalarrewrite.hpp"
#include <iostream>

using namespace std;

void printmatch(const expr &e, const expr &pat) {
	auto m = match(e,pat);
	cout << tostring(e) << " <=?=> " << tostring(pat) << "? ";
	if (m) {
		cout << "yes" << endl;
		for(auto &p : *m)
			cout << "\t" << p.first << ": " << tostring(p.second) << endl;
	} else
		cout << "no" << endl;
}

void printrewrite(const expr &e, const std::vector<ruleptr> &rules) {
	cout << tostring(e) << " ==> " << tostring(rewrite(e,rules)) << endl;
}

int main(int argc, char **argv) {
	expr x = newvar<double>("x");
	expr y = newvar<double>("y");
	expr z = newvar<double>("z");

	expr e1 = x+y+z;
	cout << draw(e1) << endl;

	expr e2 = x-y;
	cout << draw(e2) << endl;

	expr e3 = x+1;
	expr e4 = 1+x;

	expr p1 = V_ + V_;
	cout << draw(p1) << endl;

	expr p2 = chainpatternmod(L(1,E_) + L(2,E_));
	cout << draw(p2) << endl;

	expr p3 = chainpatternmod(E_ + E_);
	cout << draw(p3) << endl;

	expr p4 = chainpatternmod(1+ L(1,E_));

	printmatch(e1,p1);
	printmatch(e2,p1);
	printmatch(e1,p2);
	printmatch(e2,p2);
	printmatch(e1,p3);
	printmatch(e2,p3);

	printmatch(e3,p4);
	printmatch(e4,p4);
	
	//------

	expr a1 = x+0;
	expr a2 = x+y+0;
	expr a3 = x/y + 0 + z;

	std::vector<ruleptr> rules = {{SR(chainpatternmod(0+L(1,E_)), P(1))}};

	printrewrite(a1,rules);
	printrewrite(a2,rules);
	printrewrite(a3,rules);

	expr a4 = x*y + x*y;
	expr a5 = x*y + x;

	std::vector<ruleptr> rules2 = {{SR(chainpatternmod(E1_+E1_), 2*P1_)}};
	printrewrite(a4,rules2);
	printrewrite(a5,rules2);
}
	
