/*
 * module.h
 *
 *  Created on: Feb 17, 2014
 *      Author: arau
 */

#ifndef MODULE_H_
#define MODULE_H_

#include "toys/sdl.h"
#include <functional>

std::unique_ptr<toys::sdl::itoy> make_radio_button(const toys::sdl::font& f, const std::function<void (bool)>& changed);
std::unique_ptr<toys::sdl::itoy> make_resizing_item();

#endif /* MODULE_H_ */
