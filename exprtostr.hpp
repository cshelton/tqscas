#ifndef EXPRTOSTR_HPP
#define EXPRTOSTR_HPP

#include <string>
#include <numeric>
#include "base.hpp"
#include "typestuff.hpp"

std::string to_string(const std::monostate &) { return std::string(""); }

template<typename T, typename = void>
struct hasmytostring : std::false_type {};

template<typename T>
struct hasmytostring<T,std::void_t<
		decltype(to_string(std::declval<T>()))>> : std::true_type {};

template<typename T>
inline constexpr bool hasmytostring_v = hasmytostring<T>::value;

template<typename T>
std::string tostring(const T &x) {
	if constexpr (hasmytostring_v<T>) return to_string(x);
	else return std::to_string(x);
}

// this should be overloaded!
template<typename O>
constexpr int precedence(const O &) {
	return 100;
}

// this should be overloaded!
template<typename O>
constexpr std::string symbol(const O &) {
	return "?";
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
	int myprec = precedence(op);
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
std::string writeprefixunary(O op,
		const std::vector<std::pair<std::string,int>> &subst) {
	return symbol(op) + putinparen(subst[0].first,subst[0].second>precedence(op));
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

// this can be overloaded as needed
template<typename O>
std::string write(const O &op,
		const std::vector<std::pair<std::string,int>> &subst) {
	switch(subst.size()) {
		case 1: return writeprefixunary(op,subst);
		case 2: return writeinplace(op,subst); // assume left associativity
		default: return writeasfunc(op,subst);
	}
}



template<typename E,
	std::enable_if_t<isexpr_v<E>,int> = 0>
std::string to_string(const E &e) {
	return exprfold(e,
			[](const auto &l) {
				using L = std::decay_t<decltype(l)>;
				if constexpr (isvartype_v<L>)
					return std::make_pair(l.name(),0);
				else if constexpr (isconsttype_v<L>)
					return std::make_pair(tostring(l.v),0);
				else return std::make_pair(std::string(""),0);
			},
			[](const auto &o, auto &&chval) {
				return std::make_pair(write(o,
					std::forward<decltype(chval)>(chval)),
					precedence(o));
			}
		).first;
}

template<typename E>
std::string draw(const E &e) {
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

	auto lines = exprfold(e,
			[](const auto &l) -> retT {
				using L = std::decay_t<decltype(l)>;
				if constexpr (isvartype_v<L>)
					return l.name()+"\n";
				else if constexpr (isconsttype_v<L>)
					return tostring(l.v) +"\n";
				else return std::string("\n");
			},
			[](const auto &o, auto &&ch) {
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




#endif
