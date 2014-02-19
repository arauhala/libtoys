_cxx      = ccache clang
_cxxflags = -Wall -fmessage-length=0 -Isrc -std=c++1y -stdlib=libc++ -Os

.phony : all

# NOTE: I could not get the ASM.js builds to work
#       (also: em++ generated 3 MB json for simplest possible SDL app??!)
#
#asm.js: _cxx = em++
#asm.js: _test_target = libtoystest.js
#asm.js: _example_target = toysexample.js
#asm.js: _cxxflags+= -O2 -D__emcc__
#asm.js: $(_target) $(_example_target)

_lflags = -std=c++1y 

_target    = libtoys
_srcs      = $(shell find src -name *.cpp)
_headers   = $(shell find src -name *.h)
_objs      = $(patsubst %.cpp,%.o,$(_srcs))
_deps      = $(patsubst %.o,%.d,$(_objs))

_test_target = libtoystest
_test_srcs   = $(shell find test -name *.cpp)
_test_objs   = $(patsubst %.cpp,%.o,$(_test_srcs))
_test_deps   = $(patsubst %.o,%.d,$(_test_objs))

_example_target = toysexample
_example_srcs   = $(shell find example -name *.cpp)
_example_objs   = $(patsubst %.cpp,%.o,$(_example_srcs))
_example_deps   = $(patsubst %.o,%.d,$(_example_objs))

_libs = -lSDL2 -lSDL2_ttf -lc++

all:  info $(_target) $(_test_target) $(_example_target)

$(_objs): %.o: %.cpp
	$(_cxx) -c -MMD -MP $(_cxxflags) $< -o $@
	@sed -i -e '1s,\($*\)\.o[ :]*,\1.o $*.d: ,' $*.d

$(_test_objs): %.o: %.cpp
	$(_cxx) -c -MMD -MP $(_cxxflags) -Itest $< -o $@
	@sed -i -e '1s,\($*\)\.o[ :]*,\1.o $*.d: ,' $*.d

$(_example_objs): %.o: %.cpp
	$(_cxx) -c -MMD -MP $(_cxxflags) -Iexample $< -o $@
	@sed -i -e '1s,\($*\)\.o[ :]*,\1.o $*.d: ,' $*.d


-include $(_deps) $(_test_deps) $(_example_deps)

$(_target):	$(_objs)
	$(AR) rcs $(_target) $(_objs)

clean:
	rm -f $(_objs) $(_target) $(_test_objs) $(_target_deps) $(_test_deps) $(_test_target) $(_example_target) $(_example_deps) $(_example_objs)

$(_test_target) : $(_test_objs) $(_target)
	$(_cxx) $(_lflags) -o $(_test_target) $(_test_objs) $(_target) $(_libs)

$(_example_target) : $(_example_objs) $(_target)
	$(_cxx) $(_lflags) -o $(_example_target) $(_example_objs) $(_target) $(_libs)
    

info: 
	@echo sources:
	@echo $(_srcs)
	@echo deps:
	@echo $(_deps)
	@echo headers:
	@echo $(_headers)
	@echo test sources:
	@echo $(_test_srcs)
