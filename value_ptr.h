/* This is taken from Proposal N3339 by Walter E. Brown */
/* dated 2012-01-09 */

// cshelton: 2014
// also modified so that can copy and move and assign even if 
//  "Cloner" and "Deleter" don't match exactly (either they match,
//    or both are defaults)
// allowed creation of unique_ptr from value_ptr (by copying)
//

// ======================================================================
//
// value_ptr: A pointer treating its pointee as-if contained by value
//
// This smart pointer template mimics value semantics for its pointee:
// - the pointee lifetime matches the pointer lifetime, and
// - the pointee is copied whenever the pointer is copied.
//
// Having such a template provides a standard vocabulary to denote such
// pointers, with no need for further comment or other documentation to
// describe the semantics involved.
//
// As a small bonus, this template’s c’tors ensure that all instance
// variables are initialized.
//
// ======================================================================
#ifndef VALUE_PTR_H
#define VALUE_PTR_H

#include <cstddef> // nullptr_t
#include <functional> // less
#include <memory> // default_delete
#include <type_traits> // add_pointer, ...
#include <utility> // move, swap

	namespace internal {
		template<typename T> struct is_cloneable;
		template<typename Element,
				bool = std::is_polymorphic<Element>::value
					&& internal::is_cloneable<Element>::value>
			struct default_action;
		template<typename Element>
			struct default_action<Element, false>;
	} 

	template<typename Element> struct default_copy;
	template<typename Element> struct default_clone;
	template<typename Element,
			typename Cloner = internal::default_action<Element>,
			typename Deleter = std::default_delete<Element>>
		class value_ptr;

	template<typename E, typename C, typename D>
	void swap(value_ptr<E,C,D> &,value_ptr<E,C,D> &) noexcept;

	template<typename E, typename C, typename D>
	bool operator==(value_ptr<E,C,D> const &, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator!=(value_ptr<E,C,D> const &, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator==(value_ptr<E,C,D> const &, std::nullptr_t);

	template<typename E, typename C, typename D>
	bool operator!=(value_ptr<E,C,D> const &, std::nullptr_t);

	template<typename E, typename C, typename D>
	bool operator==(std::nullptr_t, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator!=(std::nullptr_t, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator<(value_ptr<E,C,D> const &, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator>(value_ptr<E,C,D> const &, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator<=(value_ptr<E,C,D> const &, value_ptr<E,C,D> const &);

	template<typename E, typename C, typename D>
	bool operator>=(value_ptr<E,C,D> const &, value_ptr<E,C,D> const &);

	// ======================================================================
	template<typename T>
	struct internal::is_cloneable {
		private:
			typedef char (& yes_t)[1];
			typedef char (& no_t)[2];
			template<typename U, U* (U::*)() const = &U::clone>
			struct cloneable { };

			template<typename U> static yes_t test(cloneable<U>*);
			template<typename> static no_t test(...);
		public:
			static bool const value
					= sizeof(test<T>(0)) == sizeof(yes_t);
	};
	// -----------------------------------------------
	template<typename Element>
	struct default_copy {
	public:
		Element *operator()(Element * p) const { return new Element(*p); }
	};
	// -----------------------------------------------
	template<typename Element>
	struct default_clone {
	public:
		Element *operator()(Element * p) const { return p->clone(); }
	};
	// -----------------------------------------------
	template<typename Element, bool>
	struct internal::default_action : public default_clone<Element> {
	public:
		using default_clone<Element>::operator();
	};
	template<typename Element>
	struct internal::default_action<Element, false>
				: public default_copy<Element> {
	public:
		using default_copy<Element>::operator();
	}; // default_action<,false>
	// -----------------------------------------------
	template<typename Element, typename Cloner, typename Deleter>
	class value_ptr {
	public:
	// -- publish our template parameters and variations thereof:
		typedef Element element_type;
		typedef Cloner cloner_type;
		typedef Deleter deleter_type;
		typedef typename std::add_pointer<Element>::type pointer;
		typedef typename std::add_lvalue_reference<Element>::type reference;
		typedef std::unique_ptr<Element,Deleter> uptr;
	//private:
		template<typename P>
		struct is_compatible
			: public std::is_convertible<typename std::add_pointer<P>::type,
									pointer>
		{ };
		template<typename E2, typename C2, typename D2>
		struct can_assign {
			static bool const value = 
				is_compatible<E2>::value
				&& (std::is_same<Cloner,C2>::value
				    || (std::is_same<Cloner,
							internal::default_action<Element>>::value
					   && std::is_same<C2,
							internal::default_action<E2>>::value
					))
				&& (std::is_same<Deleter,C2>::value
				    || (std::is_same<Deleter,
							std::default_delete<Element>>::value
					   && std::is_same<D2,
							std::default_delete<E2>>::value
					));
		};
	public:
		// default c’tor:
		constexpr value_ptr() noexcept : p(nullptr) { }
		// ownership-taking c’tors:
		constexpr value_ptr(std::nullptr_t) noexcept : p(nullptr) { }

		template<typename E2>
		explicit value_ptr(E2 *other) noexcept : p(other) {
			static_assert(is_compatible<E2>::value,
						"value_ptr<>’s pointee type is incompatible!");
			static_assert(!std::is_polymorphic<E2>::value
				|| !(std::is_same<Cloner,
					internal::default_action<Element,false>
						>::value),
				"value_ptr<>’s pointee type would slice when copying!");
		}
		// copying c’tors:
		value_ptr(value_ptr const &other) : p(clone_from(other.p)) {}

		//template<typename E2>
		//value_ptr(value_ptr<E2,C2,D2> const & other,
		//value_ptr(value_ptr<E2,Cloner,Deleter> const & other,
		//	typename std::enable_if<is_compatible<E2>::value>::type *ig=nullptr)
		//		: p(clone_from(other.p)) {
		//}
		template<typename E2,typename C2,typename D2>
		value_ptr(value_ptr<E2,C2,D2> const &other,
		   typename std::enable_if<can_assign<E2,C2,D2>::value>::type *ig=nullptr)
				: p(clone_from(other.p)) {
		}
		// moving c’tors:
		value_ptr(value_ptr &&other) noexcept : p(other.release()) { }

		//template<typename E2>
		//value_ptr(value_ptr<E2,Cloner,Deleter> &&other,
		//	typename std::enable_if<is_compatible<E2>::value>::type *ig=nullptr)
		template<typename E2,typename C2,typename D2>
		value_ptr(value_ptr<E2,C2,D2> &&other,
		   typename std::enable_if<can_assign<E2,C2,D2>::value>::type *ig=nullptr)
					noexcept : p(other.release()) { }

		// d’tor:
		~value_ptr() noexcept { reset(); }
		// copying assignments:
		value_ptr &operator=(std::nullptr_t) noexcept {
			reset(nullptr); return *this;
		}
		value_ptr &operator=(value_ptr const &other) {
			value_ptr tmp(other);
			swap(tmp); return *this;
		}
		//template<typename E2>
		//typename std::enable_if<is_compatible<E2>::value,value_ptr &>::type
		//		operator=(value_ptr<E2,Cloner,Deleter> const &other)
		template<typename E2,typename C2, typename D2>
		typename std::enable_if<can_assign<E2,C2,D2>::value,value_ptr &>::type
				operator=(value_ptr<E2,C2,D2> const &other) {
			value_ptr tmp(other);
			swap(tmp); return *this;
		}
		// moving assignments:
		value_ptr &operator=(value_ptr &&other) {
			value_ptr tmp(std::move(other));
			swap(tmp); return *this;
		}
		//template<typename E2>
		//typename std::enable_if<is_compatible<E2>::value,value_ptr &>::type
		//	operator= (value_ptr<E2,Cloner,Deleter> && other)
		template<typename E2, typename C2, typename D2>
		typename std::enable_if<can_assign<E2,C2,D2>::value,value_ptr &>::type
			operator= (value_ptr<E2,C2,D2> &&other) {
			value_ptr tmp(std::move(other));
			swap(tmp); return *this;
		}

		// unique_ptr conversion:
		operator uptr() const & noexcept {
			value_ptr tmp(*this);
			return uptr(tmp.release());
		}
		operator uptr() && noexcept {
			return uptr(release());
		}
		value_ptr(uptr other) noexcept : p(other.release()) {
		}
		value_ptr &operator=(uptr other) {
			value_ptr tmp(other.release());
			swap(tmp); return *this;
		}
		// unique_ptr conversion for more general type
		template<typename E2, typename D2,
			typename Rev = typename value_ptr<E2,Cloner,D2>
					::template can_assign<Element,Cloner,Deleter>,
			typename IG = typename std::enable_if<Rev::value>>
		operator std::unique_ptr<E2,D2> () const & noexcept {
			value_ptr tmp(*this);
			return uptr(tmp.release());
		}
		template<typename E2, typename D2,
			typename Rev = typename value_ptr<E2,Cloner,D2>
					::template can_assign<Element,Cloner,Deleter>,
			typename IG = typename std::enable_if<Rev::value>>
		operator std::unique_ptr<E2,D2> () && noexcept {
			return uptr(release());
		}

		template<typename E2, typename D2,
			typename IG
				= typename std::enable_if<can_assign<E2,Cloner,D2>::value>::type>
		value_ptr(std::unique_ptr<E2,D2> other)
				noexcept : p(other.release()) {
		}
		template<typename E2, typename D2,
			typename IG
				= typename std::enable_if<can_assign<E2,Cloner,D2>::value>::type>
		value_ptr &operator=(std::unique_ptr<E2,D2> other) {
			value_ptr tmp(other.release());
			swap(tmp); return *this;
		}

			
		// observers:
		reference operator*() const { return *get(); }
		pointer operator->() const noexcept { return get(); }
		pointer get() const noexcept { return p; }
		explicit operator bool () const noexcept { return get(); }
		// modifiers:
		pointer release() noexcept {
			pointer old = p; p = nullptr;
			return old;
		}
		void reset(pointer t = pointer()) noexcept {
			std::swap(p, t); Deleter()(t);
		}
		void swap(value_ptr & other) noexcept {
			std::swap(p, other.p);
		}

	private:
		pointer p;

		template<typename P>
		pointer clone_from(P const p) const {
			return p ? Cloner()(p) : nullptr;
		}
	}; // value_ptr<>

	// ======================================================================
	// non-member functions:
	// -----------------------------------------------
	// non-member swap:
	template<typename E, typename C, typename D>
	void swap(value_ptr<E,C,D> & x, value_ptr<E,C,D> & y) noexcept {
		x.swap(y);
	}
	// -----------------------------------------------
	// non-member (in)equality comparison:
	template<typename E, typename C, typename D>
	bool operator==(value_ptr<E,C,D> const &x, value_ptr<E,C,D> const &y)
		{ return x.get() == y.get(); }
	template<typename E, typename C, typename D>
	bool operator!=(value_ptr<E,C,D> const &x, value_ptr<E,C,D> const &y)
		{ return !operator==(x,y); }
	template<typename E, typename C, typename D>
	bool operator==(value_ptr<E,C,D> const &x, std::nullptr_t y)
		{ return x.get() == y; }
	template<typename E, typename C, typename D>
	bool operator!=(value_ptr<E,C,D> const &x, std::nullptr_t y)
		{ return !operator==(x,y); }
	template<typename E, typename C, typename D>
	bool operator==(std::nullptr_t x, value_ptr<E,C,D> const &y)
		{ return x == y.get(); }
	template<typename E, typename C, typename D>
	bool operator!=(std::nullptr_t x, value_ptr<E,C,D> const &y)
		{ return !operator==(x,y); }
	// -----------------------------------------------
	// non-member ordering:
	template<typename E, typename C, typename D>
	bool operator<(value_ptr<E,C,D> const &x, value_ptr<E,C,D> const &y) {
		typedef typename std::common_type<typename value_ptr<E,C,D>::pointer,
						typename value_ptr<E,C,D>::pointer>::type CT;
			return std::less<CT>()(x.get(), y.get());
	}
	template<typename E, typename C, typename D>
	bool operator>(value_ptr<E,C,D> const & x, value_ptr<E,C,D> const & y)
		{ return y < x; }
	template<typename E, typename C, typename D>
	bool operator<=(value_ptr<E,C,D> const & x, value_ptr<E,C,D> const & y)
		{ return !(y < x); }
	template<typename E, typename C, typename D>
	bool operator>=(value_ptr<E,C,D> const & x, value_ptr<E,C,D> const & y)
		{ return !(x < y); }

#endif
