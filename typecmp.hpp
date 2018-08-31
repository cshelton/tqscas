#ifndef TYPECMP_HPP
#define TYPECMP_HPP

#include <type_traits>
#include <variant>

constexpr size_t cstrlen(const char* p) {
	size_t len = 0;
	while (*p) {
		++len;
		++p;
	}
	return len;
}

template<typename T1, typename T2>
constexpr bool cmp() {
	// typeid::before is not constexpr
	/* NO GOOD:
	return typeid(T1).before(typeid(T2));
	*/

	// typeid::name is not constexpr
	/* NO GOOD:
	const char *n1 = typeid(T1).name();
	const char *n2 = typeid(T1).name();
	std::size_t i = 0;
	while(n1[i] && n2[i] && n1[i]==n2[i]) i++;
	return (n1[i] || n2[i]);
	*/
	
	// not sufficient (for instance, long and double are the same!)
	/* NO GOOD:
	return sizeof(T1) < sizeof(T2);
	*/
	if constexpr (std::is_same_v<T1,std::monostate>)
		return !std::is_same_v<T2,std::monostate>;
	else if constexpr(std::is_same_v<T2,std::monostate>)
		return 0;
	else if constexpr (sizeof(T1)!=sizeof(T2))
		return sizeof(T1) < sizeof(T2);
	else {
		// what goes here is troublesome and cannot be done
		// exactly (at least as of Aug 2018)
		// see https://stackoverflow.com/questions/48723974/how-to-order-types-at-compile-time
		const char *pf = __PRETTY_FUNCTION__;	
		const char* a = pf + 
#ifdef __clang__
			cstrlen("bool cmp() [T = ")
#else
			cstrlen("constexpr bool cmp() [with T = ")
#endif
			;

		const char* b = a + 1;
#ifdef __clang__
		while (*b != ',') ++b;
#else
		while (*b != ';') ++b;
#endif
		std::size_t a_len = b - a;
		b += cstrlen("; U = ");
		const char* end = b + 1;
		while (*end != ']') ++end;
		std::size_t b_len = end - b;    

		for (std::size_t i = 0; i < std::min(a_len, b_len); ++i) {
			if (a[i] != b[i]) return a[i] < b[i];
		}

		return a_len < b_len;
	}
}
	

#endif
