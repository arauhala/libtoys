
#include "module.h"

using namespace toys::sdl::vals;
using namespace toys::sdl;
using namespace toys;
using namespace std;

// following widget does not require separate class definition, but its state gets allocated in heap
unique_ptr<itoy> make_radio_button(const font& f, const function<void (bool)>& changed) {
	shared_ptr<bool> state(new bool(false));
	rgba black{0, 0, 10, 0};
	shared_ptr<itoy> on = shared_itoy(fb(lay(mid, tx("on", f, black)), rc(rgba{150, 250, 150, 0})));
	shared_ptr<itoy> off = shared_itoy(fb(lay(mid, tx("off", f, black)), rc(rgba{250, 150, 150, 0})));
	return
		owned_itoy(
			sz(xy(75, 50),
				on_click([state, changed](click&) {*state = !*state; changed(*state);},
					prop([state, &f, on, off, black] () { // radio button appearance depends of its state
						return *state ? on.get() : off.get();
					}))));
}

// composing a stateful and flat (not split into several memory blocks) widget from stateless
// microwidgets is possible, if a bit clumsy.
auto make_resizing_toy(vec* v) {
	return on_click([v](click& c){*v = (v->x() == 50) ? vec(100, 100) :vec(50,50); },
					sz([v](vec){return *v;}, rc(rgba{255, 255, 255, 0})));
}
class resizing_item : public itoy {
private:
	vec sz_;
	decltype(make_resizing_toy(0)) toy_;
public:
	resizing_item() : sz_(50, 50), toy_(make_resizing_toy(&sz_)) {}
	vec size(const vec& size) const 				   { return toy_->size(size); };
	void draw(const vec& size, graphics_type& i) const { return toy_->draw(size, i); }
	bool recv(const vec& size, ievent& e) 			   { return toy_->recv(size, e); }
};
unique_ptr<itoy> make_resizing_item() {
	return unique_ptr<itoy>(new resizing_item());
}
