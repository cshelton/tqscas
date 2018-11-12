#include "scalarrewrite.hpp"
#include <iostream>
#include <iomanip>
#include <vector>
#include <tuple>
#include <type_traits>

#include <chrono>
#include <ctime>


//------
// from papagaga at https://codereview.stackexchange.com/questions/193420/apply-a-function-to-each-element-of-a-tuple-map-a-tuple

template <typename Fn, typename Argument, std::size_t... Ns>
auto tuplemapimpl(Fn&& fn, Argument&& argument, std::index_sequence<Ns...>) {
    if constexpr (sizeof...(Ns) == 0) return std::tuple<>(); // empty tuple
    else if constexpr (std::is_same_v<decltype(fn(std::get<0>(argument))), void>) {
        (fn(std::get<Ns>(argument)),...); // no return value expected
        return;
    }
    // then dispatch lvalue, rvalue ref, temporary
    else if constexpr (std::is_lvalue_reference_v<decltype(fn(std::get<0>(argument)))>) {
        return std::tie(fn(std::get<Ns>(argument))...);
    }
    else if constexpr (std::is_rvalue_reference_v<decltype(fn(std::get<0>(argument)))>) {
        return std::forward_as_tuple(fn(std::get<Ns>(argument))...);
    }
    else {
        return std::tuple(fn(std::get<Ns>(argument))...);
    }
}

template <typename Fn, typename... Ts>
auto tuplemap(Fn&& fn, const std::tuple<Ts...>& tuple) {
    return tuplemapimpl(std::forward<Fn>(fn), tuple,
                          std::make_index_sequence<sizeof...(Ts)>());
}

//------

template<std::size_t I, typename Fn, typename Arg, typename Arg2, typename X>
auto tuplefoldimpl2(Fn&& fn, Arg&& arg, Arg2&& arg2, const X &x0) {
	if constexpr (I==0) return x0;
	else return tuplefoldimpl2<I-1>(fn,arg,arg2,fn(x0,std::get<I-1>(arg),std::get<I-1>(arg2)));
}

template<typename Fn, typename... Ts, typename... Ts2, typename X> 
auto tuplefold2(Fn &&fn, const std::tuple<Ts...>& tup, const std::tuple<Ts2...> &tup2, const X &x) {
	return tuplefoldimpl2<sizeof...(Ts)>(fn,tup,tup2,x);
}

//-------


auto mytostr(const double &s) {
	return std::to_string(s);
}
auto mytostr(const scalarreal &s) {
	return tostring(s);
}
auto mytostr(const boost::any &s) {
	return tostring(s);
}

using namespace std;
using namespace std::chrono;

template<typename... Es, typename... Ts>
any evalwith(const expr &e, const tuple<Es...> &vars, const tuple<Ts...> &vals) {
	if (e.isleaf()) {
		if (isconst(e)) return getconstany(e);
		return tuplefold2([&e](auto x0, auto var, auto val) {
				if (e.sameas(var)) return scalarreal{val};
				else return x0;
			},vars,vals,scalarreal{0.0});
	} else {
		std::vector<any> vs;
		vs.reserve(e.children().size());
		for(auto &c : e.children())
			vs.emplace_back(evalwith(c,vars,vals));
		auto ret = e.asnode()->opeval(vs);
		return ret;
		//return e.asnode()->opeval(vs);
	}
}

struct traj {
	scalarreal T;
	vector<scalarreal> t;
};


template<typename L>
auto ppllh(const traj tr, L lambda) {
	expr t = newvar<scalarreal>("t");
	auto ll = lambda(tr);
	auto ret = -integrate(ll(t),t,0,tr.T);
	for(auto &tevent : tr.t) 
		ret = ret+log(ll(tevent));
	return ret;
}

template<typename K, typename T>
auto hplambda(K kernel, T mu) {
	return [mu,kernel](const traj &tr) {
		return [mu,kernel,tr](auto t) {
			auto ret = mu;
			for(auto &tevent : tr.t) 
				ret = ret+ifthenelse(tevent-t,kernel(t-tevent),scalarreal{0.0});
			return ret;
		};
	};
}

template<typename T>
auto powkernel(T alpha, T beta, T gamma) {
	return [alpha,beta,gamma](auto t) {
		return alpha*pow(t+gamma,beta);
	};
}

template<typename F,typename... Ts>
auto grad(F f, tuple<Ts...> x) {
	return tuplemap([&f](auto v) {
			return scalarruleset.rewrite(deriv(f,v));
		},x);
}

template<typename... Vs, typename... Vs2, typename T, std::size_t... Ns>
void addtupimpl(tuple<Vs...> &v, const tuple<Vs2...> &v2, T mult, std::index_sequence<Ns...>) {
	((get<Ns>(v) += get<Ns>(v2)*mult), ...);
}
template<typename... Vs, typename... Vs2, typename T>
void addtup(tuple<Vs...> &v, const tuple<Vs2...> &v2, T mult) {
	addtupimpl(v,v2,mult,std::make_index_sequence<sizeof...(Vs)>());
}

template<typename F, typename P, typename V>
auto evalat(F f, const P &params, const V &vals) {
	return MYany_cast<scalarreal>(evalwith(f,params,vals));
}

template<typename... Fs, typename P, typename V, std::size_t... Ns>
auto evalatimpl(const std::tuple<Fs...> &f, const P &params, const V &vals, std::index_sequence<Ns...>) {
	return std::forward_as_tuple(evalat(get<Ns>(f),params,vals)...);
}

template<typename... Fs, typename P, typename V>
auto evalat(const std::tuple<Fs...> &f, const P &params, const V &vals) {
	return evalatimpl(f,params,vals,
			std::make_index_sequence<sizeof...(Fs)>());
}
	

template<typename F, typename P, typename V>
int gradopt(F f, const P &params, V &vals) {
	auto fval = evalat(f,params,vals), fvalold=fval-1.0;
	auto gradf = grad(f,params);
	//cout << "grad-alpha: " << tostring(get<0>(gradf)) << endl;
	//cout << "grad-beta: " << tostring(get<1>(gradf)) << endl;
	//cout << "grad-gamma: " << tostring(get<2>(gradf)) << endl;
	//cout << "grad-mu: " << tostring(get<3>(gradf)) << endl;

	int i=0;
	while(fval-fvalold>1e-3) {
		auto gval = evalat(gradf,params,vals);
		//cout << mytostr(fval) << endl;
		//cout << mytostr(fval-fvalold) << endl;
		addtup(vals,gval,0.01);
		i++;
		if (get<0>(vals)<0.01) get<0>(vals) = 0.01;
		if (get<1>(vals)>-1.1) get<1>(vals) = -1.1;
		if (get<2>(vals)<0.1) get<2>(vals) = 0.1;
		if (get<3>(vals)<0.01) get<3>(vals) = 0.01;
		//cout << "\talpha = " << mytostr(get<0>(vals)) << ", grad = " << mytostr(get<0>(gval)) << endl;
		//cout << "\tbeta  = " << mytostr(get<1>(vals)) << ", grad = " << mytostr(get<1>(gval)) << endl;
		//cout << "\tgamma = " << mytostr(get<2>(vals)) << ", grad = " << mytostr(get<2>(gval)) << endl;
		//cout << "\tmu    = " << mytostr(get<3>(vals)) << ", grad = " << mytostr(get<3>(gval)) << endl;
		fvalold = fval;
		fval = evalat(f,params,vals);
	}
	return i;
}


template<typename F, typename... Gs, typename... Ts, typename... Vs>
int gdoptimize(F f, const tuple<Gs...> gs, const tuple<Ts...> &params, tuple<Vs...> &vals) {
	auto fval = f(vals);
	auto fvalold = fval-1.0;

	//cout << mytostr(fval) << endl;
	int i = 0;
	while(fval-fvalold>1e-3) {
		auto gval = tuplemap([&params,&vals](auto g) {
				return g(vals);
			},gs);
		//cout << "\tdelta = " << mytostr(fval-fvalold) << endl;
		//cout << mytostr(fval) << endl;
		//cout << mytostr(fval-fvalold) << endl;
		addtup(vals,gval,0.01);
		i++;
		if (get<0>(vals)<0.01) get<0>(vals) = 0.01;
		if (get<1>(vals)>-1.1) get<1>(vals) = -1.1;
		if (get<2>(vals)<0.1) get<2>(vals) = 0.1;
		if (get<3>(vals)<0.01) get<3>(vals) = 0.01;
		//cout << "\talpha = " << mytostr(get<0>(vals)) << ", grad = " << mytostr(get<0>(gval)) << endl;
		//cout << "\tbeta  = " << mytostr(get<1>(vals)) << ", grad = " << mytostr(get<1>(gval)) << endl;
		//cout << "\tgamma = " << mytostr(get<2>(vals)) << ", grad = " << mytostr(get<2>(gval)) << endl;
		//cout << "\tmu    = " << mytostr(get<3>(vals)) << ", grad = " << mytostr(get<3>(gval)) << endl;
		fvalold = fval;
		fval = f(vals);
		//cout << mytostr(fval) << endl;
	}
	return i;
}

double &operator+=(double &x, const scalarreal &s) {
	return x += (double)(s);
}
double &operator-=(double &x, const scalarreal &s) {
	return x -= (double)(s);
}



int main(int argc, char **argv) {

	int todo = argc>1 ? atoi(argv[1]) : 0;

	traj data;
	data.T = 10.0;
	data.t.push_back(0.2);
	data.t.push_back(1.3);
	data.t.push_back(1.35);
	data.t.push_back(1.5);
	data.t.push_back(1.7);
	data.t.push_back(5.9);
	data.t.push_back(6.2);
	data.t.push_back(9.25);
	data.t.push_back(9.72);
/*
	data.t.push_back(2.0);
	data.t.push_back(7.0);
*/

	expr mu = newvar<scalarreal>("mu");
	expr alpha = newvar<scalarreal>("alpha");
	expr beta = newvar<scalarreal>("beta");
	expr gamma = newvar<scalarreal>("gamma");

	tuple<expr,expr,expr,expr> params{alpha,beta,gamma,mu};

	if (todo & 1 || todo & 4) {
		auto t1 = high_resolution_clock::now();
		tuple<scalarreal,scalarreal,scalarreal,scalarreal>
					vals{1.0,-2.0,1.0,1.0};
		auto hp = hplambda(powkernel(alpha,beta,gamma),mu);
		auto llhcomplex = ppllh(data,hp);
		auto llh = scalarruleset.rewrite(llhcomplex);
		//cout << "llh: " << tostring(llh) << endl;
		auto casfn = [&llh,&params](auto v) {
			return MYany_cast<scalarreal>(evalwith(llh,params,v)); };

		if (todo & 4) {
			cout << "values: " << endl;
			double a,b,g,m;
			cin >> a >> b >> g >> m;
			get<0>(vals) = a;
			get<1>(vals) = b;
			get<2>(vals) = g;
			get<3>(vals) = m;
/*
			cin >> get<0>(vals) >>
				get<1>(vals) >>
				get<2>(vals) >>
				get<3>(vals);
*/
			cout << mytostr(casfn(vals)) << endl;
		} else {

/*
		auto gradllh = grad(llh,params);
		auto casgrad = std::make_tuple
			([&params,&gradllh](auto v) {
				return MYany_cast<scalarreal>(
					evalwith(get<0>(gradllh),params,v)); },
			 [&params,&gradllh](auto v) {
				return MYany_cast<scalarreal>(
					evalwith(get<1>(gradllh),params,v)); },
			 [&params,&gradllh](auto v) {
				return MYany_cast<scalarreal>(
					evalwith(get<2>(gradllh),params,v)); },
			 [&params,&gradllh](auto v) {
				return MYany_cast<scalarreal>(
					evalwith(get<3>(gradllh),params,v)); });

		auto t2 = high_resolution_clock::now();
		int itt = gdoptimize(casfn,casgrad,params,vals);
		auto t3 = high_resolution_clock::now();
*/
		auto t2 = high_resolution_clock::now();
		int itt = gradopt(llh,params,vals);
		auto t3 = high_resolution_clock::now();

		duration<double,std::milli> d1 = t2-t1;
		duration<double,std::milli> d2 = t3-t2;
		cout << "\tf = " << mytostr(casfn(vals)) << endl;
		cout << "\talpha = " << mytostr(get<0>(vals)) << endl;
		cout << "\tbeta  = " << mytostr(get<1>(vals)) << endl;
		cout << "\tgamma = " << mytostr(get<2>(vals)) << endl;
		cout << "\tmu    = " << mytostr(get<3>(vals)) << endl;
		cout << "CAS set-up duration: " << d1.count() << " ms" << endl;
		cout << "CAS run duration: " << d2.count() << " ms" << endl;
		cout << "CAS run iterations: " << itt << endl;
		cout << "CAS run duration/itt: " << d2.count()/itt << " ms" << endl;
		}
	}

	if (todo & 2 || todo & 4) {
		auto t1 = high_resolution_clock::now();
		tuple<double,double,double,double> vals{1.0,-2.0,1.0,1.0};
		// kernel is a*(t+g)^b
		// kernelintegrated is a/(b+1)*((t+g)^(b+1) - g^(b+1))
		auto hardcodefn = [&data](auto v) {
			scalarreal a=get<0>(v),b=get<1>(v),g=get<2>(v),m=get<3>(v);
			scalarreal ret = -m*data.T;
			ret += a/(b+1)*pow(g,b+1)*(double)(data.t.size());
			for(auto &t : data.t) {
				//ret -= a/(b+1)*(pow(data.T-t+g,b+1)-pow(g,b+1));
				ret -= a/(b+1)*pow(data.T-t+g,b+1);
				scalarreal l = m;
				for(auto &s : data.t) {
					if (s>=t) break;
					l += a*pow(t-s+g,b);
				}
				ret += log(l);
			}
			return (double)(ret);
		};
		auto hardcodegrad = std::make_tuple(
			[&data](auto v) { // grad wrt alpha
				scalarreal a=get<0>(v),b=get<1>(v),g=get<2>(v),m=get<3>(v);
				scalarreal ret = 0.0;
				for(auto &t : data.t) {
					ret -= 1/(b+1)*(pow(data.T-t+g,b+1)-pow(g,b+1));
					scalarreal l = 0.0;
					for(auto &s : data.t) {
						if (s>=t) break;
						l += pow(t-s+g,b);
					}
					ret += l/(m+a*l);
				}
				return (double)(ret);
			},
			[&data](auto v) { // grad wrt beta
				scalarreal a=get<0>(v),b=get<1>(v),g=get<2>(v),m=get<3>(v);
				scalarreal ret = 0.0;
				for(auto &t : data.t) {
					ret += a/(b+1)*
						(pow(data.T-t+g,b+1)*(1/(b+1)-log(data.T-t+g))
						 - pow(g,b+1)*(1/(b+1)-log(g)));
					scalarreal l = 0.0;
					scalarreal lm = 0.0;
					for(auto &s : data.t) {
						if (s>=t) break;
						l += pow(t-s+g,b);
						lm += pow(t-s+g,b)*log(t-s+g);
					}
					ret += a*lm/(m+a*l);
				}
				return (double)(ret);
			},
			[&data](auto v) { // grad wrt gamma
				scalarreal a=get<0>(v),b=get<1>(v),g=get<2>(v),m=get<3>(v);
				scalarreal ret = 0.0;
				for(auto &t : data.t) {
					ret -= a*(pow(data.T-t+g,b)-pow(g,b));
					scalarreal l = 0.0, lm = 0.0;
					for(auto &s : data.t) {
						if (s>=t) break;
						l += pow(t-s+g,b);
						lm += pow(t-s+g,b-1);
					}
					ret += a*b*lm/(m+a*l);
				}
				return (double)(ret);
			},
			[&data](auto v) { // grad wrt mu
				scalarreal a=get<0>(v),b=get<1>(v),g=get<2>(v),m=get<3>(v);
				scalarreal ret = -data.T;
				for(auto &t : data.t) {
					scalarreal l = 0.0;
					for(auto &s : data.t) {
						if (s>=t) break;
						l += pow(t-s+g,b);
					}
					ret += 1.0/(m+a*l);
				}
				return (double)(ret);
			}
		);

		if (todo&4) {
			cout << "values: " << endl;
			cin >> get<0>(vals) >>
				get<1>(vals) >>
				get<2>(vals) >>
				get<3>(vals);
			cout << setprecision(17) << hardcodefn(vals) << endl;
		} else {
				
		auto t2 = high_resolution_clock::now();
		int itt = gdoptimize(hardcodefn,hardcodegrad,params,vals);
		auto t3 = high_resolution_clock::now();
		duration<double,std::milli> d1 = t2-t1;
		duration<double,std::milli> d2 = t3-t2;
		cout << "\tf = " << mytostr(hardcodefn(vals)) << endl;
		cout << "\talpha = " << mytostr(get<0>(vals)) << endl;
		cout << "\tbeta  = " << mytostr(get<1>(vals)) << endl;
		cout << "\tgamma = " << mytostr(get<2>(vals)) << endl;
		cout << "\tmu    = " << mytostr(get<3>(vals)) << endl;
		cout << "hand set-up duration: " << d1.count() << " ms" << endl;
		cout << "hand run duration: " << d2.count() << " ms" << endl;
		cout << "hand run iterations: " << itt << endl;
		cout << "hand run duration/itt: " << d2.count()/itt << " ms" << endl;
		}
	}

}
