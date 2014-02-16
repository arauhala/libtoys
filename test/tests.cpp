#include "toys/itoy.h"
#include "toys/text.h"
#include "toys/sdl.h"
#include "tester.h"

using namespace toys;
using namespace toys::gen;

class text_image {
private:
	vec sz_;
	char* buf_;
	text_graphics g_;
public:
	text_image(vec sz)
	: sz_(sz.x()+1, sz.y(), 1), buf_(new char[sz_.volume()]), g_(sz_, buf_) {
		for (int i = 0; i < sz_.volume(); ++i) buf_[i] = ' ';
		for (int i = 0; i < sz_.y(); ++i) {
			g_.set(vec(sz_.x()-1,i), '\n');
		}
		g_.set(vec(sz_.x()-1, sz_.y()-1), '\0');
	}
	vec sz() const {
		return vec(sz_.x()-1, sz_.y(), sz_.z());
	}
	text_graphics& g() {
		return g_;
	}
	~text_image() {
		delete[] buf_;
	}
	const char* str() const {
		return buf_;
	}
};

void test_setup( test_tool& t ) {
	text_image i(vec(16, 16));
	t<<i.str()<<"\n";
}

void test_rect( test_tool& t ) {
	text_image i(vec(16, 16));
	rect<text_traits, char> r('x');
	r.draw(i.sz(), i.g());
	t<<i.str()<<"\n";
}

void test_text( test_tool& t ) {
	text_image i(vec(16, 16));
	auto txt = text<text_traits, std::string, nothing, nothing>( "this is text" );
	txt.draw(i.sz(), i.g());
	t<<i.str()<<"\n";
}

void test_at( test_tool& t ) {
	text_image i(vec(16, 16));
	rect<text_traits, char> r('x');
	at<text_traits, vec, itoy<text_traits>*> a(vec(3, 3), &r);
	a.draw(i.sz(), i.g());
	t<<i.str()<<"\n";
}

void test_sz( test_tool& t ) {
	text_image i(vec(16, 16));
	rect<text_traits, char> r('x');
	sz<text_traits, vec, itoy<text_traits>*> a(vec(13, 13), &r);
	a.draw(i.sz(), i.g());
	t<<i.str()<<"\n";
}

void test_split( test_tool& t ) {
	text_image i(vec(16, 16));
	rect<text_traits, char> a('a');
	rect<text_traits, char> b('b');
	rect<text_traits, char> c('c');
	split<text_traits, aval, itoy<text_traits>*, itoy<text_traits>*> h(0, aval(8), &a, &b);
	split<text_traits, aval, itoy<text_traits>*, itoy<text_traits>*> v(1, aval(8), &h, &c);
	v.draw(i.sz(), i.g());
	t<<i.str()<<"\n";
}

// return complex ui component as value
template <typename _traits>
auto complex_toy(const typename _traits::pixel_type* colors,
				 const typename _traits::font_type& font 		= typename _traits::font_type(),
				 const typename _traits::color_type& font_color = typename _traits::color_type()) {
	typedef box<_traits, value_copy> x;
	auto h = x::lr(.5, x::rc(colors[0]), x::rc(colors[1]));
	auto v = x::ud(.5, h, x::rc(colors[2]));
	auto txt = x::tx(std::string("click!"), font, font_color);
	auto mid = x::on_click([](click& c){ system::exit(0); }, x::fb(x::lay(xy(0.5, 0.5), txt), x::rc(colors[3])));
	auto w = x::lay(xy(.5, .5), x::sz(xy(.5, .5), mid));
	return x::fb(w, v);
}

const char text_colors[] = "abc ";

void test_ibox( test_tool& t ) {
	text_image i(vec(16, 16));
	auto z = complex_toy<text_traits>(text_colors);
	z->draw(i.sz(), i.g());
	t<<"the ui component size is "<<sizeof(decltype(z))<<" bytes\n\n";
	t<<i.str()<<"\n";
}

void test_sbox( test_tool& t ) {
	text_image i(vec(16, 16));
	auto z = complex_toy<baseless_text_traits>(text_colors);
	z->draw(i.sz(), i.g());
	t<<"the ui component size is "<<sizeof(decltype(z))<<" bytes\n\n";
	t<<i.str()<<"\n";
}

void test_srect( test_tool& t ) {
	text_image i(vec(16, 16));
	typedef box<baseless_text_traits, value_copy> x;
	auto z = x::rc('a');
	z->draw(i.sz(), i.g());
	t<<"the ui component size is "<<sizeof(decltype(z))<<" bytes\n\n";
	t<<i.str()<<"\n";
}

void test_ssz( test_tool& t ) {
	text_image i(vec(16, 16));
	typedef box<baseless_text_traits, value_copy> x;
	auto z = x::sz(vec(6, 6), x::rc('a'));
	z->draw(i.sz(), i.g());
	t<<"the ui component size is "<<sizeof(decltype(z))<<" bytes\n\n";
	t<<i.str()<<"\n";
}

void test_ssplit( test_tool& t ) {
	text_image i(vec(16, 16));
	typedef box<baseless_text_traits, value_copy> x;
	auto z = x::lr(0.5, x::rc('a'), x::rc('b'));
	z->draw(i.sz(), i.g());
	t<<"the ui component size is "<<sizeof(decltype(z))<<" bytes\n\n";
	t<<i.str()<<"\n";
}

const char* font_path = "/usr/share/fonts/truetype/freefont/FreeMono.ttf";
//const char* font_path = "/usr/share/fonts/truetype/msttcorefonts/Arial.ttf";
//const char* font_path = "/usr/share/fonts/truetype/ubuntu-font-family/UbuntuMono-B.ttf";
int font_size = 20;

void test_sdlbox( test_tool& t ) {
	using namespace sdl;
	sdl::sdltoys infra;
	rgba colors[4] = {
		{255, 0, 0, 255},
		{0, 255, 0, 255},
		{0, 0, 255, 255},
		{255, 255, 255, 255}
	};
	TTF_Font *font = nullptr;
	font = TTF_OpenFont(font_path, font_size);
	auto z = complex_toy<sdl::baseless_traits>(colors, font, {0, 0, 0, 255});
	vec sz(256, 256);
	sdl::window<decltype(z)> wnd("foo", vec(), sz, z);
	sdl::reactor<sdl::window<decltype(z)>*> r(&wnd);
	wnd.draw();
	r.run();
	TTF_CloseFont(font);
}

void test_sdltext( test_tool& t ) {
	using namespace sdl;
	sdl::sdltoys infra;
	typedef box<sdl::baseless_traits, value_copy> x;
	TTF_Font *font = nullptr;
	font = TTF_OpenFont(font_path, font_size);
//	typedef toys::sdl::text<sdl::baseless_traits, std::string, TTF_Font*, rgba> sdltext;
	rgba color = {0, 0, 0, 255};
	auto z = x::on_click([](click& c){ system::exit(0); },
					     x::fb(x::lay(xy(0.5, 0.5),
					    	          x::tx(std::string("foo"), font, color)),
					           x::rc(rgba{255, 255, 255, 255})));
	vec sz(256, 256);
	sdl::window<decltype(z)> wnd("foo", vec(), sz, z);
	sdl::reactor<sdl::window<decltype(z)>*> r(&wnd);
	wnd.draw();
	r.run();
	TTF_CloseFont(font);
}

void add_toy_tests(test_runner& runner) {
	runner.add("toys/setup", std::set<std::string>(), &test_setup);
	runner.add("toys/rect", std::set<std::string>(), &test_rect);
	runner.add("toys/text", std::set<std::string>(), &test_text);
	runner.add("toys/at", std::set<std::string>(), &test_at);
	runner.add("toys/sz", std::set<std::string>(), &test_sz);
	runner.add("toys/split", std::set<std::string>(), &test_split);
	runner.add("toys/ibox", std::set<std::string>(), &test_ibox);
	runner.add("toys/sbox", std::set<std::string>(), &test_sbox);
	runner.add("toys/srect", std::set<std::string>(), &test_srect);
	runner.add("toys/ssz", std::set<std::string>(), &test_ssz);
	runner.add("toys/ssplit",  std::set<std::string>(), &test_ssplit);
	runner.add("toys/sdltext", std::set<std::string>(), &test_sdltext);
	runner.add("toys/sdlbox", std::set<std::string>(), &test_sdlbox);
}

int main(int argc, char** argv) {
	test_runner t("toys tests");
	add_toy_tests(t);
	return t.exec(argc, argv);
}
