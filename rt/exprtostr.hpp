#ifndef EXPRTOSTR_HPP
#define EXPRTOSTR_HPP

#include <string>
#include "exprbase.hpp"
#include <unordered_map>
#include <typeinfo>
#include <typeindex>
#include <utility>
#include <numeric>

std::string tostring(const any &x) {
	std::unordered_map<std::type_index,std::function<std::string(const any &)>>
		tostringlookup =
		 {
		   {typeid(double),
		    [](const any &x) { return std::to_string(MYany_cast<double>(x)); }},
		   {typeid(long double),
		    [](const any &x) { return std::to_string(MYany_cast<long double>(x)); }},
		   {typeid(float),
		    [](const any &x) { return std::to_string(MYany_cast<float>(x)); }},
		   {typeid(int),
		    [](const any &x) { return std::to_string(MYany_cast<int>(x)); }},
		   {typeid(unsigned int),
		    [](const any &x) { return std::to_string(MYany_cast<unsigned int>(x)); }},
		   {typeid(long),
		    [](const any &x) { return std::to_string(MYany_cast<long>(x)); }},
		   {typeid(unsigned long),
		    [](const any &x) { return std::to_string(MYany_cast<unsigned long>(x)); }},
		   {typeid(long long),
		    [](const any &x) { return std::to_string(MYany_cast<long long>(x)); }},
		   {typeid(unsigned long long),
		    [](const any &x) { return std::to_string(MYany_cast<unsigned long long>(x)); }}
		  };

	return tostringlookup[x.type()](x);
}

std::string tostring(const expr &e) {
	return e.fold([](const leaf &l) {
			if (l.type() == typeid(var)) return
				std::make_pair(MYany_cast<var>(l)->name,-1);
			else return std::make_pair(tostring(l),-1);
			},
			[](const op &o, const std::vector<std::pair<std::string,int>> &ch) {
				std::string ret;
				if (!o->infix || ch.size()<=1) {
					ret += o->name;
					if (ch.size()>1) {
						ret += '(';
						ret += ch[0].first;
						for(int i=1;i<ch.size();i++) {
							ret += ',';
							ret += ch[i].first;
						}
						ret += ')';
					} else if (!ch.empty()) {
						if (o->prec<ch[0].second) {
							ret += '(';
							ret += ch[0].first;
							ret += ')';
						} else ret += ' ' + ch[0].first;
					} else ret += "()";
				} else {
					if (o->prec<ch[0].second || o->prec==ch[0].second && !o->leftassoc) {
						ret += '('; ret += ch[0].first; ret += ')';
					} else ret += ch[0].first;
					for(int i=1;i<ch.size();i++) {
						ret += o->name;
						if (o->prec<ch[i].second
							//	|| i!=ch.size()-1
								|| (o->prec==ch[i].second && o->leftassoc)) {
							ret += '('; ret += ch[i].first; ret += ')';
						} else ret += ch[i].first;
					}
				}
				return std::make_pair(ret,o->prec);
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
			if (l.type() == typeid(var))
				return retT(MYany_cast<var>(l)->name+"\n");
			else return retT(tostring(l)+"\n");
			},
			[](const op &o, const std::vector<retT> &ch) {
				if (ch.empty()) return retT(o->treename);
				retT ret("");
				for(int i=0;i<ch.size();i++) {
					std::string pre =
						std::string(o->treename.size()+4,' ')
						+ (i==0 ? "   " : "|  ");
					std::string post =
						std::string(o->treename.size()+4,' ')
						+ (i==ch.size()-1 ? "   " : "|  ");
					std::string mid;
					if (i*2== ch.size()-1) {
						if (i==0) mid = std::string("[") + o->treename + "] --- ";
						else mid = std::string("[") + o->treename + "] -+- ";
					} else if (i==0) {
						mid = std::string(o->treename.size()+4,' ')+"/- ";
					} else if (i==ch.size()-1) {
						mid = std::string(o->treename.size()+4,' ')+"\\- ";
					} else {
						mid = std::string(o->treename.size()+4,' ')+"+- ";
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
				if (ch.size()%2 == 0) ret.at = std::string("[") + o->treename + "] < \n";
				return ret;
			}
	);
	auto ls = lines.single("","","");
	return std::accumulate(ls.begin(),ls.end(),std::string(""));
}

#endif
