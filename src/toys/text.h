/*
 * text.h
 *
 *  Created on: Nov 6, 2013
 *      Author: arau
 */

#ifndef TEXT_H_
#define TEXT_H_

#include "toys/toys.h"

namespace toys {

	struct text_graphics {
	public:
		typedef translation_sentry<text_graphics> translation_sentry_type;
	private:
		vec sz_;
		char* buf_;
		vec tr_;
	public:
		inline text_graphics(const vec& sz, char* buf)
		: sz_(sz), buf_(buf), tr_() {}
		inline void set(vec p, char c) {
			p += tr_;
			if (sz_.contains(p)) {
				buf_[p.x() + sz_.x()*p.y()] = c;
			}
		}
		inline void translate(const vec& t) {
			tr_ += t;
		}
		inline translation_sentry_type translation(const vec& p) {
			return translation_sentry_type(*this, p);
		}
		inline char get(vec p) const {
			char rv = 0;
			p += tr_;
			if (sz_.contains(p)) {
				rv = buf_[p.x() + sz_.x()*p.y()];
			}
			return rv;
		}
	};
	struct text_traits {
		public:
			typedef gen::itoy<text_traits> base_type;
			typedef char pixel_type;
			typedef text_graphics graphics_type;
			typedef nothing font_type;
			typedef nothing color_type;
	};

	struct baseless_text_traits {
		public:
			typedef gen::stoy base_type;
			typedef char pixel_type;
			typedef text_graphics graphics_type;
			typedef nothing font_type;
			typedef nothing color_type;
	};

	namespace gen {

		template <>
		void fill_rect(text_graphics& g, const vec& size, char c) {
			for (int y = 0; y < size.y(); ++y) {
				for (int x = 0; x < size.x(); ++x) {
					g.set(vec(x, y), c);
				}
			}
		}

		template <typename _traits, typename _text>
		class text_text : public _traits::base_type {
			private:
				_text txt_;
			public:
				text_text(const _text& txt, const nothing& font = nothing(), const nothing& color = nothing())
				: txt_(txt) {}
				vec size(const vec& ) const {
					return vec(txt_.end() - txt_.begin(), 1);
				}
				void draw(const vec& , text_graphics& g) const {
					int i = 0;
					for (auto c : txt_) {
						g.set(vec(i, 0), c);
						i++;
					}
				}
		};

		template <typename _text>
		class text<text_traits, _text, nothing, nothing> : public text_text<text_traits, _text> {
		public:
			using text_text<text_traits, _text>::text_text;
		};


		template <typename _text>
		class text<baseless_text_traits, _text, nothing, nothing> : public text_text<baseless_text_traits, _text> {
		public:
			using text_text<baseless_text_traits, _text>::text_text;
		};

	}

}



#endif /* TEXT_H_ */
