#ifndef OPS_H
#define OPS_H

struct opplus {
	template<typename ARG>
	constexpr auto apply(ARG &&arg) {
		return std::forward<ARG>(arg);
	}
	template<typename ARG1,typename ARG2>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2) {
		return std::forward<ARG1>(arg1)+std::forward<ARG2>(arg2);
	}
	template<typename ARG1, typename ARG2, typename ARG3, typename... ARGS>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2, ARG3 &&arg3, ARGS &&...args) {
		return apply(std::forward<ARG1>(arg1) + std::forward<ARG2>(arg2),
			apply(std::forward<ARG3>(arg3),std::forward<ARGS>(args)...));
	}

	enum { mincard=2 };
	enum { maxcard=2 };
};

struct opminus {
	template<typename ARG>
	constexpr auto apply(ARG &&arg) {
		return std::forward<ARG>(arg);
	}
	template<typename ARG1,typename ARG2>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2) {
		return std::forward<ARG1>(arg1)-std::forward<ARG2>(arg2);
	}
	template<typename ARG1, typename ARG2, typename ARG3, typename... ARGS>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2, ARG3 &&arg3, ARGS &&...args) {
		return apply(std::forward<ARG1>(arg1) - std::forward<ARG2>(arg2),
			apply(std::forward<ARG3>(arg3),std::forward<ARGS>(args)...));
	}

	enum { mincard=2 };
	enum { maxcard=2 };
};

struct opmult {
	template<typename ARG>
	constexpr auto apply(ARG &&arg) {
		return std::forward<ARG>(arg);
	}
	template<typename ARG1,typename ARG2>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2) {
		return std::forward<ARG1>(arg1)*std::forward<ARG2>(arg2);
	}
	template<typename ARG1, typename ARG2, typename ARG3, typename... ARGS>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2, ARG3 &&arg3, ARGS &&...args) {
		return apply(std::forward<ARG1>(arg1) * std::forward<ARG2>(arg2),
			apply(std::forward<ARG3>(arg3),std::forward<ARGS>(args)...));
	}

	enum { mincard=2 };
	enum { maxcard=2 };
};

struct opdiv {
	template<typename ARG>
	constexpr auto apply(ARG &&arg) {
		return std::forward<ARG>(arg);
	}
	template<typename ARG1,typename ARG2>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2) {
		return std::forward<ARG1>(arg1)/std::forward<ARG2>(arg2);
	}
	template<typename ARG1, typename ARG2, typename ARG3, typename... ARGS>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2, ARG3 &&arg3, ARGS &&...args) {
		return apply(std::forward<ARG1>(arg1) / std::forward<ARG2>(arg2),
			apply(std::forward<ARG3>(arg3),std::forward<ARGS>(args)...));
	}

	enum { mincard=2 };
	enum { maxcard=2 };
};

struct oppow {
	template<typename ARG1, typename ARG2>
	constexpr auto apply(ARG1 &&arg1, ARG2 &&arg2) {
		return ::pow(std::forward(arg1),std::forward(arg2));
	}

	enum {mincard=2 };
	enum (maxcard=2 };
};

struct opunaryplus {
	template<typename ARG1>
	constexpr auto apply(ARG1 &&arg1) {
		return +std::forward(arg1);
	}

	enum {mincard=1 };
	enum (maxcard=1 };
};

struct opunaryminus {
	template<typename ARG1>
	constexpr auto apply(ARG1 &&arg1) {
		return -std::forward(arg1);
	}

	enum {mincard=1 };
	enum (maxcard=1 };
};

#endif
