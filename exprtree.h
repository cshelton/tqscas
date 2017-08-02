#ifndef EXPRTREE_H
#define EXPRTREE_H

#include "ctstring.h"
#include "commonunion/commonunion.h"
#include "typeset.h"
#include "opapply.h"
#include <tuple>
#include <vector>

template<typename ETYPE>
struct expr {
	typedef typename splittype::stripdupargs<
		typename splittype::flattentype<typeset<>,ETYPE>::type>::type etypes;
};

// Constants:

template<typename ETYPE, ETYPE VAL>
struct expr_const_ct : expr<ETYPE> {
};

template<typename ETYPE>
struct expr_const_rt : expr<ETYPE> {
	commonunion::cu_impl<ETYPE> val;
};
template<typename... ETYPES>
struct expr_const_rt<typeset<ETYPES...>> : expr<typeset<ETYPES...>> {
	commonunion::cu_impl<ETYPES...> val;
};

// Symbols:

template<typename,typename> struct expr_sym_ct {};

template<typename ETYPE, char... CS>
struct expr_sym_ct<ETYPE,ctstring<CS...>> : expr<ETYPE> {
};

template<typename ETYPE>
struct expr_sym_rt : expr<ETYPE> {
	std::string name;
};

// Operators:

template<typename OP, typename... ARGS>
struct expr_op_ct : expr<decltype(
		std::declval<OP>().apply(std::declval<ARGS>()...))> {
	OP op;
	std::tuple<ARGS...> args;
};

template<typename,typename> struct expr_rt_expand;
template<typename,typename> struct expr_op_rt_expand;

template<typename OPSET, typename ETYPESET>
struct expr_op_rt : expr_op_rt_expand<OPSET,
				typename opsclosure<OPSET,ETYPESET>::type> {
};

template<typename... OPS, typename ETYPESET>
struct expr_op_rt_expand<typeset<OPS...>,ETYPESET> : expr<ETYPESET> {
	commonunion::cu_impl<OPS...> op;
	std::vector<expr_rt_expand<typeset<OPS...>,ETYPESET>> args;
};

// General expression:
//
template<typename OPSET, typename ETYPESET>
struct expr_rt : expr_rt_expand<OPSET,typename opsclosure<OPSET,ETYPESET>::type> {
};

template<typename OPSET, typename ETYPESET>
struct expr_rt_expand : expr<ETYPESET> {
	commonunion::cu_impl<
			expr_const_rt<ETYPESET>,
			expr_sym_rt<ETYPESET>,
			expr_op_rt_expand<OPSET,ETYPESET>>
					impl;
};

#endif
