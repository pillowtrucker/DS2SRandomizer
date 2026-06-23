# Build system for the DS2S randomizer core.
#
# The GUI is not part of this source drop; `ds2rando_cli` is a small headless
# driver (cli_main.cpp) so the core can be built and run on its own.
#
#   make            # build the ds2rando_cli executable
#   make lib        # build a static library (libds2rando.a) of the core only
#   make clean
#
# Note: param_editor.cpp is intentionally NOT compiled. Its contents are an
# older duplicate of the templates that now live in modules/param_editor.hpp;
# building it would cause redefinition errors.

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -O2 -Wall -I.

CORE_OBJS = randomizer.o itemrando.o
HEADERS   = modules/randomizer.hpp modules/item_rando.hpp modules/param_editor.hpp modules/utils.hpp

all: ds2rando_cli

randomizer.o: randomizer.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

itemrando.o: itemrando.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

cli_main.o: cli_main.cpp modules/randomizer.hpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

ds2rando_cli: cli_main.o $(CORE_OBJS)
	$(CXX) $^ -o $@

lib: $(CORE_OBJS)
	ar rcs libds2rando.a $(CORE_OBJS)

clean:
	rm -f *.o *.a ds2rando_cli

.PHONY: all lib clean
