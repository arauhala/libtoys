/*
 * itoy.h
 *
 *  Created on: Sep 14, 2013
 *      Author: arau
 */

#ifndef TOYS_ITOY_H
#define TOYS_ITOY_H

#include <memory>
#include <stdexcept>

namespace toys {

	struct vec { // xyz ?
		private:
			int x_;
			int y_;
			int z_;
		public:
			vec() : x_(0), y_(0), z_(0) {}
			vec(int x, int y) : x_(x), y_(y), z_(0) {}
			vec(int x, int y, int z) : x_(x), y_(y), z_(z) {}
			inline int x() const {
				return x_;
			}
			inline int y() const {
				return y_;
			}
			inline int z() const {
				return z_;
			}
			inline int volume() const {
				return x_*y_*z_;
			}
			bool contains(const vec& p) const {
				return p.x_ >= 0 && p.y_ >= 0 && p.z_ >= 0
				   && p.x_ < x_ && p.y_ < y_ && p.z_ < z_;
			}
			vec operator+(const vec& s) const {
				return vec(x_+s.x_, y_+s.y_, z_+s.z_);
			}
			vec operator() (const vec& p) const {
				return *this;
			}
			int operator[] (int i) const {
				switch (i) {
				case 0: return x_;
				case 1: return y_;
				case 2: return z_;
				default:
					throw std::runtime_error("");
				}
			}
			int& operator[] (int i) {
				switch (i) {
				case 0: return x_;
				case 1: return y_;
				case 2: return z_;
				default:
					throw std::runtime_error("");
				}
			}
			vec& operator+=( const vec& s) {
				x_+=s.x_;
				y_+=s.y_;
				z_+=s.z_;
				return *this;
			}
			vec& operator-=( const vec& s) {
				x_-=s.x_;
				y_-=s.y_;
				z_-=s.z_;
				return *this;
			}
	};
	// translation
	struct itr {
		virtual vec operator() (const vec& xy) const = 0;
	};
	inline vec operator-(const vec& f, const vec& s) {
		return vec(f.x()-s.x(), f.y()-s.y(), f.z()-s.z());
	}
	inline vec operator-(const vec& p) {
		return vec(-p.x(), -p.y(), -p.z());
	}
	template <typename _x, typename _y>
	class tr {
	private:
		_x x_;
		_y y_;
	public:
		tr(const _x& x, const _y& y) : x_(x), y_(y) {}
		vec operator() (const vec& xy) const {
			return vec(x_(xy.x()), y_(xy.y()), xy.z());
		}

	};

	// absolute split
	struct aval {
		int s_;
		aval(int s) : s_(s) {}
		int operator() (int v) const {
			return s_;
		}
	};
	// relative
	struct rval {
		float s_;
		rval(float s) : s_(s) {}
		int operator() (int v) const {
			return v*s_;
		}
	};

	const tr<rval, rval> mid(0.5, 0.5);

	inline auto xy(int x, int y) {
		return vec(x, y);
	}
	inline auto xy(int x, double y) {
		return tr<aval, rval>(x, y);
	}
	inline auto xy(double x, int y) {
		return tr<rval, aval>(x, y);
	}
	inline auto xy(double x, double y) {
		return tr<rval, rval>(x, y);
	}

	template <typename _t>
	struct translation_sentry {
	private:
		_t& g_;
		vec t_;
	public:
		translation_sentry(_t& g, const vec& t) : g_(g), t_(t) {
			g.translate(t);
		}
		~translation_sentry() {
			g_.translate(-t_);
		}
	};

	class ievent {
		public:
			typedef translation_sentry<ievent> translation_sentry_type;
			virtual ~ievent() {}
			virtual void translate(const vec& xy) = 0;
			virtual bool hit(const vec& xy) const { return true; }
			translation_sentry_type translation(const vec& p) {
				return translation_sentry_type(*this, p);
			};
	};


	class pointevent : public ievent {
	private:
		vec xy_;
	public:
		pointevent(const vec& xy) : xy_(xy) {}
		void translate(const vec& xy) {
			xy_ -= xy;
		}
		bool hit(const vec& r) const {
			return xy_.x() >= 0 && xy_.y() >= 0
				&& r.x()-xy_.x() >= 0 && r.y()-xy_.y() >= 0;
		}
	};

	class click : public pointevent {
		public:
			click(const vec& xy) : pointevent(xy) {}
	};

	class nothing {};

	namespace gen {
		// let's put the generics here.
		// they are not meant to be used by user directly, so they
	    // can be hidden in separate namespace

		// the abstract toy interface. this is mainly mean to be used
	    // with heap allocation & reference semantics. this abstract
	    // interface is meant to be used with bigger UI widgets and not with
	    // the micro toys (rc, split, lay, sz, ...) as if you create e.g. 20 separate
	    // microtoys in heap to form single widget; the heap overhead will be significant.
		template <typename _traits>
		class itoy {
			public:
				typedef typename _traits::graphics_type graphics_type;
				~itoy(){}
				virtual vec size(const vec& size) const { return size; };
				virtual void draw(const vec& size, graphics_type& i) const = 0;
				virtual bool recv(const vec& size, ievent& e) { return false; }
		};

		// base class for static toys that is mean to be used within templates
		// and with copy/value semantics. the static toys are meant to collect
		// the microscopic toys (rc, split, lay, sz, ...) into more macroscopic widgets,
		// that can be encapsulated inside itoy interface.
		struct stoy {
			vec size(const vec& size) const { return size; };
			bool recv(const vec& size, ievent& e) { return false; }
		};

		template <typename _graphics, typename _pixel>
		void fill_rect(_graphics& g, const vec& size, _pixel p);

		template <typename _traits, typename _pixel>
		class rect : public _traits::base_type {
			public:
				typedef typename _traits::graphics_type graphics_type;
				typedef _pixel pixel_type;
			private:
				pixel_type p_;
			public:
				rect(pixel_type p)
				: p_(p) {}
				void draw(const vec& size, graphics_type& g) const {
					fill_rect<graphics_type, typename _traits::pixel_type>(g, size, p_);
				}
				bool recv(const vec& size, ievent& e) { return e.hit(size); }
		};

		template <typename _traits, typename _tr, typename _toy_ref>
		class at : public _traits::base_type {
			public:
				typedef typename _traits::graphics_type graphics_type;
			private:
				_tr tr_;
				_toy_ref toy_;
			public:
				at(const _tr& tr, const _toy_ref& toy)
				: tr_(tr), toy_(toy) {}
				vec size(const vec& size) const {
					vec at = tr_(size);
					vec tsz = toy_->size(size - at);
					return tsz + at;
				}
				void draw(const vec& size, graphics_type& g) const {
					vec at = tr_(size);
					auto ts = g.translation(at);
					toy_->draw(size - at, g);
				}
				bool recv(const vec& size, ievent& e) {
					vec at = tr_(size);
					auto ts = e.translation(at);
					return toy_->recv(size - at, e);
				}
		};

		template <typename _traits, typename _text, typename _font, typename _color>
		class text;

		template <typename _traits, typename _tr, typename _toy_ref>
		class sz : public _traits::base_type {
				typedef typename _traits::graphics_type graphics_type;
			private:
				_tr tr_;
				_toy_ref toy_;
			public:
				sz(const _tr& tr, const _toy_ref& toy)
				: tr_(tr), toy_(toy) {}
				vec size(const vec& size) const {
					return tr_(size);
				}
				void draw(const vec& size, graphics_type& g) const {
					toy_->draw(tr_(size), g);
				}
				bool recv(const vec& size, ievent& e) {
					return toy_->recv(tr_(size), e);
				}
		};

		template <typename _traits, typename _tr, typename _toy_ref>
		class lay : public itoy<_traits> {
			public:
				typedef typename _traits::graphics_type graphics_type;
			private:
				_tr      tr_;
				_toy_ref toy_;
			public:
				lay(const _tr& tr, const _toy_ref& toy) : tr_(tr), toy_(toy) {}
				vec size(const vec& size) const {
					return size;
				}
				void draw(const vec& size, graphics_type& g) const {
					vec sz = toy_->size(size);
					vec at = tr_(size-sz);
					auto ts = g.translation(at);
					toy_->draw(size, g);
				}
				bool recv(const vec& size, ievent& e) {
					vec sz = toy_->size(size);
					vec at = tr_(size-sz);
					auto ts = e.translation(at);
					return toy_->recv(size, e);
				}
		};

		template <typename _traits, typename _event, typename _lambda, typename _toy_ref>
		class on : public _traits::base_type {
			public:
				typedef typename _traits::graphics_type graphics_type;
			private:
				_lambda f_;
				_toy_ref toy_;
			public:
				on(const _lambda& f, const _toy_ref& t)
				: f_(f), toy_(t) {}
				vec size(const vec& size) const {
					return toy_->size(size);
				}
				void draw(const vec& size, graphics_type& g) const {
					toy_->draw(size, g);
				}
				bool recv(const vec& size, ievent& e) {
					_event* ce = dynamic_cast<_event*>(&e);
					if (ce && ce->hit(size)) {
						f_(*ce);
						return true;
					} else {
						return toy_->recv(size, e);
					}
				}
		};



		template <typename _traits, typename _tr, typename _f_ref, typename _s_ref = _f_ref>
		class split : public _traits::base_type {
			public:
				typedef typename _traits::graphics_type graphics_type;
			private:
				size_t dim_;
				_tr split_;
				_f_ref first_;
				_s_ref second_;
			public:
				split(size_t d, _tr tr, const _f_ref& first, const _s_ref& second)
				: dim_(d), split_(tr), first_(first), second_(second) {}
				void lay(const vec& sz, vec& fsz, vec& spos, vec& ssz) const {
					vec z(sz);
					z[dim_] = split_(sz[dim_]);
					fsz = first_->size(z);
					spos[dim_] = z[dim_];
					z[dim_] = sz[dim_] - z[dim_];
					ssz = second_->size(z);
				}
				void draw(const vec& sz, graphics_type& g) const {
					vec fsz, spos, ssz;
					lay(sz, fsz, spos, ssz);
					{
						auto ts = g.translation(spos);
						second_->draw(ssz, g);
					}
					first_->draw(fsz, g);
				}
				bool recv(const vec& sz, ievent& e) {
					bool rv = false;
					vec fsz, spos, ssz;
					lay(sz, fsz, spos, ssz);
					if (!(rv = first_->recv(fsz, e))) {
						auto ts = e.translation(spos);
						rv = second_->recv(ssz, e);
					}
					return rv;
				}
		};

		template <typename _l, typename _t>
		class property {
		public:
			_l l_;
			property(_l l) : l_(l) {}
			operator _t () const {
				return l_();
			}
		};

	}

	// value semantics
	template <typename T>
	struct val {
		public:
			typedef T value_type;
		private:
			T v_;
		public:
			val(const T& v) : v_(v) {}
			val(const val<T>& v) : v_(v.v_) {}
			val(const val<T>&& v) : v_(std::move(v.v_)) {}
			const T* operator->() const {
				return &v_;
			}
			T* operator->() {
				return &v_;
			}
			const T& operator*() const {
				return v_;
			}
			T& operator*() {
				return v_;
			}
			const T* get() const {
				return v_;
			}
			T* get() {
				return v_;
			}
	};


	template <typename T>
	val<T> to_val(const T& t) {
		return val<T>(t);
	}

	template <typename _l>
	auto prop(_l l) {
		return gen::property<_l, decltype(l())>(l);
	}



	struct value_copy {
		template <typename T>
		static val<T> ref(const T& t) {
			return to_val<T>(t);
		}
	};

	class system {
	private:
		static int exit_value_;
		static bool exiting_;
	public:
		static void init() {
			exit_value_ = 0;
			exiting_ = false;
		}
		static bool exiting() {
			return exiting_;
		}
		static int exit_value() {
			return exit_value_;
		}
		static void exit(int exit_value) {
			exit_value_ = exit_value;
			exiting_ = true;
		}
	};

	template <typename _traits, typename _mem>
	class box {
		public:
			template <typename T>
			static auto ref(const T& t) {
				return _mem::ref(t);
			}
			template <typename _tr, typename _toy_ref>
			static auto sz(const _tr& tr, const _toy_ref& ref) {
				return _mem::ref(gen::sz<_traits, _tr, _toy_ref>(tr, ref));
			}
			template <typename _lambda, typename _toy_ref>
			static auto on_click(const _lambda& l, const _toy_ref& ref) {
				return _mem::ref(gen::on<_traits, click, _lambda, _toy_ref>(l, ref));
			}
			template <typename _xy, typename _toy_ref>
			static auto at(_xy xy, const _toy_ref& ref) {
				return _mem::ref(gen::at<_traits, _xy, _toy_ref>(xy, ref));
			}
			template <typename _xy, typename _toy_ref>
			static auto lay(_xy xy, const _toy_ref& ref) {
				return _mem::ref(gen::lay<_traits, _xy, _toy_ref>(xy, ref));
			}
			template <typename _pixel>
			static auto rc(_pixel pixel) {
				return ref(gen::rect<_traits, _pixel>(pixel));
			}
			template <typename _text, typename _font, typename _color>
			static auto tx(_text text, const _font& font, _color color) {
				return ref(gen::text<_traits, _text, _font, _color>(text, font, color));
			}
			template <typename _font, typename _color>
			static auto tx(const char* text, const _font& font, _color color) {
				return ref(gen::text<_traits, std::string, _font, _color>(text, font, color));
			}
			template <typename _f_ref, typename _s_ref>
			static auto lr(int sp, const _f_ref& f, const _s_ref& s) {
				return ref(gen::split<_traits, aval, _f_ref, _s_ref>(0, aval(sp), f, s));
			}
			template <typename _f_ref, typename _s_ref>
			static auto lr(double sp, const _f_ref& f, const _s_ref& s) {
				return ref(gen::split<_traits, rval, _f_ref, _s_ref>(0, rval(sp), f, s));
			}
			template <typename _f_ref, typename _s_ref>
			static auto ud(int sp, const _f_ref& f, const _s_ref& s) {
				return ref(gen::split<_traits, aval, _f_ref, _s_ref>(1, aval(sp), f, s));
			}
			template <typename _f_ref, typename _s_ref>
			static auto ud(double sp, const _f_ref& f, const _s_ref& s) {
				return ref(gen::split<_traits, rval, _f_ref, _s_ref>(1, rval(sp), f, s));
			}
			template <typename _f_ref, typename _s_ref>
			static auto fb(int sp, const _f_ref& f, const _s_ref& s) {
				return ref(gen::split<_traits, aval, _f_ref, _s_ref>(2, aval(sp), f, s));
			}
			template <typename _f_ref, typename _s_ref>
			static auto fb(const _f_ref& f, const _s_ref& s) {
				return ref(gen::split<_traits, rval, _f_ref, _s_ref>(2, rval(0.5), f, s));
			}

	};

}

#endif
