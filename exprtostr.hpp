#ifndef EXPRTOSTR_HPP
#define EXPRTOSTR_HPP

#include <string>
#include "exprbase.hpp"
#include "typestuff.hpp"

template<typename E>
std::string tostring(const E &e) {
	return e.fold([](const leaf &l) {
			if (istype<var>(l))
				return std::make_pair(std::get<var>(l)->name,0);
			if (istype<noexprT>(l))
				return std::make_pair(std::string(""),0);
			return std::visitor([](auto &&a) -> std::string {
					return std::to_string(std::forward<decltype(a)>(a));
					}, l);
			},
			[](const op &o, const std::vector<std::pair<std::string,int>> &ch) {
				return std::make_pair(write(o,ch),precidence(o));
			}
	).first;
}

std::string draw(const expr &e) {
	struct retT {
		std::vector<std::string> before,after;
		std::string at;

		retT(const std::string &s) : at(s) {}

		std::vector<std::string> single(const std::string &pre1, const std::string &pre2, const std::string &pre3) const {
			std::vector<std::string> ret;
			ret.reserve(pre1.size()+pre2.size()+pre3.size());
			for(auto &l : before) ret.emplace_back(pre1+l);
			ret.emplace_back(pre2+at);
			for(auto &l : after) ret.emplace_back(pre3+l);
			return ret;
		}
	};

	auto lines = e.fold([](const leaf &l) {
			if (istype<var>(l))
				return std::get<var>(l)->name+"\n";
			if (istype<noexprT>(l))
				return std::make_pair(std::string("\n"),0);
			return std::visitor([](auto &&a) -> std::string {
				return std::to_string(std::forward<decltype(a)>(a))+"\n";
				}, l);
			},
			[](const op &o, const std::vector<retT> &ch) {
				auto sym = symbol(o);
				if (ch.empty()) return retT(sym);
				retT ret("");
				for(int i=0;i<ch.size();i++) {
					std::string pre =
						std::string(sym.size()+4,' ')
						+ (i==0 ? "   " : "|  ");
					std::string post =
						std::string(sym.size()+4,' ')
						+ (i==ch.size()-1 ? "   " : "|  ");
					std::string mid;
					if (i*2== ch.size()-1) {
						if (i==0) mid = std::string("[") + sym + "] --- ";
						else mid = std::string("[") + sym + "] -+- ";
					} else if (i==0) {
						mid = std::string(sym.size()+4,' ')+"/- ";
					} else if (i==ch.size()-1) {
						mid = std::string(sym.size()+4,' ')+"\\- ";
					} else {
						mid = std::string(sym.size()+4,' ')+"+- ";
					}
					if (i*2<ch.size()-1) {
						auto ls = ch[i].single(pre,mid,post);
						for(auto &l : ls) ret.before.emplace_back(l);
					} else if (i*2>ch.size()-1) {
						auto ls = ch[i].single(pre,mid,post);
						for(auto &l : ls) ret.after.emplace_back(l);
					} else {
						for(auto &l : ch[i].before) ret.before.emplace_back(pre+l);
						ret.at = mid+ch[i].at;
						for(auto &l : ch[i].after) ret.after.emplace_back(post+l);
					}
				}
				if (ch.size()%2 == 0) ret.at = std::string("[") + sym + "] < \n";
				return ret;
			}
	);
	auto ls = lines.single("","","");
	return std::accumulate(ls.begin(),ls.end(),std::string(""));
}

// some helpful routines for constructing "write" functions for operators


std::string putinparen(std::string s, bool yes) {
	if (yes) return std::string("(") + s + std::string(")");
	else return s;
}

template<typename O>
std::string writeinplace(O op,
		const std::vector<std::pair<std::string,int>> &subst,
		bool leftassoc=true) {
	int myprec = precidence(op);
	if (subst.size()<=1) return subst[0].first;
	std::string ret;
	if (leftassoc) {
		ret = putinparen(subst[0].first,subst[0].second>myprec);
		for(int i=1;i<subst.size();i++)
			ret += symbol(op) + putinparen(subst[i].first,subst[i].second>=myprec);
	} else {
		ret = putinparen(subst.back().first,subst.back().second>myprec);
		for(int i=subst.size()-2;i>=0;i--)
			ret += putinparen(subst[i].first,subst[i].second>=myprec) +
				symbol(op) + ret;
	}
	return ret;
}

template<typename O>
std::string writeasfunc(O op,
		const std::vector<std::pair<std::string,int>> &subst) {
	std::string ret = symbol(op) + "(";
	for(int i=0;i<subst.size();i++) {
		if (i) ret += ",";
		ret += subst[i].first;
	}
	return ret+")";
}

#endif
