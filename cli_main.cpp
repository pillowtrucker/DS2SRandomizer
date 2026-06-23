// Minimal headless entry point for the DS2S randomizer core.
//
// The full GUI is not part of this source drop, so this driver lets the
// param-writing core be built and run from the command line. It reads all
// settings from er_config.txt (created with defaults on first run); enable the
// enemy shuffle there with:
//     #SHUFFLE_ENEMIES 1     (permute existing enemies instead of randomizing)
//     #SHUFFLE_GLOBAL 1      (1 = across the whole game, 0 = within each map)
//
// Run from a directory containing the randomizer's `data/` folder; output is
// written to `Param/`.
//
//     ./ds2rando_cli [seed]

#include "modules/randomizer.hpp"
#include <iostream>
#include <string>

int main(int argc, char** argv){
    randomizer::Data data;
    if(!randomizer::load_data(data)){
        std::cerr << "Failed to load randomizer data (run from a folder containing data/).\n";
        return 1;
    }
    if(argc > 1){
        try{
            data.config.seed = std::stoull(argv[1]);
        }catch(const std::exception&){
            std::cerr << "Invalid seed argument: " << argv[1] << "\n";
            return 1;
        }
    }
    std::cout << "Seed: " << data.config.seed << "\n";
    bool ok = randomizer::randomize(data, false);
    randomizer::free_stuff(data);
    std::cout << (ok ? "Done. Wrote params to Param/.\n" : "Randomization failed.\n");
    return ok ? 0 : 1;
}
