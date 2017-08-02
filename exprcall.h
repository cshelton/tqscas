#ifndef EXPRCALL_H
#define EXPRCALL_H

template<typename F, typename ETYPE, ETYPE VAL>
constexpr auto callonexpr(const F &f, const expr_const_ct<ETYPE,VAL> &e) {
	return f.evalconst(VAL);
}

template<typename F, typename ETYPE>
constexpr auto callonexpr(const F &f, const expr_const_rt<ETYPE> &e) {
	return f.evalconst(e.val);
}

template<typename F, typename ETYPE>
constexpr auto callonexpr(const F &f, expr_const_rt<ETYPE> &&e) {
	return f.evalconst(std::move(e.val));
}

template<typename F, typename ETYPE, typename NAME>
constexpr auto callonexpr(const F &f, const expr_sym_ct<ETYPE,NAME> &e) {
	return f.evalsym<ETYPE>(NAME);
}

template<typename F, typename ETYPE>
constexpr auto callonexpr(const F &f, const expr_sym_rt<ETYPE> &e) {
	return f.evalsym<ETYPE>(e.name);
}

template<typename F, typename ETYPE>
constexpr auto callonexpr(const F &f, expr_sym_rt<ETYPE> &&e) {
	return f.evalsym<ETYPE>(std::move(e.name));
}

// a method to invoke val using arguments from a tuple:
// on stackoverflow 7858817, answer by Walter
// (edited slightly)
// helper class
template<typename F, template<typename...> class Params,
		typename... Args, std::size_t... I>
constexpr auto call_helper(const F &f,
		Params<Args...> const &params, std::index_sequence<I...>) {
	return f(std::get<I>(params)...);
}
// "return func(params...)"
template<typename F, template<typename...> class Params, typename... Args>
constexpr auto call(const F &f, const Params<Args...> &params) {
	return call_helper(std::forward<F>(f),params,
			std::index_sequence_for<Args...>{});
}

template<typename F, typename OP, typename... ARGS>
constexpr auto callonexpr(const F &f, const expr_op_ct<OP,ARGS> &e) {
	struct callevalop {
		callevalop(const F &ff, const OP &oopp) : f(ff), op(oopp) {}

		const F &f;
		const OP &op;

		template<typename... ARGS>
		constexpr auto operator()(ARGS &&...args) {
			return f.evalop(op,std::forward<ARGS>(args)...);
		}
	};
	return call(callevalop{f,e.op},e.args);
}

template<typename F, typename OP, typename... ARGS>
constexpr auto callonexpr(const F &f, const expr_op_ct<OP,ARGS> &e) {
	struct callevalop {
		callevalop(const F &ff, const OP &oopp) : f(ff), op(oopp) {}

		const F &f;
		const OP &op;

		template<typename... ARGS>
		constexpr auto operator()(ARGS &&...args) {
			return f.evalop(op,std::forward<ARGS>(args)...);
		}
	}; //sigh... this would be easier if constexpr allowed lambda fns
	return call(callevalop{f,e.op},e.args);
}

template<typename F, typename OPSET, typename ETYPESET>
constexpr auto callonexpr(const F &f, const expr_op_rt_expand<OPSET,ETYPESET> &e) {
	struct callevalop {
		template<typename OP, typename F, typename... ARGS>
		constexpr auto operator()(OP &&op, const F &f, ARGS &&...args) {
			return f.evalop(std::forward<OP>(op),std::forward<ARGS>(args)...);
		}
	};
	return e.op.callfn(callevalop{},f,e.args);
}

template<typename F, typename OPSET, typename ETYPESET>
constexpr auto callonexpr(const F &f, const expr_rt_expand<OPSET,ETYPESET> &e) {
	struct callevalop {
		template<typename E, typename F, typename... ARGS>
		constexpr auto operator()(E &&e, const F &f) {
			return callonexpr(f,std::forward<E>(e));
		}
	};
	return e.impl.callfn(callevalop{},f);
}

template<typename F, typename COMB, typename T, typename ARG1, typename... ARGS>
constexpr auto callonargs(const F &f, const COMB &comb, T &&baseval,
		ARG1 &&arg1, ARGS &&...args) {
	return comb(callonexpr(f,std::forward<ARG1>(arg1)),
			callonargs(f,comb,std::forward<T>(baseval),std::forward<ARGS>(args)...));
}

template<typename F, typename COMB, typename T>
constexpr auto callonargs(const F &f, const COMB &comb, T &&baseval) {
	return baseval;
}

template<typename F, typename COMB, typename T, typename E>
constexpr auto callonargs(const F &f, const COMB &comb, T &&baseval,
		const std::vector<E> &args) {
	T ret{std::forward<T>(baseval)};
	for(auto &&x : args) ret = comb(ret,callonexpr(f,x));
	return ret;
}

#endif
