#include "expr.hpp"
#include <string>
#include "scalar.hpp"
#include "exprtostr.hpp"

using namespace std;

using E = expr;

string writenode(char c, vector<string> ch) {
	string ret(1,c);
	ret += '(';
	bool f = true;
	for(auto &s : ch) {
		if (!f) ret += string(1,',');
		f = false;
		ret += s;
	}
	ret += ')';
	return ret;
}

string writeleaf(int i) {
	return to_string(i);
}

template<typename T1, typename T2>
auto myfn(T1 a, T2 b) {
	auto c = a+b;
	auto d = a*b;
	return c-d/(a-b);
}

void showsimp(const expr &e) {
	cout << "--------------------" << endl;
	cout << tostring(e) << ": " << endl;
	cout << draw(e) << " to " << endl;
	auto ee = rewrite(e,scalarsimprules);
	cout << tostring(ee) << ": " << endl;
	cout << draw(ee) << endl;
}

void showsimp(const expr &e, const vector<expr> &vs) {
	cout << "--------------------" << endl;
	cout << tostring(e) << ": " << endl;
	cout << draw(e) << endl;
	cout << "with respect to";
	for(auto &v : vs) cout << ' ' << tostring(v);
	cout << endl;
	cout << " to " << endl;
	auto ee = rewrite(e,scalarsimpwrt(vs));
	cout << tostring(ee) << ": " << endl;
	cout << draw(ee) << endl;
}
	

int main(int argc, char **argv) {
	E x(newvar<double>("x"));
	E y(newvar<double>("y"));
	E z(newvar<double>("z"));
	E e2(plusop,x,x);

/*
	showsimp((x+x)*(x+x)-y);
	showsimp(expr(minusop,newconst(3.0),newconst(1.2))+(y+y)+y+(x+x)+(x+x));
	showsimp(x+(x+x)+(2*x)+(x*pow(x,2.0))+(x*pow(3.0,2.0)));
	showsimp(x*pow(x,2.0));	
	showsimp(x*1*(x+0) + pow((x+x),0) + (x*y*(4+x)*0*y) + pow(1,x+y));
	showsimp(5*pow(x,y) + 6*pow(x,y));
	showsimp(pow(x,y)*pow(x,4));
	showsimp(pow(x,1) * pow(x,y));
	showsimp(pow(x,pow(y,2)));
	showsimp(pow(pow(2,x),3));

	showsimp(pow(pow(x,y),2));
	showsimp(pow(pow(x,y),2),vector<expr>{x});
	showsimp(pow(pow(x,y),2),vector<expr>{y});

	showsimp(x*y + 3*y);
	showsimp(x*y + 3*y,vector<expr>{x});
	showsimp(x*y + 3*y,vector<expr>{y});
	showsimp(x*y + 3*y,vector<expr>{x,y});
	showsimp(x*y + 3*y,vector<expr>{z});

	showsimp(log(2*x));
	showsimp(log(2*x*y*x), vector<expr>{x});
	showsimp(log(2*x*y*x), vector<expr>{y});

	showsimp(log(exp(x)));
	showsimp(exp(log(x)));

	showsimp(pow(exp(3),log(x) + log(4.0) + log(y)));

	showsimp(3*(x+y));
	showsimp((y+x)*(x+3));
	showsimp((y+x)*(x+3),vector<expr>{y});

	showsimp(x/y);
	showsimp(pow(x,2)/pow(y,4));

	showsimp((2+pow(x,2))+(pow(x,2)+2));

	showsimp((x*y)+(y*x));
*/

	showsimp(abs(x+y));
	showsimp(abs(x+y)+abs(x+y));
	showsimp(abs(2*x+2*y)+abs(x+y));
	showsimp(cond(x,cond(x+5,1,y),cond(x+6,3,cond(x-2,y,y*y))));

	showsimp(log(cond(x,x,x*x)));
	showsimp(cond(x,log(x),log(x*x)));
}
