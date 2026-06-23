#ifndef MY_ITEMRANDO
#define MY_ITEMRANDO

#include <stdint.h>
namespace item_rando{
    struct ItemRandoConfig{
        uint64_t seed{0u};
        uint64_t weight_limit{70u};
        bool unlock_common_shop{false};
        bool unlock_straid_trades{false};
        bool unlock_ornifex_trades{false};
        bool infinite_shop_slots{true};
        bool infuse_weapons{false};
        bool randomize_classes{true};
        bool randomize_gifts{true};
        bool allow_twohanding{true};
        bool allow_unusable{false}; 
        bool allow_shield_weapon{false};
        bool allow_catalysts{true};
        bool allow_bows{true};
        bool full_rando_classes{true};
        bool early_blacksmith{true};
        bool write_cheatsheet{false};
        bool melentia_lifegems{false};
        bool randomize_key_items{true};
        bool valid{false};
    };
    struct ItemRandoData;
    struct IRData{
        ItemRandoConfig config;
        ItemRandoData* data;
    };
    bool load_randomizer_data(IRData& irdata);
    bool randomize_items(IRData& irdata,bool devmode);
    void write_config_file(ItemRandoConfig& config);
    void free_rando_data(IRData& irdata);
    bool restore_default_params();
    bool add_item_shop(int32_t item_id,bool devmode);
}

#endif