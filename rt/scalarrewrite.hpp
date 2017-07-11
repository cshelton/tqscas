#ifndef SCALARREWRITE_HPP
#define SCALARREWRITE_HPP

#include "scalar.hpp"
#include <algorithm>

struct optochain : public rewriterule {
	std::vector<std::shared_ptr<opchain>> cop;

	optochain(std::vector<std::shared_ptr<opchain>> &&chains)
			: cop(std::move(chains)) {}
	optochain(const std::vector<std::shared_ptr<opchain>> &chains)
			: cop(chains) {}

	template<typename... T>
	optochain(T &&...cops) {
		insertchains(std::forward<T>(cops)...);
	}

	void insertchains() {}
	template<typename T, typename... Ts>
	void insertchains(T &&x, Ts &&...xs) {
		cop.emplace_back(std::forward<T>(x));
		insertchains(std::forward<Ts>(xs)...);
	}

	virtual bool apply(expr &e) const {
		if (!e.isleaf()) {
			auto &n = e.asnode();
			auto &ch = e.children();
			for(auto &op : cop) {
				bool ret = false;
				if (n==op->baseop) {
					e = expr(op,ch);
					ret=true;
				}
				if (n==op) {
					bool tomod=false;
					for(auto &c : ch)
						tomod |= (!c.isleaf() && c.asnode()==op);
					if (tomod) {
						std::vector<expr> newch;
						for(auto &c : ch)
							if (c.isleaf() || c.asnode()!=op)
								newch.emplace_back(c);
							else
								for(auto &cc : c.children())
									newch.emplace_back(cc);
						e = expr(op,newch);
						ret = true;
					}
				}
				if (ret) return true;
			}
		}
		return false;
	}
};

struct rewritechain : public rewriterule {
	std::shared_ptr<binrewrite> base;
	rewritechain(std::shared_ptr<binrewrite> baserule) : base(baserule) {}

	virtual bool apply(expr &e) const {
		if (e.isleaf()) return false;
		auto aschain = std::dynamic_pointer_cast<opchain>(e.asnode());
		if (!aschain || aschain->baseop != base->matchop) return false;
		auto &ch = e.children();
		if (ch.size()==2) {
			if (base->valid(ch[0],ch[1])) {
				e = base->exec(ch[0],ch[1]);
				return true;
			} else return false;
		}
		for(int i=0;i<ch.size();i++)
			for(int j=i+1;j<ch.size();j++)
				if (base->valid(ch[i],ch[j])) {
					std::vector<expr> newch;
					newch.reserve(ch.size()-1);
					for(int k=0;k<i;k++) newch.emplace_back(ch[k]);
					newch.emplace_back(base->exec(ch[i],ch[j]));
					for(int k=i+1;k<j;k++) newch.emplace_back(ch[k]);
					for(int k=j+1;k<ch.size();k++) newch.emplace_back(ch[k]);
					e = expr(e.asnode(),newch);
					return true;
				}
		return false;
	}
};

struct minustoplus : public binrewrite {
	minustoplus() : binrewrite(minusop) {}
	virtual bool valid(const expr &e1, const expr &e2) const { return true; }
	virtual expr exec(const expr &e1, const expr &e2) const {
		return {plusop,e1,expr(negateop,e2)};
	}
};

struct makehyper: public binrewrite {
	op hop;
	makehyper(op baseop, op hyperop) : binrewrite(baseop), hop(hyperop) {}
	virtual bool valid(const expr &e1, const expr &e2) const { return e1==e2; }
	virtual expr exec(const expr &e1, const expr &e2) const {
		return {hop,newconst(2.0),e1};
	}
};

/*
expr omitchild(const expr &e, int chi) {
	auto &ch = e.children();
	std::vector<expr> newch;
	newch.reserve(ch.size()-1);
	for(int i=0;i<ch.size();i++)
		if (i!=chi) newch.emplace_back(ch[i]);
	return {e.asnode(),newch};
}

int findsingleconst(const expr &e) {
	auto &ch = e.children();
	int ki = -1;
	for(int i=0;i<ch.size();i++)
		if (isconst(ch[i])) {
			if (ki==-1) ki=i;
			else { ki=-2; break; }
		}
	return ki;
}

template<typename... V>
int findsingleconstwrt(const expr &e, V &&...vs) {
	auto &ch = e.children();
	int ki = -1;
	for(int i=0;i<ch.size();i++)
		if (isconstwrt(ch[i],std::forward<V>(vs)...)) {
			if (ki==-1) ki=i;
			else { ki=-2; break; }
		}
	return ki;
}

int findsingleconstwrt(const expr &e, std::vector<expr> &vs) {
	auto &ch = e.children();
	int ki = -1;
	for(int i=0;i<ch.size();i++)
		if (isconstwrt(ch[i],vs)) {
			if (ki==-1) ki=i;
			else { ki=-2; break; }
		}
	return ki;
}
*/

bool hasconstwrt(const expr &e, const std::vector<expr> &vs) {
	for(auto &c : e.children())
		if (isconstwrt(c,vs)) return true;
	return false;
}

bool hasnonconstwrt(const expr &e, const std::vector<expr> &vs) {
	for(auto &c : e.children())
		if (!isconstwrt(c,vs)) return true;
	return false;
}

// returns whether all elements are constants
bool factorconstwrt(const expr &e, const std::vector<expr> &vs, 
		expr &k, expr &nonk, double defconst=1.0) {
	std::vector<expr> kch,nonkch;

	for(auto &c : e.children())
		if (isconstwrt(c,vs)) kch.emplace_back(c);
		else nonkch.emplace_back(c);

	if (kch.size()==0) k = newconst(defconst);
	else if (kch.size()==1) k = kch[0];
	else k = expr{e.asnode(),kch};

	if (nonkch.size()==1) nonk = nonkch[0];
	else nonk = expr{e.asnode(),nonkch};

	return nonkch.empty();
}

struct mergehyper : public binrewrite {
	std::vector<expr> vars;

	op hop;
	bool kfirst;
	mergehyper(op baseop, op hyperop, bool constfirst=true) : binrewrite(baseop), hop(hyperop), kfirst(constfirst) { vars.emplace_back(allvars); }

	mergehyper(op baseop, op hyperop, std::vector<expr> v, bool constfirst=true) : binrewrite(baseop), hop(hyperop), kfirst(constfirst), vars(std::move(v)) {}

	bool factorconst(expr &e, expr &k) const {
		if (e.isleaf()) {
			k = newconst(1.0);
			return true;
		}
		if (e.asnode()!=hop) {
			auto aschain = std::dynamic_pointer_cast<opchain>(e.asnode());
			if (!aschain || aschain->baseop != hop) {
				k = newconst(1.0);
				return true;
			}
		}
		auto &ch = e.children();
		if (ch.size()<2) return false;
		expr nonk;
		if (factorconstwrt(e,vars,k,nonk)) return false;
		e = nonk;
		return true;
	}

	virtual bool valid(const expr &e1, const expr &e2) const {
		expr oe1(e1),oe2(e2),k1,k2;
		return factorconst(oe1,k1) && factorconst(oe2,k2) && oe1==oe2;
	}

	virtual expr exec(const expr &e1, const expr &e2) const {
		expr oe1(e1),oe2(e2),k1,k2;
		factorconst(oe1,k1);
	     factorconst(oe2,k2);
		if (kfirst) return {hop,k1+k2,oe1};
		else return {hop,oe1,k1+k2};
	}
};


struct multtopow : public binrewrite {
	multtopow() : binrewrite(multipliesop) {}
	virtual bool valid(const expr &e1, const expr &e2) const { return e1==e2; }
	virtual expr exec(const expr &e1, const expr &e2) const {
		return {powerop,newconst(2.0),e1};
	}
};

struct consteval : public rewriterule {
	virtual bool apply(expr &e) const {
		if (!e.isleaf()) {
			bool allconst = true;
			for(auto &c : e.children())
				if (!isconst(c)) allconst = false;
			if (allconst) {
				std::vector<any> subexpr;
				subexpr.reserve(e.children().size());
				for(auto &c : e.children()) 
					subexpr.emplace_back(c.asleaf());
				e = expr(e.asnode()->eval(subexpr));
				return true;
			}
		}
		return false;
	}
};

struct chainconstcompress : public rewriterule {
	virtual bool apply(expr &e) const {
		if (e.isleaf()) return false;
		auto aschain = std::dynamic_pointer_cast<opchain>(e.asnode());
		if (!aschain) return false;
		int numconst = 0;
		auto &ch = e.children();
		for(auto &c : ch)
			if (isconst(c)) numconst++;
		if (numconst<=1) return false;
		if (numconst == ch.size()) return false; // to be done by consteval
		std::vector<expr> newch;
		newch.reserve(ch.size()-numconst+1);
		std::vector<any> ks;
		ks.reserve(numconst);
		for(auto &c : ch)
			if (isconst(c)) ks.emplace_back(c.asleaf());
		newch.emplace_back(aschain->eval(ks));
		for(auto &c : ch) if (!isconst(c)) newch.emplace_back(c);
		e = expr(e.asnode(),newch);
		return true;
	}
};

struct negatetomult : public rewriterule {
	virtual bool apply(expr &e) const {
		if (!e.isleaf() && e.asnode()==negateop) {
			e = expr(multipliesop,newconst(-1.0),e.children()[0]);
			return true;
		}
		return false;
	}
};

struct removeidentity : public binrewrite {
	double k;
	bool on1,on2;
	removeidentity(op oo, double kk, bool onfirst=true, bool onsecond=true)
			: binrewrite(oo), k(kk), on1(onfirst), on2(onsecond) {}

	bool kmatch(const expr &e) const {
		if (!isconst(e)) return false;
		auto ek = getconst<double>(e);
		return ek == k;
	}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return (on1 && kmatch(e1)) || (on2 && kmatch(e2));
	}
	virtual expr exec(const expr &e1, const expr &e2) const {
		if (on1 && kmatch(e1)) return e2;
		return e1; // we assume "valid" returned true
	}
};

struct divtomult : public binrewrite {
	divtomult() : binrewrite(dividesop) {}
	virtual bool valid(const expr &e1, const expr &e2) const {
		return true;
	}
	virtual expr exec(const expr &e1, const expr &e2) const {
		return e1*pow(e2,-1.0);
	}
};

struct collapseidentity : public binrewrite {
	double oldk,newk;
	bool on1,on2;
	collapseidentity(op oo, double oldkk, double newkk, bool onfirst=true, bool onsecond=true)
			: binrewrite(oo), oldk(oldkk), newk(newkk), on1(onfirst), on2(onsecond) {}

	bool kmatch(const expr &e) const {
		if (!isconst(e)) return false;
		auto ek = getconst<double>(e);
		return ek == oldk;
	}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return (on1 && kmatch(e1)) || (on2 && kmatch(e2));
	}
	virtual expr exec(const expr &e1, const expr &e2) const {
		return newconst(newk);
	}
};

struct multpow : public binrewrite {
	std::vector<expr> vars;

	multpow() :binrewrite(multipliesop) { vars.emplace_back(allvars); }
	multpow(std::vector<expr> v) :binrewrite(multipliesop),
			vars(std::move(v)) { }
	static bool breakexp(const expr &e, expr &b, expr &exp) {
		if (e.isleaf() || e.asnode()!=powerop) {
			b = e;
			exp = newconst(1.0);
		} else {
			b = e.children()[0];
			exp = e.children()[1];
		}
		return true;
	}
	virtual bool valid(const expr &e1, const expr &e2) const {
		if (isconstwrt(e1,vars) && !isconstwrt(e2,vars)) return false;
		expr b1,exp1,b2,exp2;
		breakexp(e1,b1,exp1);
		breakexp(e2,b2,exp2);
		return b1==b2;
	}

	virtual expr exec(const expr &e1, const expr &e2) const {
		expr b1,exp1,b2,exp2;
		breakexp(e1,b1,exp1);
	    	breakexp(e2,b2,exp2);
		return {powerop,b1,exp1+exp2};
	}
};

struct powpow : public binrewrite {
	std::vector<expr> vars;
	powpow() : binrewrite(powerop) { vars.emplace_back(allvars); }
	powpow(std::vector<expr> vs) : binrewrite(powerop), vars(std::move(vs)) {}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return !e1.isleaf()
			&& e1.asnode()==powerop
			&& (!isconstwrt(e1,vars) || isconstwrt(e2,vars));
	}

	virtual expr exec(const expr &e1, const expr &e2) const {
		const expr &exp1(e1.children()[0]);
		const expr &exp2(e1.children()[1]);
		return {powerop,exp1,exp2*e2};
	}
};

struct powmult : public binrewrite {
	std::vector<expr> vars;

	powmult() : binrewrite(powerop) { vars.emplace_back(allvars); }
	powmult(std::vector<expr> v) : binrewrite(powerop), vars(std::move(v)) {}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return !e2.isleaf() && e2.asnode()==multiplieschain
			&& isconstwrt(e1,vars) && hasconstwrt(e2,vars)
			&& hasnonconstwrt(e2,vars);
	}

	virtual expr exec(const expr &e1, const expr &e2) const {
		expr newe,k;
		factorconstwrt(e2,vars,k,newe);
		return {powerop,expr(powerop,e1,k),newe};
	}
};

struct powmult2 : public binrewrite {
	std::vector<expr> vars;

	powmult2() : binrewrite(powerop) { vars.emplace_back(allvars); }
	powmult2(std::vector<expr> v) : binrewrite(powerop), vars(std::move(v)) {}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return !e1.isleaf() && e1.asnode()==multiplieschain
			&& !isconstwrt(e2,vars);
	}

	virtual expr exec(const expr &e1, const expr &e2) const {
		std::vector<expr> newc;
		for(auto &c : e1.children())
			newc.emplace_back(powerop,c,e2);
		return {e1.asnode(),newc};
	}
};

struct multpow2 : public binrewrite {
	std::vector<expr> vars;

	multpow2() : binrewrite(multipliesop) { vars.emplace_back(allvars); }
	multpow2(std::vector<expr> v) : binrewrite(multipliesop),
				vars(std::move(v)) {}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return !e1.isleaf() && !e2.isleaf()
			&& e1.asnode()==powerop && e2.asnode()==powerop
			&& e1.children()[1] == e2.children()[1]
			&& isconstwrt(e1.children()[0],vars)
			&& isconstwrt(e2.children()[0],vars);
	}

	virtual expr exec(const expr &e1, const expr &e2) const {
		return {powerop,
			e1.children()[0]*e2.children()[0],
			e1.children()[1]};
	}
};

struct logmult : public rewriterule {
	std::vector<expr> vars;

	logmult() { vars.emplace_back(allvars); }
	logmult(std::vector<expr> v) : vars(std::move(v)) {}

	/*
	virtual bool apply(expr &e) const {
		if (e.isleaf() || e.asnode()!=logop) return false;
		auto &c = e.children()[0];
		if (c.isleaf() || (c.asnode()!=multipliesop
					&& c.asnode()!=multiplieschain)) return false;
		if (!hasconstwrt(c,vars) || !hasnonconstwrt(c,vars)) return false;
		expr k,nonk;
		factorconstwrt(c,vars,k,nonk);
		e = log(k)+log(nonk);
		return true;
	}
	*/
	virtual bool apply(expr &e) const {
		if (e.isleaf() || e.asnode()!=logop) return false;
		auto &c = e.children()[0];
		if (c.isleaf() || (c.asnode()!=multipliesop
					&& c.asnode()!=multiplieschain)) return false;
		std::vector<expr> newch;
		newch.reserve(c.children().size());
		for(auto &ch : c.children())
			newch.emplace_back(log(ch));
		e = expr{pluschain,newch};
		return true;
	}

};

struct logpow : public rewriterule {
	
	virtual bool apply(expr &e) const {
		if (e.isleaf() || e.asnode()!=logop) return false;
		auto &c = e.children()[0];
		if (c.isleaf() || c.asnode()!=powerop) return false;
		e = c.children()[1] * log(c.children()[0]);
		return true;
	}
};

struct powlog : public rewriterule {
	std::vector<expr> vars;

	powlog() { vars.emplace_back(allvars); }
	powlog(std::vector<expr> v) : vars(std::move(v)) {}

	virtual bool apply(expr &e) const {
		if (e.isleaf() || e.asnode()!=powerop) return false;
		auto &ch = e.children();
		auto &b = ch[0];
		auto &exp = ch[1];
		if (exp.isleaf() || exp.asnode()!=logop) return false;
		auto &logexp = ch[1].children()[0];
		if (!isconstwrt(b,vars) || isconstwrt(logexp,vars))
			return false;
		e = pow(logexp,log(b));
		return true;
	}
};

struct powlog2 : public rewriterule {
	std::vector<expr> vars;

	powlog2() { vars.emplace_back(allvars); }
	powlog2(std::vector<expr> v) : vars(std::move(v)) {}

	virtual bool apply(expr &e) const {
		if (e.isleaf() || e.asnode()!=powerop) return false;
		auto &ch = e.children();
		auto &b = ch[0];
		if (!isconstwrt(b,vars)) return false;
		auto &exp = ch[1];
		if (exp.isleaf() || (exp.asnode()!=plusop
					&& exp.asnode()!=pluschain)) return false;
		auto &expch = exp.children();
		std::vector<expr> logch,nonlogch;
		for(auto &c : expch)
			if (c.isleaf() || c.asnode()!=logop) nonlogch.emplace_back(c);
			else logch.emplace_back(c);
		if (logch.empty()) return false;
		for(auto &c : logch) c = pow(b,c);
		if (nonlogch.empty())
			e = expr{multiplieschain,logch};
		else e = expr{multiplieschain,logch}*pow(b,expr{pluschain,nonlogch});
		return true;
	}
};


struct powplus: public rewriterule {
	std::vector<expr> vars;

	powplus() { vars.emplace_back(allvars); }
	powplus(std::vector<expr> v) : vars(std::move(v)) {}

	virtual bool apply(expr &e) const {
		if (e.isleaf() || e.asnode()!=powerop) return false;
		auto &ch = e.children();
		auto &b = ch[0];
		auto &exp = ch[1];
		if (!isconstwrt(b,vars)) return false;
		if (exp.isleaf() || (exp.asnode() != plusop 
						&& exp.asnode() != pluschain)) return false;
		if (!hasconstwrt(exp,vars) || !hasnonconstwrt(exp,vars)) return false;
		expr k,nonk;
		factorconstwrt(exp,vars,k,nonk);
		e = pow(b,k)*pow(b,nonk);
		return true;
	}
};

struct distribute : public binrewrite {
	std::vector<expr> vars;
	distribute() : binrewrite(multipliesop) { vars.emplace_back(allvars); }
	distribute(std::vector<expr> v) : binrewrite(multipliesop),
			vars(std::move(v)) {}

	bool check(const expr &e1, const expr &e2) const {
		return isconstwrt(e1,vars) 
			&& !e2.isleaf()
			&& (e2.asnode()==plusop || e2.asnode()==pluschain)
			&& !isconstwrt(e2,vars);
	}

	virtual bool valid(const expr &e1, const expr &e2) const {
		return check(e1,e2) || check(e2,e1);
	}
	virtual expr exec(const expr &e1, const expr &e2) const {
		if (check(e1,e2)) {
			auto ch = e2.children();
			for(auto &c : ch) c = e1*c;
			return {e2.asnode(),ch};
		} else {
			auto ch = e1.children();
			for(auto &c : ch) c = c*e2;
			return {e1.asnode(),ch};
		}
	}
};



struct sortchildren : public rewriterule {
	std::vector<expr> vars;
	std::vector<op> cops;

	sortchildren(std::vector<op> comops)
		: cops(comops) { vars.emplace_back(allvars); }
	sortchildren(std::vector<op> comops, std::vector<expr> v)
		: cops(comops), vars(std::move(v)) {}

	int typeorder(const expr &e) const {
		if (e.isleaf()) {
			if (isconst(e)) return 0;
			if (isconstwrt(e,vars)) return 1;
			return 7;
		}
		int add = (isconstwrt(e,vars) ? 2 : 8);
		auto &op = e.asnode();
		if (op==plusop || op==pluschain) return 0+add;
		if (op==multipliesop || op==multiplieschain) return 1+add;
		if (op==powerop) return 2+add;
		if (op==logop) return 3+add;
		return 4+add;
	}

	int secondordering(const expr &e1, const expr &e2) const {
		if (e1.isleaf()) {
			if (isconst(e1)) {
				double v1 = getconst<double>(e1);
				double v2 = getconst<double>(e2);
				if (v1<v2) return -1;
				if (v2>v1) return +1;
				return 0;
			}
			return MYany_cast<var>(e1.asleaf())->name
					.compare(MYany_cast<var>(e2.asleaf())->name);
		}
		auto &ch1 = e1.children();
		auto &ch2 = e2.children();
		for(int i=std::min(ch1.size(),ch2.size())-1;i>=0;i--) {
			int res = exprcmp(ch1[i],ch2[i]);
			if (res!=0) return res;
		}
		return ch1.size()-ch2.size();
	}

	int exprcmp(const expr &e1, const expr &e2) const {
		int o1 = typeorder(e1), o2 = typeorder(e2);
		if (o1!=o2) return o1-o2;
		return secondordering(e1,e2);
	}

	virtual bool apply(expr &e) const {
		if (e.isleaf()) return false;
		if (std::find(cops.begin(),cops.end(),e.asnode())==cops.end()) return false;
		auto &ch = e.children();
		if (ch.size()<2) return false;
		for(int i=1;i<ch.size();i++)
			if (exprcmp(ch[i-1],ch[i])>0) {
				std::vector<expr> che = ch;
				std::sort(che.begin(),che.end(),
						[this](const expr &e1, const expr &e2) {
							return exprcmp(e1,e2)<0;
						}
				);
				auto node = e.asnode();
				e = expr{node,che};
				return true;
			}
		return false;
	}
};


struct removesinglechain : public rewriterule {
	op matchop;
	removesinglechain(op mop) : matchop(mop) {}

	virtual bool apply(expr &e) const {
		if (!e.isleaf() && e.asnode()==matchop && e.children().size()==1) {
			e = e.children()[0];
			return true;
		}
		return false;
	}
};

const std::vector<ruleptr> scalarsimprules {{
		toptr<minustoplus>(),
		toptr<consteval>(),
		toptr<chainconstcompress>(),
		toptr<negatetomult>(),
		toptr<divtomult>(),
		toptr<optochain>(pluschain,multiplieschain),
		toptr<removeidentity>(powerop,1.0,false,true),
		toptr<rewritechain>(toptr<removeidentity>(plusop,0.0)),
		toptr<rewritechain>(toptr<removeidentity>(multipliesop,1.0)),
		toptr<collapseidentity>(powerop,0.0,1.0,false,true),
		toptr<collapseidentity>(powerop,1.0,1.0,true,false),
		toptr<rewritechain>(toptr<collapseidentity>(multipliesop,0.0,0.0)),
		//toptr<makehyper>(plusop,multipliesop),
		//toptr<makehyper>(multipliesop,powerop),
		//toptr<rewritechain>(toptr<makehyper>(plusop,multipliesop)),
		//toptr<rewritechain>(toptr<makehyper>(multipliesop,powerop)),
		toptr<rewritechain>(toptr<mergehyper>(plusop,multipliesop)),
		//toptr<rewritechain>(toptr<mergehyper>(multipliesop,powerop,false)),
		toptr<rewritechain>(toptr<multpow>()),
		toptr<removesinglechain>(pluschain),
		toptr<removesinglechain>(multiplieschain),
		toptr<powpow>(),
		toptr<powmult>(),
		toptr<powmult2>(),
		toptr<rewritechain>(toptr<multpow2>()),
		toptr<logmult>(),
		toptr<logpow>(),
		toptr<powlog>(),
		toptr<powplus>(),
		toptr<powlog2>(),
		toptr<rewritechain>(toptr<distribute>()),
		toptr<sortchildren>(std::vector<op>{pluschain,multiplieschain,plusop,multipliesop}),
		}};

struct rewriteconst : public rewriterule {
	std::vector<expr> vars;

	rewriteconst(std::vector<expr> v) : vars(std::move(v)) {}

	virtual bool apply(expr &e) const {
		if (!isconstwrt(e,vars)) return false;
		bool ch = false;
		e = rewrite(e,scalarsimprules,ch);
		return ch;
	}
};


std::vector<ruleptr> scalarsimpwrt(const std::vector<expr> &vs) {
	return {{
		toptr<minustoplus>(),
		toptr<consteval>(),
		toptr<chainconstcompress>(),
		toptr<negatetomult>(),
		toptr<divtomult>(),
		toptr<optochain>(pluschain,multiplieschain),
		toptr<removeidentity>(powerop,1.0,false,true),
		toptr<rewritechain>(toptr<removeidentity>(plusop,0.0)),
		toptr<rewritechain>(toptr<removeidentity>(multipliesop,1.0)),
		toptr<collapseidentity>(powerop,0.0,1.0,false,true),
		toptr<collapseidentity>(powerop,1.0,1.0,true,false),
		toptr<rewritechain>(toptr<collapseidentity>(multipliesop,0.0,0.0)),
		//toptr<makehyper>(plusop,multipliesop),
		//toptr<makehyper>(multipliesop,powerop),
		//toptr<rewritechain>(toptr<makehyper>(plusop,multipliesop)),
		//toptr<rewritechain>(toptr<makehyper>(multipliesop,powerop)),
		toptr<rewritechain>(toptr<mergehyper>(plusop,multipliesop,vs)),
		//toptr<rewritechain>(toptr<mergehyper>(multipliesop,powerop,vs,false)),
		toptr<rewritechain>(toptr<multpow>(vs)),
		toptr<removesinglechain>(pluschain),
		toptr<removesinglechain>(multiplieschain),
		toptr<powpow>(vs),
		toptr<powmult>(vs),
		toptr<powmult2>(vs),
		toptr<rewritechain>(toptr<multpow2>(vs)),
		toptr<logmult>(vs),
		toptr<logpow>(),
		toptr<powlog>(vs),
		toptr<powplus>(vs),
		toptr<powlog2>(vs),
		toptr<rewritechain>(toptr<distribute>(vs)),
		toptr<sortchildren>(std::vector<op>{pluschain,multiplieschain,plusop,multipliesop},vs),
		toptr<rewriteconst>(vs),
		}};
}

#endif
