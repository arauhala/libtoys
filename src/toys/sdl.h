/*
 * sdltoy.h
 *
 *  Created on: Oct 28, 2013
 *      Author: arau
 */

#ifndef SDLTOY_H_
#define SDLTOY_H_

#ifdef __emcc__
#include "SDL/SDL.h"
#include "SDL/SDL_ttf.h"
#else
#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"
#ifdef __clang__
char* gets(char*);
#endif
#endif


#include "toys/toys.h"
#include <stdint.h>


#include <string>

namespace toys {
	namespace sdl {

		class sdltoys {
		public:
			sdltoys() {
				if (SDL_Init(SDL_INIT_EVERYTHING) == -1) {
					throw std::runtime_error("setting up SDL failed");
				}
				if (TTF_Init() == -1){
					throw std::runtime_error("setting up TFF failed");
				}
			}
			template <typename _toyr>
			int run(_toyr r);
			~sdltoys() {
				TTF_Quit();
				SDL_Quit();
			}
		};

		class font {
		private:
			std::shared_ptr<TTF_Font> font_;
		public:
			font(const char* path, int size)
			: font_(TTF_OpenFont(path, size), TTF_CloseFont){}
			operator TTF_Font* () const {
				return font_.get();
			}
			~font() {}
		};

		class graphics {
		public:
			typedef translation_sentry<graphics> translation_sentry_type;
		private:
			SDL_Renderer* ren_;
			SDL_Surface* sur_;
			vec tr_;
		public:
			graphics(SDL_Renderer* ren, SDL_Surface* sur) : ren_(ren), sur_(sur), tr_() {
				SDL_RenderClear(ren_);
			}
			~graphics() {
				SDL_RenderPresent(ren_);
			}
			SDL_Renderer* renderer() { return ren_; }
			inline void translate(const vec& t) {
				tr_ += t;
			}
			inline translation_sentry_type translation(const vec& p) {
				return translation_sentry_type(*this, p);
			}
			vec tr() const {
				return tr_;
			}
			inline SDL_Surface* surface() const {
				return sur_;
			}
		};

		template <typename _recvr>
		class reactor {
		private:
			_recvr toy_;
		public:
			reactor(const _recvr& t) : toy_(t) {}
			int run() {
				system::init();
				while (!system::exiting()) {
					SDL_Event event;
					SDL_WaitEvent(&event);
					if (event.type == SDL_MOUSEBUTTONDOWN) {
						click e(xy(event.button.x, event.button.y));
						toy_->recv(vec(), e);
					}
				}
				return system::exit_value();
			}
		};

		typedef SDL_Color rgba;

		struct traits {
			public:
				typedef gen::itoy<traits> base_type;
				typedef rgba 		 pixel_type;
				typedef graphics 	 graphics_type;
				typedef TTF_Font*    font_type;
				typedef rgba 		 color_type;
		};

		typedef traits::base_type itoy;

		struct baseless_traits {
			public:
				typedef gen::stoy    base_type;
				typedef rgba 		 pixel_type;
				typedef graphics 	 graphics_type;
				typedef TTF_Font*    font_type;
				typedef rgba 		 color_type;
		};

		template <typename _tref>
		std::unique_ptr<itoy> owned_itoy(_tref t) {
			return std::unique_ptr<itoy>(new gen::wrap<traits, _tref>(t));
		};

		template <typename _tref>
		std::shared_ptr<itoy> shared_itoy(_tref t) {
			return std::shared_ptr<itoy>(new gen::wrap<traits, _tref>(t));
		};

		template <typename _toyr>
		class window {
		private:
			SDL_Window* win_;
			SDL_Renderer* ren_;
			SDL_Surface* sur_;
			_toyr toy_;

		public:
			window(const char* name, vec pos, vec size, const _toyr& toy)
		   : win_(), ren_(), sur_(), toy_(toy) {
				win_ = SDL_CreateWindow(name, pos.x(), pos.y(), size.x(), size.y(), SDL_WINDOW_SHOWN);
				if (!win_) {
					throw std::runtime_error("creating SDL window failed");
				}
				ren_ = SDL_CreateRenderer( win_, -1, SDL_RENDERER_ACCELERATED
											 	   | SDL_RENDERER_PRESENTVSYNC );
				if (!ren_) {
					SDL_DestroyWindow(win_);
					throw std::runtime_error("creating SDL renderer failed");
				}
				sur_ = SDL_GetWindowSurface(win_);
				if (!sur_) {
					SDL_DestroyWindow(win_);
					SDL_DestroyRenderer(ren_);
					throw std::runtime_error("getting window surface failed");
				}
			}
			vec size() const {
				int w, h;
				SDL_GetWindowSize( win_, &w, &h );
				return vec(w, h);
			};
			void draw() const {
				vec sz = size();
				auto g = graphics();
				toy_->draw(sz, g);
			}
			bool recv(const vec& , ievent& e) {
				bool rv = toy_->recv(size(), e);
				draw();
				return rv;
			}
			::toys::sdl::graphics graphics() const {
				return ::toys::sdl::graphics(ren_, sur_);
			}
			~window() {
				SDL_DestroyRenderer(ren_);
				SDL_DestroyWindow(win_);
			}
		};

		inline const char* utf8_ptr(const char* s) {
			return s;
		}
		inline const char* utf8_ptr(const std::string& s) {
			return s.c_str();
		}

		template <typename _traits, typename _text, typename _font, typename _color>
		class text : public _traits::base_type {
			private:
				_text txt_;
				_font font_;
				_color color_;
			public:
				text(const _text& txt, const _font& font, const _color& color)
				: txt_(txt), font_(font), color_(color) {}
				vec size(const vec& ) const {
					int w, h;
					TTF_SizeUTF8(font_, utf8_ptr(txt_), &w, &h);
					return vec(w, h);
				}
				void draw(const vec& , graphics& g) const {
					SDL_Surface* s = TTF_RenderUTF8_Blended(font_,utf8_ptr(txt_), color_);
					SDL_Texture* t = SDL_CreateTextureFromSurface(g.renderer(), s);
					SDL_Rect r = {g.tr().x(), g.tr().y(), s->w, s->h };
					SDL_RenderCopy(g.renderer(), t, 0, &r);
					SDL_DestroyTexture(t);
					SDL_FreeSurface(s);
				}
		};
		template <typename _toyr>
		int sdltoys::run(_toyr t) {
			sdl::reactor<_toyr> r(t);
			t->draw();
			return r.run();
		}

		namespace vals {
			//
		    // shortcut functions to toys with value semantics in order to avoid following:
		    //
		    // typedef toys::box<baseless_traits, value_copy> x;
		    // auto toy = x::lr(x::foo(...), x::bar(...))
		    //
		    // the code is MUCH more readable and better to write by skipping the typedef and x:: prefixes
		    //
		    // this is an ugly hack and definately a maintainability problem, but what you can do.
		    //
		    // the syntax has to be nice & readable and there is no 'using static toys::vals::box::*' mechanism
		    // for including all static members of a (template instance) class or ability to define
		    // generic namespaces (template <X> namespace foo { } ), using namespace foo<X>;
		    //

			typedef toys::box<baseless_traits, value_copy> box;

			template <typename _t> auto ref(const _t& t) 				{ return box::ref(t); }
			template <typename _tr, typename _toy_ref>
			inline auto sz(const _tr& tr, const _toy_ref& ref) 			{ return box::sz( tr, ref); }
			template <typename _lambda, typename _toy_ref>
			inline auto on_click(const _lambda& l, const _toy_ref& ref) { return box::on_click( l, ref); };
			template <typename _xy, typename _toy_ref>
			inline auto at(const _xy& xy, const _toy_ref& ref) 			{ return box::at( xy, ref); }
			template <typename _xy, typename _toy_ref>
			inline auto lay(const _xy& xy, const _toy_ref& ref) 		{ return box::lay( xy, ref ); }
			template <typename _pixel>
			inline auto rc(_pixel pixel) 								{ return box::rc(pixel); };
			template <typename _text, typename _font, typename _color>
			inline auto tx(const _text& text, const _font& font, const _color& color) { return box::tx(text, font, color); }
			template <typename _sp, typename _f_ref, typename _s_ref>
			inline auto lr(_sp sp, const _f_ref& f, const _s_ref& s)    { return box::lr(sp, f, s); };
			template <typename _sp, typename _f_ref, typename _s_ref>
			static auto ud(_sp sp, const _f_ref& f, const _s_ref& s)    { return box::ud(sp, f, s); }
			template <typename _f_ref, typename _s_ref>
			auto fb(const _f_ref& f, const _s_ref& b) 					{ return box::fb(f, b); }
		}
	}
	namespace gen { // necessary specializations

		template <typename _text, typename _font, typename _color>
		class text<sdl::traits, _text, _font, _color>
		: public sdl::text<sdl::traits, _text, _font, _color> {
		public:
			using sdl::text<sdl::traits, _text, _font, _color>::text;
		};

		template <typename _text, typename _font, typename _color>
		class text<sdl::baseless_traits, _text, _font, _color>
		: public sdl::text<sdl::baseless_traits, _text, _font, _color> {
		public:
			using sdl::text<sdl::baseless_traits, _text, _font, _color>::text;
		};


		template <>
		void fill_rect<sdl::graphics, sdl::rgba>(sdl::graphics& g, const vec& size, sdl::rgba c);
	}

}

#endif /* SDLTOY_H_ */
