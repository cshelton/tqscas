#ifndef SCALARTYPE_HPP
#define SCALARTYPE_HPP

#include <boost/rational.hpp>
#include <boost/operators.hpp>
#include <sstream>
#include <iomanip>
#include <iostream>

struct scalarreal : boost::operators<scalarreal> {
	template<typename... T>
	using variant = boost::variant<T...>;
	using rtype = boost::rational<int>;

	struct eulerconst {};
	struct piconst {};

	variant<rtype,double,eulerconst,piconst> v;

	scalarreal(double val) : v(val) {}
	explicit scalarreal(eulerconst ec) : v(ec) {}
	explicit scalarreal(piconst pc) : v(pc) {}
	explicit scalarreal(rtype rt) : v(std::move(rt)) {}

	scalarreal(int i) : v(rtype(i)) {}

	template<typename I1, typename I2>
	explicit scalarreal(I1 &&num, I2 &&den) : v(rtype(std::forward<I1>(num),std::forward<I2>(den))) {}

	/*
	scalarreal(scalarreal &r) : v(r.v) {}
	scalarreal(const scalarreal &r) : v(r.v) {}
	scalarreal(scalarreal &&r) : v(std::move(r.v)) {}
	*/

	explicit operator double() const {
		switch(v.index()) {
			case 0: return (double)(boost::get<rtype>(v).numerator())/(double)(boost::get<rtype>(v).denominator());
			case 1: return boost::get<double>(v);
			case 2: return std::exp(1.0);
			case 3: return std::acos(-1.0);
			default: return 0.0;
		}
	}

	explicit operator int() const {
		return asint();
	}

	scalarreal operator-() const {
		if (v.index()==0) return scalarreal{-boost::get<rtype>(v)};
		else return scalarreal{-(double)(*this)};
		return *this;
	}

	scalarreal &operator+=(const scalarreal &s) {
		if (v.index()==0 && s.v.index()==0) boost::get<rtype>(v) += boost::get<rtype>(s.v);
		else v = (double)(*this) + (double)(s);
		return *this;
	}
	scalarreal &operator-=(const scalarreal &s) {
		if (v.index()==0 && s.v.index()==0) boost::get<rtype>(v) -= boost::get<rtype>(s.v);
		else v = (double)(*this) - (double)(s);
		return *this;
	}
	scalarreal &operator*=(const scalarreal &s) {
		if (v.index()==0 && s.v.index()==0) boost::get<rtype>(v) *= boost::get<rtype>(s.v);
		else v = (double)(*this) * (double)(s);
		return *this;
	}
	scalarreal &operator/=(const scalarreal &s) {
		if (v.index()==0 && s.v.index()==0 && s) boost::get<rtype>(v) /= boost::get<rtype>(s.v);
		else v = (double)(*this) / (double)(s);
		return *this;
	}
	bool operator<(const scalarreal &s) const {
		if (v.index()==0 && s.v.index()==0) return boost::get<rtype>(v) < boost::get<rtype>(s.v);
		else return (double)(*this) < (double)(s);
	}
	bool operator==(const scalarreal &s) const {
		if (v.index()==0 && s.v.index()==0) return boost::get<rtype>(v) == boost::get<rtype>(s.v);
		else return (double)(*this) == (double)(s);
	}
	bool operator==(const double &d) const {
		return (double)(*this) == d;
	}
	bool operator<(const double &d) const {
		return (double)(*this) < d;
	}
	bool operator==(const int &i) const {
		switch (v.index()) {
			case 0: return boost::get<rtype>(v)==i;
			case 1: return (double)(*this) == i;
			default: return false;
		}
	}
	bool operator<(const int &i) const {
		if (v.index()==0) return boost::get<rtype>(v)<i;
		else return (double)(*this) < i;
	}
	const scalarreal &operator++() {
		if (v.index()==0) boost::get<rtype>(v)++;
		else v = (double)(*this)+1.0;
		return *this;
	}
	const scalarreal &operator--() {
		if (v.index()==0) boost::get<rtype>(v)--;
		else v = (double)(*this)-1.0;
		return *this;
	}
		
	explicit operator bool() const {
		switch(v.index()) {
			case 0: return (bool)(boost::get<rtype>(v));
			case 1: return boost::get<double>(v)!=0.0;
			case 2: return true;
			case 3: return true;
			default: return true;
		}
	}
	bool operator!() const {
		switch(v.index()) {
			case 0: return !(boost::get<rtype>(v));
			case 1: return boost::get<double>(v)==0.0;
			case 2: return false;
			case 3: return false;
			default: return false;
		}
	}

	bool isint() const {
		switch(v.index()) {
			case 0: return boost::get<rtype>(v).numerator() % boost::get<rtype>(v).denominator() == 0;
			case 1: return std::fmod(boost::get<double>(v),1.0) == 0.0;
			default: return false;
		}
	}

	bool iseven() const {
		return isint() && asint()%2==0;
	}
	bool isodd() const {
		return isint() && asint()%2==1;
	}

	scalarreal round() const {
		switch(v.index()) {
			case 0: {
				   auto &r = boost::get<rtype>(v);
				   auto div = std::div(r.numerator(),r.denominator());
				   // away from zero... as in std::round
				   if (div.rem*2<=-abs(r.denominator()))
					   return div.quot - 1;
				   else if (div.rem*2>=abs(r.denominator()))
					   return div.quot + 1;
				   else return div.quot;
			   }
			default: return std::round((double)(*this));
		}
	}

	scalarreal floor() const {
		switch(v.index()) {
			case 0: {
				   auto &r = boost::get<rtype>(v);
				   auto div = std::div(r.numerator(),r.denominator());
				   if (div.rem<0)
					   return div.quot - 1;
				   else return div.quot;
			   }
			default: return std::floor((double)(*this));
		}
	}

	scalarreal ceil() const {
		switch(v.index()) {
			case 0: {
				   auto &r = boost::get<rtype>(v);
				   auto div = std::div(r.numerator(),r.denominator());
				   if (div.rem>0)
					   return div.quot + 1;
				   else return div.quot;
			   }
			default: return std::ceil((double)(*this));
		}
	}

	int asint() const {
		switch(v.index()) {
			case 0: return boost::get<rtype>(v).numerator() / boost::get<rtype>(v).denominator();
			default: return std::round((double)(*this));
		}
	}

	bool iszero() const {
		switch(v.index()) {
			case 0: return boost::get<rtype>(v).numerator() ==0;
			case 1: return boost::get<double>(v) == 0.0;
			default: return false;
		}
	}
};

namespace std {
	template<> struct numeric_limits<scalarreal> {
		static scalarreal infinity() {
			return scalarreal{std::numeric_limits<double>::infinity()};
		}
		static constexpr bool is_specialized = true;
		static constexpr bool is_signed = true;
		static constexpr bool is_integer = false;
		static constexpr bool is_exact = false;
		static constexpr int digits = numeric_limits<double>::digits;
		static constexpr int digits10 = numeric_limits<double>::digits10;
		static constexpr int radix = numeric_limits<double>::radix;
	};

	bool isfinite(const scalarreal &s) {
		switch(s.v.index()) {
			case 1: return std::isfinite(boost::get<double>(s.v));
			default: return true;
		}
	}
}

auto round(const scalarreal &s) {
	return s.round();
}
auto ceil(const scalarreal &s) {
	return s.ceil();
}
auto floor(const scalarreal &s) {
	return s.floor();
}

std::string tostring(const scalarreal &s) {
	if (s.isint()) return std::to_string(s.asint());
	switch (s.v.index()) {
		case 0: {
				   auto &r = boost::get<scalarreal::rtype>(s.v);
				   return std::string("(")+std::to_string(r.numerator())+"/"+std::to_string(r.denominator())+")";
			   }
		case 1: {
				   std::ostringstream ss;
				   ss << std::setprecision(17) << boost::get<double>(s.v);
				   return {ss.str()};
			   }
		case 2: return {"e"};
		case 3: return {"pi"};
		default: return {"?"};
	}
}

/*
std::ostream &operator<<(std::ostream &os, const scalarreal &s) {
	return os << tostring(s);
}

std::istream &operator>>(std::istream &is, scalarreal &s) {
	// not implemented
	return is;
}
*/

scalarreal abs(const scalarreal &s) {
	if (s.v.index()==0)
		return scalarreal{abs(boost::get<scalarreal::rtype>(s.v))};
	return scalarreal{(double)std::abs((double)(s))};
}

scalarreal log(const scalarreal &e) {
	if (e==1.0) return scalarreal{0};
	switch(e.v.index()) {
		case 2: return scalarreal{1};
		default: return log((double)e);
	}
}

template<typename T>
T intpow1(T b, int e) {
	if (e==1) return b;
	T ret = intpow1(b,e/2);
	ret *= ret;
	if (e&1) ret *= b;
	return ret;
}

template<typename T>
T intpow(T b, int e) {
	if (e==0) return T{1};
	if (e<0) return T{1}/intpow1(b,-e);
	return intpow1(b,e);
}

scalarreal pow(const scalarreal &b, const int &e) {
	if (e==1) return b;
	if (e==0) return scalarreal{1};
	if (e==-1) return 1/b;
	if (b.v.index()==2)
		return scalarreal{std::exp((double)e)};
	if (b.v.index()==1)
		return scalarreal{ std::pow((double)b,(double)e) };
	if (b==1) return scalarreal{1};
	if (b.v.index()==0)
		return intpow(b,e);
	else
		return scalarreal{ std::pow((double)b,(double)e) };
}

scalarreal pow(const scalarreal &b, const scalarreal &e) {
	if (e==1) return b;
	if (e==0) return scalarreal{1};
	if (e==-1) return 1/b;
	if (b.v.index()==2)
		return scalarreal{std::exp((double)e)};
	if (b.v.index()==1 || e.v.index()==1)
		return scalarreal{ std::pow((double)b,(double)e) };
	if (b==1) return scalarreal{1};
	if (b.v.index()==0 && e.isint())
		return intpow(b,e.asint());
	else
		return scalarreal{ std::pow((double)b,(double)e) };
}

#endif
