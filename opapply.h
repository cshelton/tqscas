#ifndef OPAPPLY_H
#define OPAPPLY_H
#include "commonunion/splittype.h"
#include <type_traits>

template<typename OP, typename... ARGS>
struct opapply {
     template<typename,typename,typename> struct help1;
     template<typename,typename,typename> struct help2;


	template<typename, typename=void>
	struct validop {
		enum {value=false};
	};

     template<typename,typename,typename=void>
     struct help1 {
          typedef typeset<> type;
     };

     template<typename... SETARGS, typename EXPR1, typename... UNSETEXPRS>
     struct help1<typeset<SETARGS...>,typeset<EXPR1, UNSETEXPRS...>,void> {
          typedef typename help2<
               typeset<SETARGS...>,EXPR1,typeset<UNSETEXPRS...>
          >::type type;
     };
	
	template<typename... SETARGS>
	struct help1<typeset<SETARGS...>,typeset<>,
		decltype(void(
               std::declval<OP>().apply
					(std::declval<SETARGS>()...)))> {
          typedef typeset<decltype(
               std::declval<OP>().apply(std::declval<SETARGS>()...)
          )> type;
     };

     template<typename... SETARGS, typename ET1, typename... ETYPES, typename... LEXPRS>
     struct help2<typeset<SETARGS...>,typeset<ET1, ETYPES...>,typeset<LEXPRS...>> {
          typedef typename splittype::flattenmerge<
               typename help1<typeset<SETARGS...,ET1>,
                              typeset<LEXPRS...>>::type,
               typename help2<typeset<SETARGS...>,
                              typeset<ETYPES...>,
                              typeset<LEXPRS...>>::type
          >::type type;
     };

     template<typename... SETARGS, typename... LEXPRS>
     struct help2<typeset<SETARGS...>,typeset<>,typeset<LEXPRS...>> {
          typedef typeset<> type;
     };

     typedef typename help1<typeset<>,typeset<ARGS...>>::type type;
};

template<std::size_t C,typename OP, typename ARG, typename... ARGS>
struct opapplycard {
	typedef typename opapplycard<C-1,OP,ARG,ARG,ARGS...>::type type;
};

template<typename OP, typename ARG, typename... ARGS>
struct opapplycard<0,OP,ARG,ARGS...> {
	typedef typename opapply<OP,ARGS...>::type type;
};

template<typename OP, typename ARGS, std::size_t MINC, std::size_t MAXC>
struct opapplyall {
	typedef typename splittype::flattenmerge<
			typename opapplycard<MINC,OP,ARGS>::type,
			typename opapplyall<OP,ARGS,MINC+1,MAXC>::type
		>::type type;
};

template<typename OP, typename ARGS, std::size_t MINC>
struct opapplyall<OP,ARGS,MINC,MINC> {
	typedef typename opapplycard<MINC,OP,ARGS>::type type;
};


template<typename OP, typename=void>
struct opcards {
	enum {mincard=1};
	enum {maxcard=2};
};

template<typename OP>
struct opcards<OP,typename std::enable_if<OP::mincard!=1 || OP::maxcard!=2>::type> {
	enum {mincard=OP::mincard};
	enum {maxcard=OP::maxcard};
};

template<typename,typename> struct opsapply;

template<typename ARGSET>
struct opsapply<typeset<>,ARGSET> {
	typedef typeset<> type;
};

template<typename OP1, typename... OPS, typename ARGSET>
struct opsapply<typeset<OP1,OPS...>,ARGSET> {
	typedef typename splittype::flattenmerge<
			typename opapplyall<OP1,ARGSET,opcards<OP1>::mincard,opcards<OP1>::maxcard>::type,
			typename opsapply<typeset<OPS...>,ARGSET>::type
		>::type type;
};

template<typename,typename,typename> struct opsclosureitt;

template<typename OPS,typename ARGS> struct opsclosure {
	typedef typename splittype::flattenmerge<
			ARGS, typename opsapply<OPS,ARGS>::type>::type nextargs;
	typedef typename opsclosureitt<OPS,ARGS,nextargs>::type type;
};

template<typename OPS,typename OLDARGS, typename NEWARGS>
struct opsclosureitt {
	typedef typename opsclosure<OPS,NEWARGS>::type type;
};

template<typename OPS,typename ARGS>
struct opsclosureitt<OPS,ARGS,ARGS> {
	typedef ARGS type;
};

#endif
