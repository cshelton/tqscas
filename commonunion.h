#ifndef COMMONUNION_H
#define COMMONUNION_H

#include <utility>
#include <type_traits>
#include "splittype.h"
#include <cstdint>
#include "foreach.h"

namespace commonunion {
	struct blanktype {};

	template<std::size_t N>
	struct largeenoughint {
		typedef typename std::conditional<
				N<(1L<<8), uint_least8_t,
				typename std::conditional<
					N<(1L<<16), uint_least16_t,
				typename std::conditional<
					N<(1L<<32), uint_least32_t, uint_least64_t>::type
				>::type
			>::type type;
	};

	template<typename...> struct argstoindextype;

	template<typename... Rs,template<typename...> class H>
	struct argstoindextype<H<Rs...>> {
		typedef typename largeenoughint<sizeof...(Rs)>::type type;
	};


	template<typename T1, typename T2>
	struct sameuniontype1 {
		enum{value=0};
	};

	template<typename R, typename... Rs, typename... Ts, template<typename...> class H>
	struct sameuniontype1<H<R,Rs...>,H<Ts...>> {
		enum { value =
			isargtype<R,H<Ts...>>::value &&
			sameuniontype1<H<Rs...>,H<Ts...>>::value};
	};

	template<typename... Ts, template<typename...> class H>
	struct sameuniontype1<H<>,H<Ts...>> {
		enum {value=1};
	};

	template<typename...> struct alltrivialdestruct;
	template<typename T1, typename... Ts>
	struct alltrivialdestruct<T1,Ts...> {
		enum {value = (std::is_void<T1>::value || std::is_trivially_destructible<T1>::value)
					&& alltrivialdestruct<Ts...>::value};
	};
	template<>
	struct alltrivialdestruct<> {
		enum { value=1 };
	};

}

#define RTEXPRSELECT(uname,cond,restrue,resfalse) \
     (cond) ? uname<typename std::decay<decltype(restrue)>::type, \
                              typename std::decay<decltype(resfalse)>::type>{restrue} \
          : uname<typename std::decay<decltype(restrue)>::type, \
                              typename std::decay<decltype(resfalse)>::type>{resfalse}

#define WRITEIMPLN1(fname) \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &i, Rs &&...rs) const \
			noexcept(noexcept((i&1)==0 ? \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)): \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...)))) \
		{ \
			return (i&1)==0 ? \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)): \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...)); \
		} \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &i, Rs &&...rs) \
			noexcept(noexcept((i&1)==0 ? \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)): \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...)))) \
		{ \
			return (i&1)==0 ? \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)): \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...)); \
		}

#define WRITEIMPLN2(uname,fname) \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &i, Rs &&...rs) const \
			noexcept(noexcept(RTEXPRSELECT(uname,(i&1)==0, \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)), \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...))))) \
		{ \
			return RTEXPRSELECT(uname,(i&1)==0, \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)), \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...))); \
		} \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &i, Rs &&...rs) \
			noexcept(noexcept(RTEXPRSELECT(uname,(i&1)==0, \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)), \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...))))) \
		{ \
			return RTEXPRSELECT(uname,(i&1)==0, \
					(t1.fname##_impl(i>>1,std::forward<Rs>(rs)...)), \
					(t2.fname##_impl(i>>1,std::forward<Rs>(rs)...))); \
		}

#define WRITEIMPLX1(a1) HERE a1;
#define WRITEIMPLX2(a1,a2) HERE a1; a2;
#define WRITEIMPLX3(a1,a2,a3) HERE a1; a2; a3;

#define WRITEIMPL(ARG) CALLVAR_N2(WRITEIMPLN,STRIPPAREN(ARG))

#define WRITEBASEIMPLN1(fname) \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &, Rs &&...rs) const \
				noexcept(noexcept(t.fname(std::forward<Rs>(rs)...))) { \
			return t.fname(std::forward<Rs>(rs)...); \
		} \
 \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &, Rs &&...rs)  \
				noexcept(noexcept(t.fname(std::forward<Rs>(rs)...))) { \
			return t.fname(std::forward<Rs>(rs)...); \
		}

#define WRITEBASEIMPLN2(rettype,fname) \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &, Rs &&...rs) const \
				noexcept(noexcept(t.fname(std::forward<Rs>(rs)...))) { \
			return t.fname(std::forward<Rs>(rs)...); \
		} \
 \
		template<typename... Rs> \
		constexpr auto fname##_impl(const itype &, Rs &&...rs)  \
				noexcept(noexcept(t.fname(std::forward<Rs>(rs)...))) { \
			return t.fname(std::forward<Rs>(rs)...); \
		}

#define WRITEBASEIMPL(ARG) CALLVAR_N2(WRITEBASEIMPLN,STRIPPAREN(ARG))

#define WRITEDISPATCHN1(fname) \
	template<typename... Rs> \
	constexpr auto fname(Rs &&...rs) const \
			noexcept(noexcept(this->node.fname##_impl(this->tag,std::forward<Rs>(rs)...))) { \
		return node.fname##_impl(tag,std::forward<Rs>(rs)...); \
	} \
 \
	template<typename... Rs> \
	constexpr auto fname(Rs &&...rs)  \
			noexcept(noexcept(this->node.fname##_impl(this->tag,std::forward<Rs>(rs)...))) { \
		return node.fname##_impl(tag,std::forward<Rs>(rs)...); \
	}
#define WRITEDISPATCHN2(rettype,fname) \
	template<typename... Rs> \
	constexpr auto fname(Rs &&...rs) const \
			noexcept(noexcept(this->node.fname##_impl(this->tag,std::forward<Rs>(rs)...))) { \
		return node.fname##_impl(tag,std::forward<Rs>(rs)...); \
	} \
 \
	template<typename... Rs> \
	constexpr auto fname(Rs &&...rs)  \
			noexcept(noexcept(this->node.fname##_impl(this->tag,std::forward<Rs>(rs)...))) { \
		return node.fname##_impl(tag,std::forward<Rs>(rs)...); \
	}

#define WRITEDISPATCH(ARG) CALLVAR_N2(WRITEDISPATCHN,STRIPPAREN(ARG))


#define DEFUNION(name,baseclause,...) \
namespace commonunion { namespace name##_node_impl { \
	template<typename EN=void,typename... Ts> struct cu_impl; \
	template<typename EN=void,typename... Ns> struct cu_node; \
} } \
template<typename... Ts> using name = typename stripdupargs< \
			typename flattentype<commonunion::name##_node_impl::cu_impl<void>,Ts...>::type \
												>::type; \
	\
namespace commonunion { namespace name##_node_impl { \
	template<typename... Ns> \
	struct typecalc { \
		typedef cu_node<void,Ns...> noduptype;  \
		typedef typename commonunion::argstoindextype<noduptype>::type itype;  \
		typedef typename splitargtype_odds_voidfirst<noduptype>::type type1;  \
		typedef typename splitargtype_evens_voidfirst<noduptype>::type type2;  \
		typedef typename splitargtype_odds<typecalc<Ns...>>::type tc1; \
		typedef typename splitargtype_evens<typecalc<Ns...>>::type tc2; \
 \
		template<typename T, typename EN=void>  \
		struct index {  \
			enum {value = 0};  \
		};  \
  \
		template<typename T>  \
		struct index<T, typename std::enable_if<  \
				isargtype<typename std::decay<T>::type,tc1>::value>::type> {  \
			enum {value = tc1::template index<T>::value*2}; \
		};  \
  \
		template<typename T>  \
		struct index<T, typename std::enable_if<  \
				isargtype<typename std::decay<T>::type,tc2>::value>::type> {  \
			enum {value = 1 + tc2::template index<T>::value*2};  \
		};  \
	}; \
	template<typename T> \
	struct typecalc<T> { \
		typedef cu_node<void,T> noduptype; \
		typedef typename commonunion::argstoindextype<noduptype>::type itype;  \
		template<typename R> \
		struct index { \
			enum { value = 0 }; \
		};\
	};\
\
	template<typename N1, typename N2, typename... Ns> \
	struct cu_node<typename std::enable_if<!alltrivialdestruct<N1,N2,Ns...>::value>::type,N1,N2,Ns...> { \
		typedef typecalc<N1,N2,Ns...> tinfo; \
		typedef typename tinfo::noduptype noduptype; \
		typedef typename tinfo::itype itype; \
		typedef typename tinfo::type1 type1; \
		typedef typename tinfo::type2 type2; \
		union { \
			commonunion::blanktype t0; \
			type1 t1; \
			type2 t2; \
		}; \
 \
		constexpr cu_node() noexcept : t0() {}; \
 \
		template<typename T1, \
			typename std::enable_if<isargtype<typename std::decay<T1>::type, \
								decltype(t1)>::value>::type *EN=nullptr> \
		constexpr cu_node(T1 &&tt1) \
			noexcept(std::is_nothrow_constructible<type1, \
					decltype(std::forward<T1>(tt1))>::value)  \
			: t1(std::forward<T1>(tt1)) {} \
 \
		template<typename T2, \
			typename std::enable_if<isargtype<typename std::decay<T2>::type, \
								decltype(t2)>::value>::type *EN=nullptr> \
		constexpr cu_node(T2 &&tt2) \
			noexcept(std::is_nothrow_constructible<type2, \
					decltype(std::forward<T2>(tt2))>::value)  \
			: t2(std::forward<T2>(tt2)) {} \
 \
		constexpr void predestruct(const itype &i) \
				noexcept(noexcept(t1.predestruct(i)) \
					&& noexcept(t2.predestruct(i))) { \
			if ((i&1)==0) { \
				t1.predestruct(i>>1); \
				t1.~type1(); \
			} else { \
				t2.predestruct(i>>1); \
				t2.~type2(); \
			} \
		} \
 \
		~cu_node() /* noexcept */ {} /* noexcept causes clang crash \
				llvm.org/bugs/show_bug.cgi?id=23584 */ \
			 \
		template<typename T1, \
			typename std::enable_if<isargtype<typename std::decay<T1>::type, \
								decltype(t1)>::value>::type *EN=nullptr> \
		constexpr void assign(const itype &priori,T1 &&tt1) \
				noexcept(noexcept(t1.assign(priori>>1,std::forward<T1>(tt1))) \
					&& noexcept(t2.predestruct(priori>>1)) \
					&& std::is_nothrow_destructible<type2>::value) { \
			if ((priori&1)==0) \
				t1.assign(priori>>1,std::forward<T1>(tt1)); \
			else { \
				t2.predestruct(priori>>1); \
				t2.~type2(); \
				new(&t1) type1(std::forward<T1>(tt1)); \
			} \
		} \
 \
		template<typename T2, \
			typename std::enable_if<isargtype<typename std::decay<T2>::type, \
								decltype(t2)>::value>::type *EN=nullptr> \
		constexpr void assign(const itype &priori,T2 &&tt2) \
				noexcept(noexcept(t2.assign(priori>>1,std::forward<T2>(tt2))) \
					&& noexcept(t1.predestruct(priori>>1)) \
					&& std::is_nothrow_destructible<type1>::value) { \
			if ((priori&1)==0) { \
				t1.predestruct(priori>>1); \
				t1.~type1(); \
				new(&t2) type2(std::forward<T2>(tt2)); \
			} else t2.assign(priori>>1,std::forward<T2>(tt2)); \
		} \
 \
		template<typename T1, \
			typename std::enable_if<isargtype<typename std::decay<T1>::type, \
								decltype(t1)>::value>::type *EN=nullptr> \
		constexpr void init(T1 &&tt1) \
				noexcept(std::is_nothrow_constructible<type1, \
					decltype(std::forward<T1>(tt1))>::value) { \
			new(&t1) type1(std::forward<T1>(tt1)); \
		} \
 \
		template<typename T2, \
			typename std::enable_if<isargtype<typename std::decay<T2>::type, \
								decltype(t2)>::value>::type *EN=nullptr> \
		constexpr void init(T2 &&tt2) \
				noexcept(std::is_nothrow_constructible<type2, \
					decltype(std::forward<T2>(tt2))>::value) { \
			new(&t2) type2(std::forward<T2>(tt2)); \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &i, cu_impl<void,Rs...> &cu) const & \
				noexcept(noexcept(this->t1.assignto(i>>1,cu)) \
					&& noexcept(this->t2.assignto(i>>1,cu))) { \
			if ((i&1)==0) t1.assignto(i>>1,cu); \
			else t2.assignto(i>>1,cu); \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &i, cu_impl<void,Rs...> &cu) && \
				noexcept(noexcept(std::move(this->t1).assignto(i>>1,cu)) \
					&& noexcept(std::move(this->t2).assignto(i>>1,cu))) { \
			if ((i&1)==0) std::move(t1).assignto(i>>1,cu); \
			else std::move(t2).assignto(i>>1,cu); \
		} \
 \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &i, cu_impl<void,Rs...> &cu) const &\
				noexcept(noexcept(this->t1.assigntoinit(i>>1,cu)) \
					&& noexcept(this->t2.assigntoinit(i>>1,cu))) { \
			if ((i&1)==0) t1.assigntoinit(i>>1,cu); \
			else t2.assigntoinit(i>>1,cu); \
		} \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &i, cu_impl<void,Rs...> &cu) && \
			noexcept(noexcept(std::move(this->t1).assigntoinit(i>>1,cu)) \
				&& noexcept(std::move(this->t2).assigntoinit(i>>1,cu))) { \
			if ((i&1)==0) std::move(t1).assigntoinit(i>>1,cu); \
			else std::move(t2).assigntoinit(i>>1,cu); \
		} \
 \
 		FOREACH(WRITEIMPL,__VA_ARGS__) \
 \
	}; \
	\
	template<typename N1, typename N2, typename... Ns> \
	struct cu_node<typename std::enable_if<alltrivialdestruct<N1,N2,Ns...>::value>::type,N1,N2,Ns...> { \
		typedef typecalc<N1,N2,Ns...> tinfo; \
		typedef typename tinfo::noduptype noduptype; \
		typedef typename tinfo::itype itype; \
		typedef typename tinfo::type1 type1; \
		typedef typename tinfo::type2 type2; \
				 \
		union { \
			commonunion::blanktype t0; \
			type1 t1; \
			type2 t2; \
		}; \
 \
		constexpr cu_node() noexcept : t0() {}; \
 \
		template<typename T1, \
			typename std::enable_if<isargtype<typename std::decay<T1>::type, \
								decltype(t1)>::value>::type *EN=nullptr> \
		constexpr cu_node(T1 &&tt1) \
			noexcept(std::is_nothrow_constructible<type1, \
					decltype(std::forward<T1>(tt1))>::value)  \
			: t1(std::forward<T1>(tt1)) {} \
 \
		template<typename T2, \
			typename std::enable_if<isargtype<typename std::decay<T2>::type, \
								decltype(t2)>::value>::type *EN=nullptr> \
		constexpr cu_node(T2 &&tt2) \
			noexcept(std::is_nothrow_constructible<type2, \
					decltype(std::forward<T2>(tt2))>::value)  \
			: t2(std::forward<T2>(tt2)) {} \
 \
		constexpr void predestruct(const itype &i) const noexcept {} \
 \
		template<typename T1, \
			typename std::enable_if<isargtype<typename std::decay<T1>::type, \
								decltype(t1)>::value>::type *EN=nullptr> \
		constexpr void assign(const itype &priori,T1 &&tt1) \
				noexcept(noexcept(t1.assign(priori>>1,std::forward<T1>(tt1))) \
					&& noexcept(t2.predestruct(priori>>1)) \
					&& std::is_nothrow_destructible<type2>::value) { \
			if ((priori&1)==0) \
				t1.assign(priori>>1,std::forward<T1>(tt1)); \
			else { \
				/* t2.predestruct(priori>>1); \
				t2.~type2(); */ \
				new(&t1) type1(std::forward<T1>(tt1)); \
			} \
		} \
 \
		template<typename T2, \
			typename std::enable_if<isargtype<typename std::decay<T2>::type, \
								decltype(t2)>::value>::type *EN=nullptr> \
		constexpr void assign(const itype &priori,T2 &&tt2) \
				noexcept(noexcept(t2.assign(priori>>1,std::forward<T2>(tt2))) \
					&& noexcept(t1.predestruct(priori>>1)) \
					&& std::is_nothrow_destructible<type1>::value) { \
			if ((priori&1)==0) { \
				/* t1.predestruct(priori>>1); \
				t1.~type1(); */ \
				new(&t2) type2(std::forward<T2>(tt2)); \
			} else t2.assign(priori>>1,std::forward<T2>(tt2)); \
		} \
 \
		template<typename T1, \
			typename std::enable_if<isargtype<typename std::decay<T1>::type, \
								decltype(t1)>::value>::type *EN=nullptr> \
		constexpr void init(T1 &&tt1) \
				noexcept(std::is_nothrow_constructible<type1, \
					decltype(std::forward<T1>(tt1))>::value) { \
			new(&t1) type1(std::forward<T1>(tt1)); \
		} \
 \
		template<typename T2, \
			typename std::enable_if<isargtype<typename std::decay<T2>::type, \
								decltype(t2)>::value>::type *EN=nullptr> \
		constexpr void init(T2 &&tt2) \
				noexcept(std::is_nothrow_constructible<type2, \
					decltype(std::forward<T2>(tt2))>::value) { \
			new(&t2) type2(std::forward<T2>(tt2)); \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &i, cu_impl<void,Rs...> &cu) const & \
				noexcept(noexcept(this->t1.assignto(i>>1,cu)) \
					&& noexcept(this->t2.assignto(i>>1,cu))) { \
			if ((i&1)==0) t1.assignto(i>>1,cu); \
			else t2.assignto(i>>1,cu); \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &i, cu_impl<void,Rs...> &cu) && \
				noexcept(noexcept(std::move(this->t1).assignto(i>>1,cu)) \
					&& noexcept(std::move(this->t2).assignto(i>>1,cu))) { \
			if ((i&1)==0) std::move(t1).assignto(i>>1,cu); \
			else std::move(t2).assignto(i>>1,cu); \
		} \
 \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &i, cu_impl<void,Rs...> &cu) const &\
				noexcept(noexcept(this->t1.assigntoinit(i>>1,cu)) \
					&& noexcept(this->t2.assigntoinit(i>>1,cu))) { \
			if ((i&1)==0) t1.assigntoinit(i>>1,cu); \
			else t2.assigntoinit(i>>1,cu); \
		} \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &i, cu_impl<void,Rs...> &cu) && \
			noexcept(noexcept(std::move(this->t1).assigntoinit(i>>1,cu)) \
				&& noexcept(std::move(this->t2).assigntoinit(i>>1,cu))) { \
			if ((i&1)==0) std::move(t1).assigntoinit(i>>1,cu); \
			else std::move(t2).assigntoinit(i>>1,cu); \
		} \
 \
		template<typename T, typename EN=void> \
		struct index { \
			enum {value = 0}; \
		}; \
 \
		template<typename T> \
		struct index<T, typename std::enable_if< \
				isargtype<typename std::decay<T>::type,type1>::value>::type> { \
			enum {value = type1::template index<T>::value*2}; \
		}; \
 \
		template<typename T> \
		struct index<T, typename std::enable_if< \
				isargtype<typename std::decay<T>::type,type2>::value>::type> { \
			enum {value = 1 + type2::template index<T>::value*2}; \
		}; \
 \
 		FOREACH(WRITEIMPL,__VA_ARGS__) \
	}; \
 \
	template<typename T> \
	struct cu_node<typename std::enable_if<!std::is_trivially_destructible<T>::value>::type,T> { \
		typedef cu_node<void,T> noduptype; \
		union { \
			commonunion::blanktype t0; \
			T t; \
		}; \
 \
		typedef typename commonunion::largeenoughint<1>::type itype; \
 \
		constexpr cu_node() noexcept : t0() {} \
		constexpr cu_node(const T &tt) \
				noexcept(std::is_nothrow_copy_constructible<T>::value) \
			: t(tt) {} \
 \
		constexpr cu_node(T &&tt) \
				noexcept(std::is_nothrow_move_constructible<T>::value) \
			: t(std::move(tt)) {} \
 \
		constexpr void predestruct(const itype &) const \
				noexcept(std::is_nothrow_destructible<T>::value) { \
			t.~T(); \
		} \
 \
		~cu_node() /* noexcept */ {} /* causes clang to crash */ \
 \
		constexpr void assign(const itype &,const T &tt) \
				noexcept(noexcept(this->t=tt)) { \
			t = tt; \
		} \
		constexpr void assign(const itype &,T &&tt) \
				noexcept(noexcept(this->t=std::move(tt))) { \
			t = std::move(tt); \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &, cu_impl<void,Rs...> &cu) const & \
			noexcept(noexcept(cu=this->t)) { \
			cu = t; \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &, cu_impl<void,Rs...> &cu) && \
			noexcept(noexcept(cu=std::move(this->t))) { \
			cu = std::move(t); \
		} \
 \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &, cu_impl<void,Rs...> &cu) const & \
			noexcept(noexcept(cu.init(this->t))) { \
			cu.init(t); \
		} \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &, cu_impl<void,Rs...> &cu) && \
			noexcept(noexcept(cu.init(std::move(this->t)))) { \
			cu.init(std::move(t)); \
		} \
\
		constexpr void init(const T &tt) \
				noexcept(std::is_nothrow_constructible<T,const T &>::value) { \
			new(&t) T(tt); \
		} \
		constexpr void init(T &&tt) \
				noexcept(std::is_nothrow_constructible<T,T &&>::value) { \
			new(&t) T(std::move<T>(tt)); \
		} \
 \
		template<typename R> \
		struct index { \
			enum { value=0 }; \
		}; \
 \
 		FOREACH(WRITEBASEIMPL,__VA_ARGS__) \
	}; \
 \
	template<typename T> \
	struct cu_node<typename std::enable_if<std::is_trivially_destructible<T>::value>::type,T> { \
		typedef cu_node<void,T> noduptype; \
		union { \
			commonunion::blanktype t0; \
			T t; \
		}; \
 \
		typedef typename commonunion::largeenoughint<1>::type itype; \
 \
		constexpr cu_node() noexcept : t0() {} \
		constexpr cu_node(const T &tt) \
				noexcept(std::is_nothrow_copy_constructible<T>::value) \
			: t(tt) {} \
 \
		constexpr cu_node(T &&tt) \
				noexcept(std::is_nothrow_move_constructible<T>::value) \
			: t(std::move(tt)) {} \
 \
		constexpr void predestruct(const itype &) const noexcept {}\
 \
		constexpr void assign(const itype &,const T &tt) \
				noexcept(noexcept(this->t=tt)) { \
			t = tt; \
		} \
		constexpr void assign(const itype &,T &&tt) \
				noexcept(noexcept(this->t=std::move(tt))) { \
			t = std::move(tt); \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &, cu_impl<void,Rs...> &cu) const & \
			noexcept(noexcept(cu=this->t)) { \
			cu = t; \
		} \
 \
		template<typename... Rs> \
		constexpr void assignto(const itype &, cu_impl<void,Rs...> &cu) && \
			noexcept(noexcept(cu=std::move(this->t))) { \
			cu = std::move(t); \
		} \
 \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &, cu_impl<void,Rs...> &cu) const & \
			noexcept(noexcept(cu.init(this->t))) { \
			cu.init(t); \
		} \
 \
		template<typename... Rs> \
		constexpr void assigntoinit(const itype &, cu_impl<void,Rs...> &cu) && \
			noexcept(noexcept(cu.init(std::move(this->t)))) { \
			cu.init(std::move(t)); \
		} \
\
		constexpr void init(const T &tt) \
				noexcept(std::is_nothrow_constructible<T,const T &>::value) { \
			new(&t) T(tt); \
		} \
		constexpr void init(T &&tt) \
				noexcept(std::is_nothrow_constructible<T,T &&>::value) { \
			new(&t) T(std::move<T>(tt)); \
		} \
 \
		template<typename R> \
		struct index { \
			enum { value=0 }; \
		}; \
 \
 		FOREACH(WRITEBASEIMPL,__VA_ARGS__) \
	}; \
 \
	template<typename... Ts> \
	struct cu_impl<typename std::enable_if<!alltrivialdestruct<Ts...>::value>::type \
			,Ts...> baseclause { \
	 \
	 \
		typedef typename typecalc<Ts...>::noduptype nodeT; \
		template<typename R> \
		using nodeindex=typename typecalc<Ts...>::template index<R>; \
	 \
		template<typename T> \
		struct ismytype { enum {value=0}; }; \
		template<typename... Rs> \
		struct ismytype<cu_impl<void,Rs...>> { enum {value=1}; }; \
	 \
		template<typename T, \
			typename std::enable_if<!ismytype<typename std::decay<T>::type>::value>::type *EN=nullptr> \
		constexpr cu_impl(T &&tt) \
			noexcept(std::is_nothrow_constructible<nodeT, \
						decltype(std::forward<T>(tt))>::value) \
			: node(std::forward<T>(tt)) \
			, tag(nodeindex<typename std::decay<T>::type>::value) \
		{} \
	 \
		constexpr cu_impl(const cu_impl<void,Ts...> &cu) : node(), tag(0) { \
			cu.node.assigntoinit(cu.tag,*this); \
		} \
	 \
		constexpr cu_impl(cu_impl<void,Ts...> &&cu) : node(), tag(0) { \
			std::move(cu.node).assigntoinit(cu.tag,*this); \
		} \
	 \
		template<typename...> \
		struct cunodecount { enum {value=0}; }; \
		template<typename... Rs> \
		struct cunodecount<cu_node<void,Rs...>> { enum {value=sizeof...(Rs)}; }; \
		template<typename... Rs> \
		struct issingleton { enum {value=cunodecount<typename typecalc<Rs...>::noduptype>::value==1}; }; \
	 \
		template<typename... Rs, \
		   typename std::enable_if<!issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(const cu_impl<void,Rs...> &cu) \
			noexcept(std::is_nothrow_constructible<nodeT>::value \
				& noexcept(cu.node.assigntoinit(cu.tag,*this))) \
					: node(), tag(0) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
			cu.node.assigntoinit(cu.tag,*this); \
		} \
	 \
		template<typename... Rs, \
		   typename std::enable_if<!issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(cu_impl<void,Rs...> &&cu) \
			noexcept(std::is_nothrow_constructible<nodeT>::value \
				& noexcept(std::move(cu.node).assigntoinit(cu.tag,*this))) \
					: node(), tag(0) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
			std::move(cu.node).assigntoinit(cu.tag,*this); \
		} \
	 \
		template<typename... Rs, \
		   typename std::enable_if<issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(const cu_impl<void,Rs...> &cu)  \
			noexcept(std::is_nothrow_constructible<nodeT, \
									decltype(cu.node.t)>::value) \
				: node(cu.node.t), tag(nodeindex<typename std::decay<decltype(cu.node.t)>::type>::value) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
		} \
	 \
		template<typename... Rs, \
		   typename std::enable_if<issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(cu_impl<void,Rs...> &&cu) \
			noexcept(std::is_nothrow_constructible<nodeT, \
								decltype(std::move(cu.node.t))>::value) \
				: node(std::move(cu.node.t)), \
					tag(nodeindex<typename std::decay<decltype(cu.node.t)>::type>::value) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
		} \
	 \
	 \
		~cu_impl() noexcept(noexcept(this->node.predestruct(this->tag))) { \
			node.predestruct(tag); \
		} \
	 \
		template<typename T, \
			typename std::enable_if<isargtype<typename std::decay<T>::type, \
						typename typecalc<Ts...>::noduptype>::value>::type *EN=nullptr> \
		constexpr cu_impl<void,Ts...> &operator=(T &&tt) \
			noexcept(noexcept(this->node.assign(this->tag,std::forward<T>(tt)))) \
		{ \
			node.assign(tag,std::forward<T>(tt)); \
			tag = nodeindex<typename std::decay<T>::type>::value; \
			return *this; \
		} \
	 \
		template<typename T, \
			typename std::enable_if<isargtype<typename std::decay<T>::type, \
						typename typecalc<Ts...>::noduptype>::value>::type *EN=nullptr> \
		constexpr void init(T &&tt) \
				noexcept(noexcept(this->node.init(std::forward<T>(tt)))) { \
			node.init(std::forward<T>(tt)); \
			tag = nodeindex<typename std::decay<T>::type>::value; \
		} \
	 \
		template<typename... Rs> \
		constexpr cu_impl<void,Ts...> &operator=(const cu_impl<void,Rs...> &cu) \
				noexcept(noexcept(cu.node.assignto(cu.tag,*this))) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...> \
					>::value,"assignment is not type-safe"); \
			cu.node.assignto(cu.tag,*this); \
			return *this; \
		} \
	 \
		template<typename... Rs> \
		constexpr cu_impl<void,Ts...> &operator=(cu_impl<void,Rs...> &&cu) \
				noexcept(noexcept(std::move(cu.node).assignto(cu.tag,*this))) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"assignment is not type-safe"); \
			std::move(cu.node).assignto(cu.tag,*this); \
			return *this; \
		} \
	 \
		template<typename T> \
		constexpr bool istype() const noexcept { \
			return tag==nodeindex<T>::value; \
		} \
	 \
		FOREACH(WRITEDISPATCH,__VA_ARGS__) \
	 \
		nodeT node; \
		typename nodeT::itype tag; \
	}; \
	 \
	template<typename... Ts> \
	struct cu_impl<typename std::enable_if<alltrivialdestruct<Ts...>::value>::type \
			,Ts...> baseclause { \
	 \
		typedef typename typecalc<Ts...>::noduptype nodeT; \
		template<typename R> \
		using nodeindex=typename typecalc<Ts...>::template index<R>; \
	 \
		template<typename T> \
		struct ismytype { enum {value=0}; }; \
		template<typename... Rs> \
		struct ismytype<cu_impl<void,Rs...>> { enum {value=1}; }; \
	 \
		template<typename T, \
			typename std::enable_if<!ismytype<typename std::decay<T>::type>::value>::type *EN=nullptr> \
		constexpr cu_impl(T &&tt) \
			noexcept(std::is_nothrow_constructible<nodeT, \
						decltype(std::forward<T>(tt))>::value) \
			: node(std::forward<T>(tt)) \
			, tag(nodeindex<typename std::decay<T>::type>::value) \
		{} \
	 \
		constexpr cu_impl(const cu_impl<void,Ts...> &cu) : node(), tag(0) { \
			cu.node.assigntoinit(cu.tag,*this); \
		} \
	 \
		constexpr cu_impl(cu_impl<void,Ts...> &&cu) : node(), tag(0) { \
			std::move(cu.node).assigntoinit(cu.tag,*this); \
		} \
	 \
		template<typename...> \
		struct cunodecount { enum {value=0}; }; \
		template<typename... Rs> \
		struct cunodecount<cu_node<void,Rs...>> { enum {value=sizeof...(Rs)}; }; \
		template<typename... Rs> \
		struct issingleton { enum {value=cunodecount<typename typecalc<Rs...>::noduptype>::value==1}; }; \
	 \
		template<typename... Rs, \
		   typename std::enable_if<!issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(const cu_impl<void,Rs...> &cu) \
			noexcept(std::is_nothrow_constructible<nodeT>::value \
				& noexcept(cu.node.assigntoinit(cu.tag,*this))) \
					: node(), tag(0) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
			cu.node.assigntoinit(cu.tag,*this); \
		} \
	 \
		template<typename... Rs, \
		   typename std::enable_if<!issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(cu_impl<void,Rs...> &&cu) \
			noexcept(std::is_nothrow_constructible<nodeT>::value \
				& noexcept(std::move(cu.node).assigntoinit(cu.tag,*this))) \
					: node(), tag(0) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
			std::move(cu.node).assigntoinit(cu.tag,*this); \
		} \
	 \
		template<typename... Rs, \
		   typename std::enable_if<issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(const cu_impl<void,Rs...> &cu)  \
			noexcept(std::is_nothrow_constructible<nodeT, \
									decltype(cu.node.t)>::value) \
				: node(cu.node.t), tag(nodeindex<typename std::decay<decltype(cu.node.t)>::type>::value) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
		} \
	 \
		template<typename... Rs, \
		   typename std::enable_if<issingleton<Rs...>::value>::type *EN=nullptr> \
		constexpr cu_impl(cu_impl<void,Rs...> &&cu) \
			noexcept(std::is_nothrow_constructible<nodeT, \
								decltype(std::move(cu.node.t))>::value) \
				: node(std::move(cu.node.t)), \
					tag(nodeindex<typename std::decay<decltype(cu.node.t)>::type>::value) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"construction is not type-safe"); \
		} \
	 \
	 \
		template<typename T, \
			typename std::enable_if<isargtype<typename std::decay<T>::type, \
						typename typecalc<Ts...>::noduptype>::value>::type *EN=nullptr> \
		constexpr cu_impl<void,Ts...> &operator=(T &&tt) \
			noexcept(noexcept(this->node.assign(this->tag,std::forward<T>(tt)))) \
		{ \
			node.assign(tag,std::forward<T>(tt)); \
			tag = nodeindex<typename std::decay<T>::type>::value; \
			return *this; \
		} \
	 \
		template<typename T, \
			typename std::enable_if<isargtype<typename std::decay<T>::type, \
						typename typecalc<Ts...>::noduptype>::value>::type *EN=nullptr> \
		constexpr void init(T &&tt) \
				noexcept(noexcept(this->node.init(std::forward<T>(tt)))) { \
			node.init(std::forward<T>(tt)); \
			tag = nodeindex<typename std::decay<T>::type>::value; \
		} \
	 \
		template<typename... Rs> \
		constexpr cu_impl<void,Ts...> &operator=(const cu_impl<void,Rs...> &cu) \
				noexcept(noexcept(cu.node.assignto(cu.tag,*this))) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"assignment is not type-safe"); \
			cu.node.assignto(cu.tag,*this); \
			return *this; \
		} \
	 \
		template<typename... Rs> \
		constexpr cu_impl<void,Ts...> &operator=(cu_impl<void,Rs...> &&cu) \
				noexcept(noexcept(std::move(cu.node).assignto(cu.tag,*this))) { \
			static_assert(commonunion::sameuniontype1< \
						typecalc<Rs...>, \
						typecalc<Ts...>\
					>::value,"assignment is not type-safe"); \
			std::move(cu.node).assignto(cu.tag,*this); \
			return *this; \
		} \
	 \
		template<typename T> \
		constexpr bool istype() const noexcept { \
			return tag==nodeindex<T>::value; \
		} \
	 \
		FOREACH(WRITEDISPATCH,__VA_ARGS__) \
	 \
		nodeT node; \
		typename nodeT::itype tag; \
	}; \
\
} } \
\

#endif

