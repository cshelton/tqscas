#ifndef GENTREE_HPP
#define GENTREE_HPP

#include <memory>
#include <variant>
#include <vector>
#include <utility>
#include <iostream>
#include <optional>

template<typename LT, typename NT, typename TI=void>
class gentree {
	public:
	using ExtraType=TI;

	private:
		typedef LT leafT;
		struct nodeT;
		typedef std::variant<leafT,nodeT> treenodeT;
		typedef std::shared_ptr<treenodeT> treeptr;

		struct nodeT {
			NT node;
			std::vector<gentree> ch;

			template<typename... T>
			nodeT(NT &n, T &&...ch) : node(n) {
				addchildren(std::forward<T>(ch)...);
			}
			template<typename... T>
			nodeT(const NT &n, T &&...ch) : node(n) {
				addchildren(std::forward<T>(ch)...);
			}
			template<typename... T>
			nodeT(NT &&n, T &&...ch) : node(std::move(n)) {
				addchildren(std::forward<T>(ch)...);
			}

			void addchildren(std::vector<gentree> &c) {
				ch = c;
			}
			void addchildren(const std::vector<gentree> &c) {
				ch = c;
			}
			void addchildren(std::vector<gentree> &&c) {
				ch = std::move(c);
			}

			void addchildren(std::vector<treeptr> &c) {
				for(auto &x : c) ch.emplace_back(gentree(x));
			}
			void addchildren(const std::vector<treeptr> &c) {
				for(auto &x : c) ch.emplace_back(gentree(x));
			}
			void addchildren(std::vector<treeptr> &&c) {
				for(auto &x : c) ch.emplace_back(gentree(x));
			}

			void addchildren() {
			}

			template<typename... T>
			void addchildren(gentree &c, T &&...cs) {
				ch.emplace_back(c);
				addchildren(std::forward<T>(cs)...);
			}
			template<typename... T>
			void addchildren(const gentree &c, T &&...cs) {
				ch.emplace_back(c);
				addchildren(std::forward<T>(cs)...);
			}
			template<typename... T>
			void addchildren(gentree &&c, T &&...cs) {
				ch.emplace_back(std::move(c));
				addchildren(std::forward<T>(cs)...);
			}
			template<typename... T>

			void addchildren(treeptr &c, T &&...cs) {
				ch.emplace_back(gentree(c));
				addchildren(std::forward<T>(cs)...);
			}
			template<typename... T>
			void addchildren(const treeptr &c, T &&...cs) {
				ch.emplace_back(gentree(c));
				addchildren(std::forward<T>(cs)...);
			}
			template<typename... T>
			void addchildren(treeptr &&c, T &&...cs) {
				ch.emplace_back(gentree(std::move(c)));
				addchildren(std::forward<T>(cs)...);
			}

		};

		treeptr root;

	private:
		gentree(const treeptr &t) : root(t) {}

	public:
		gentree() : root(nullptr) {}

		gentree(const gentree &t) : root(t.root) {}
		gentree(gentree &&t) : root(std::move(t.root)) {}
		// below needed to prevent "T &&" constructor below
		// from matching -- this isn't ideal but works for the moment
		// (see https://mpark.github.io/programming/2014/06/07/beware-of-perfect-forwarding-constructors/)
		gentree(gentree &t) : root(t.root) {}
		gentree(const gentree &&t) : root(std::move(t.root)) {}

		gentree &operator=(const gentree &t) {
			if (&t!=this) root = t.root;
			return *this;
		}
		gentree &operator=(gentree &&t) {
			root = std::move(t.root);
			return *this;
		}

		// do we need/want this??
		template<typename T>
		explicit gentree(T &&l) : root(std::make_shared<treenodeT>(
					leafT(std::forward<T>(l)))) {
		}

		template<typename... Ts>
		gentree(NT &&n, Ts &&...args)
			: root(std::make_shared<treenodeT>(
					nodeT{std::move(n), std::forward<Ts>(args)...})) {
		}
		template<typename... Ts>
		gentree(const NT &n, Ts &&...args)
			: root(std::make_shared<treenodeT>(
					nodeT{n, std::forward<Ts>(args)...})) {
		}

		template<typename LF, typename NF>
		auto fold(LF leaffn, NF nodefn) const
				-> decltype(leaffn(std::declval<leafT>())) {
			if (isleaf()) return leaffn(std::get<leafT>(*root));
			else {
				typedef decltype(leaffn(std::declval<leafT>())) rT;
				std::vector<rT> chret;
				const nodeT &node = std::get<nodeT>(*root);
				chret.reserve(node.ch.size());
				for(auto &c : node.ch)
					chret.emplace_back(c.fold(leaffn,nodefn));
				return nodefn(node.node,std::move(chret));
			}
		}

		// F should be of type "gentree -> optional<gentree>"
		// (if fn returns no value, then use existing (sub)tree)
		template<typename F>
		gentree map(F fn) const {
			return mapmaybe(fn).value_or(*this);
		}

		/*
		template<typename F>
		std::optional<gentree> mapmaybe(F fn) const {
			auto newtree = fn(*this);
			if (newtree) return newtree;
			if (isleaf()) return {};
			auto &ch = children();
			std::vector<gentree> newch;
			for(int i=0;i<ch.size();i++) {
				auto nc = ch[i].mapmaybe(fn);
				if (nc) {
					if (newch.empty()) {
						newch.reserve(ch.size());
						for(int j=0;j<i;j++)
							newch.emplace_back(ch[j]);
					}
					newch.emplace_back(*nc);
				} else if (!newch.empty()) newch.emplace_back(ch[i]);
			}
			if (newch.empty()) return {};
			return std::optional<gentree>{asnode(),newch};
		}
		*/

		template<typename F>
		std::optional<gentree> mapmaybe(F fn) const {
			if (isleaf()) return fn(*this);
			auto &ch = children();
			std::vector<gentree> newch;
			for(int i=0;i<ch.size();i++) {
				auto nc = ch[i].mapmaybe(fn);
				if (nc) {
					if (newch.empty()) {
						newch.reserve(ch.size());
						for(int j=0;j<i;j++)
							newch.emplace_back(ch[j]);
					}
					newch.emplace_back(*nc);
				} else if (!newch.empty()) newch.emplace_back(ch[i]);
			}
			if (newch.empty()) return fn(*this);
			gentree newtree{asnode(),newch};
			auto newtree2 = fn(newtree);
			if (newtree2) return newtree2;
			return newtree;
		}

		// belong here or need a different fold?
		template<typename LT2, typename NT2, typename TI2>
		bool sameas(const gentree<LT2,NT2,TI2> &t) const {
			if (!(isleaf()==t.isleaf())) return false;
			if (isleaf()) return asleaf()==t.asleaf();
			if (!(asnode()==t.asnode())) return false;
			auto ch = children(), tch = t.children();
			if (ch.size() != tch.size()) return false;
			for(int i=0;i<ch.size();i++)
				if (!(ch[i].sameas(tch[i]))) return false;
			return true;
		}
		template<typename LT2, typename NT2, typename TI2, typename Lcmp, typename Ncmp>
		bool sameas(const gentree<LT2,NT2,TI2> &t,
				Lcmp &&lcmp, Ncmp &&ncmp) const {
			if (!(isleaf()==t.isleaf())) return false;
			if (isleaf()) return lcmp(asleaf(),t.asleaf());
			if (!(ncmp(asnode(),t.asnode()))) return false;
			auto ch = children(), tch = t.children();
			if (ch.size() != tch.size()) return false;
			for(int i=0;i<ch.size();i++)
				if (!(ch[i].sameas(tch[i],lcmp,ncmp))) return false;
			return true;
		}

		/*
		bool operator!=(const gentree &t) const {
			return !(*this==t);
		}
		*/

		bool sameptr(const gentree &t) const {
			return t.root == root;
		}

		bool isleaf() const { return root->index()==0; }

		const LT &asleaf() const { return std::get<leafT>(*root); }
		const NT &asnode() const { return std::get<nodeT>(*root).node; }
		const std::vector<gentree> &children() const {
			const nodeT &n = std::get<nodeT>(*root);
			return n.ch;
		}

		std::size_t ptrhash() const {
			return std::hash<std::shared_ptr<treenodeT>>{}(root);
		}
};
#endif
