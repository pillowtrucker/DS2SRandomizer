#ifndef MY_ENEMYRANDO
#define MY_ENEMYRANDO

#include <stdint.h>
#include <vector>
#include <string>

namespace randomizer{
    struct MapSetting{
        std::string map_name;
        int map_id;
        int enemy_limit;
        bool randomize;
    };
    struct EnemyIdName{
        std::string name;
        int32_t id;
    };
    struct BossBalance{
        bool easy_congregation;
        bool belfry_rush;
        bool easy_skelly;
        bool easy_twins;
        bool remaster_rat;
    };
    struct Multiboss{
        bool on;
        bool skeleton_lords;
        bool belfry_gargoyles;
        bool ruin_sentinels;
        bool twin_dragonrider;
        bool throne_watchers;
        bool graverobbers;
        bool lud_and_zallen;
    };
    struct Config{
        std::vector<MapSetting> map_settings;
        std::vector<size_t> banned_enemies;
        uint64_t seed;
        uint32_t enemy_shuffling;
        bool shuffle_enemies;//permute existing enemies (preserves population) instead of randomizing
        bool shuffle_global;//when shuffling: true=across the whole game, false=within each map
        BossBalance boss_balance;
        Multiboss multiboss;
        int  roaming_boss_chance;
        int  enemy_hp_scaling;
        int  enemy_dmg_scaling;
        int  boss_hp_scaling;
        int  boss_dmg_scaling;
        int  rats_clones;
        bool respawn_roaming_boss;
        bool replace_invaders;
        bool remove_invaders;
        bool replace_summons;
        bool remove_summons;
        bool replace_npcs;
        bool randomize_bosses;
        bool randomize_enemies;
        bool randomize_mimics;
        bool randomize_lizards;
        bool remove_invis;
        bool roaming_boss;
        bool enemy_scaling;
        bool scale_bosses;
        bool write_cheatsheet;
        bool valid;
    };
    //Forward declarations
    struct MapData;
    using GameData=std::vector<MapData>;
    struct EnemyTable;
    struct Data{
        GameData* game_data;
        EnemyTable* enemy_table;
        Config config;
    };
    std::vector<EnemyIdName> get_enemytable(Data& data);
    std::vector<EnemyIdName> get_bosstable(Data& data);
    void restore_zone_limit_defaults(Config& config);
    void write_configfile(Config& config);
    bool load_data(Data& data);
    bool randomize(Data& data,bool devmode);
    bool restore_default_params(bool devmode);
    void free_stuff(Data& data);
};

#endif