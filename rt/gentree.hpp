#ifndef GENTREE_HPP
#define GENTREE_HPP

#include <memory>
// to switch to std::variant when we get GCC 7
#include <boost/variant.hpp>
#include <vector>
#include <utility>
#include <iostream>


template<typename LT, typename NT>
class gentree {
	private:
		typedef LT leafT;
		struct nodeT;
		typedef boost::variant<leafT,nodeT> treenodeT;
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
		gentree(gentree &t) : root(t.root) {}
		gentree(gentree &&t) : root(std::move(t.root)) {}

		gentree &operator=(const gentree &t) {
			if (&t!=this) root = t.root;
			return *this;
		}
		gentree &operator=(gentree &&t) {
			root = std::move(t.root);
			return *this;
		}

		template<typename T>
		gentree(T &&l) : root(std::make_shared<treenodeT>(
					std::forward<T>(l))) {
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
			if (isleaf()) return leaffn(boost::get<leafT>(*root));
			else {
				typedef decltype(leaffn(std::declval<leafT>())) rT;
				std::vector<rT> chret;
				const nodeT &node = boost::get<nodeT>(*root);
				chret.reserve(node.ch.size());
				for(auto &c : node.ch)
					chret.emplace_back(c.fold(leaffn,nodefn));
				return nodefn(node.node,std::move(chret));
			}
		}

		// belong here or need a different fold?
		bool operator==(const gentree &t) const {
			if (!(isleaf()==t.isleaf())) return false;
			if (isleaf()) 
				return boost::get<leafT>(*root)==boost::get<leafT>(*t.root);
			const nodeT &n = boost::get<nodeT>(*root);
			const nodeT &tn = boost::get<nodeT>(*t.root);
			if (!(asnode() == t.asnode())) return false;
			auto ch = children(), tch = t.children();
			if (ch.size() != tch.size()) return false;
			for(int i=0;i<ch.size();i++)
				if (!(ch[i]==tch[i])) return false;
			return true;
		}

		bool sametree(const gentree &t) const {
			return t.root == root;
		}

		bool isleaf() const { return root->which()==0; }

		const LT &asleaf() const { return boost::get<leafT>(*root); }
		const NT &asnode() const { return boost::get<nodeT>(*root).node; }
		const std::vector<gentree> &children() const {
			const nodeT &n = boost::get<nodeT>(*root);
			return n.ch;
		}
};
#endif
