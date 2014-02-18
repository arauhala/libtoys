/*
 * sdl.cpp
 *
 *  Created on: Feb 17, 2014
 *      Author: arau
 */

#include "toys/sdl.h"

namespace toys {
	namespace gen {

		template <>
		void fill_rect<sdl::graphics, sdl::rgba>(sdl::graphics& g, const vec& size, sdl::rgba c) {
			SDL_Rect r = { g.tr().x(), g.tr().y(), size.x(), size.y() };
			SDL_SetRenderDrawColor( g.renderer(), c.r, c.g, c.b, 255 );
			SDL_RenderFillRect( g.renderer(), &r );
		}

	}

}
