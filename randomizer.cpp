#include "modules/randomizer.hpp"
#include "modules/param_editor.hpp"
#include "modules/utils.hpp"
namespace randomizer{

namespace paths{
    const std::filesystem::path configfile = "er_config.txt";
    const std::filesystem::path enemy_types ="data/enemy_rando/map_enemy_types";
    const std::filesystem::path enemies ="data/enemy_rando/enemies";
    const std::filesystem::path params ="data/enemy_rando/Params";
    const std::filesystem::path out_folder ="Param";
    const std::filesystem::path cheatsheet_folder = "cheatsheets/enemies";
}

namespace common{
    std::vector<std::tuple<u64,std::string,std::string,s32,float>> map_names{
        {10020000,"m10_02_00_00","Things Betwixt",200,2.1f},
        {10040000,"m10_04_00_00","Majula",300,2.0f},
        {10100000,"m10_10_00_00","Forest of Fallen Giants",250,2.0f},
        {10140000,"m10_14_00_00","Brightstone Cove Tseldora",800,1.5f},
        {10150000,"m10_15_00_00","Aldia's Keep",1800,1.4f},
        {10160000,"m10_16_00_00","The Lost Bastille & Belfy Luna",600,1.7f},
        {10170000,"m10_17_00_00","Harvest Valley & Earthen Peak",600,1.7f},
        {10180000,"m10_18_00_00","No-man's Wharf",400,1.8f},
        {10190000,"m10_19_00_00","Iron Keep & Belfry Sol",800,1.5f},
        {10230000,"m10_23_00_00","Huntsman's Copse & Undead Purgatory",400,1.8f},
        {10250000,"m10_25_00_00","The Gutter & Black Gulch",750,1.6f},
        {10270000,"m10_27_00_00","Dragon Aerie & Shrine",1700,1.4f},
        {10290000,"m10_29_00_00","Majula <=> Shaded Woods",350,1.9f},
        {10300000,"m10_30_00_00","Heide's Tower <=> No-man's Wharf",400,1.8f},
        {10310000,"m10_31_00_00","Heide's Tower of Flame",350,1.8f},
        {10320000,"m10_32_00_00","Shaded Woods & Shrine of Winter",550,1.7f},
        {10330000,"m10_33_00_00","Doors of Pharros",600,1.7f},
        {10340000,"m10_34_00_00","Grave of Saints",600,1.7f},
        {20100000,"m20_10_00_00","Giant memories",1700,1.4f},
        {20110000,"m20_11_00_00","Shrine of Amana",1300,1.4f},
        {20210000,"m20_21_00_00","Drangleic Castle & Throne",1100,1.4f},
        {20240000,"m20_24_00_00","Undead Crypt",1500,1.4f},
        {20260000,"m20_26_00_00","Dragon Memories",9999,1.4f},
        {40030000,"m40_03_00_00","Dark Chasm of Old",9999,1.4f},
        {50350000,"m50_35_00_00","Shulva, Sanctum City",2000,1.4f},
        {50360000,"m50_36_00_00","Brume Tower",2000,1.4f},
        {50370000,"m50_37_00_00","Frozen Eleum Loyce",2000,1.4f},
        {50380000,"m50_38_00_00","Memory of the King",9999,1.4f}
    };

    std::string boss_log;
};


enum class EntityType:uint8_t{
    UNKNOWN,
    NPC,
    STONE_NPC,
    SUMMON,
    INVADER,
    BOSS,
    ELITE,
    ENEMY,
    MIMIC,
    LIZARD,
    HOLLOW,
    PHANTOM,
    STONE
};

struct EntityInfo{
    EntityType type;
    uint8_t draw_group;
    uint8_t display_group;
};
struct EnemyInstance{
    u32 ai_think;
    Register regist;
};
struct EnemyVariation{
    s32 id;
    std::string name;
    std::vector<EnemyInstance> instances;
};
struct EnemyType{
    std::string name;
    std::vector<EnemyVariation> variations;
    s32 id;
    s32 diff;
    s32 size;
    s32 souls_held;
};
struct BossArena{
    u64 map_id;
    std::vector<u64> ids;
    std::string name;
    s32 size;
    s32 hp_target;
    float dmg_target;
    s32 def_target;
};
struct EnemyRepositioning{
    s32 enemy_row;
    float position[3];
};
struct EnemyTable{
    std::vector<EnemyType> enemies;
    std::vector<EnemyType> bosses;
    std::vector<EnemyType> invaders;
    std::vector<BossArena> boss_arenas;
    std::vector<EnemyInstance> npcs;
    EnemyInstance straid;
    std::vector<float> ngp_dmg_scaling;
    std::unordered_multimap<u64,EnemyRepositioning> reposition;
    ParamFile<EnemyParam> enemy_params;
};
struct MapData{
    u64 id;
    std::string code;
    std::string name;
    s32 enemy_scaling;
    float dmg_scaling;
    ParamFile<Generator> generator;
    ParamFile<Register>  regist;
    ParamFile<Location>  location;
    std::vector<EntityInfo> entity_info;
};




void load_map_names(GameData& map_data){
    for(const auto& entry:common::map_names){
        map_data.push_back({});
        map_data.back().id   = std::get<0>(entry);
        map_data.back().code = std::get<1>(entry);
        map_data.back().name = std::get<2>(entry);
        map_data.back().enemy_scaling = std::get<3>(entry);
        map_data.back().dmg_scaling = std::get<4>(entry);
    }
}

std::vector<EnemyInstance> get_enemy_instances(s32 enemy_id,const GameData& map_data){
    std::vector<EnemyInstance> possible_data;
    for(const auto& map:map_data){
        const auto& regist = map.regist;
        for(size_t i = 0;i<regist.data.size();i++){
            const auto& entry=regist.data[i];
            if(entry.enemy_id!=enemy_id) continue;
            else if(entry.enemy_id==308000&&regist.row_info[i].row==30800001)continue;//Stone Lion Clan Warrior
            else if(entry.enemy_id==300002&&regist.row_info[i].row==30000050)continue;//Stone Ogre in Aldia's
            else if(entry.enemy_id/10==15000&&entry.logic_id==115020)continue;//Stone Undead Traveler
            auto row = regist.row_info[i].row;
            for(const auto& gen:map.generator.data){
                if(gen.generator_regist_param!=row) continue;
                auto ai = gen.ai_think_id;
                bool repeated=false;
                for(const auto& d:possible_data){
                    bool same = true;
                    if(entry.default_logic_id!=d.regist.default_logic_id){
                        same=false;
                    }else if(entry.logic_id!=d.regist.logic_id){
                        same=false;
                    }else if(entry.spawn_state!=d.regist.spawn_state){
                        same=false;
                    }else if(ai!=d.ai_think){
                        same=false;
                    }
                    if(same){
                        repeated=true;
                    }
                }
                if(!repeated){
                    possible_data.push_back({gen.ai_think_id,entry});
                }
            }
        }
    }
    return possible_data;
}

size_t get_map(GameData& map_data,const std::string& id){
    for(size_t i = 0;i<map_data.size();i++){
        if(map_data[i].code==id) return i;
    }
    return SIZE_MAX;
}
size_t get_map(GameData& map_data,u64 id){
    for(size_t i = 0;i<map_data.size();i++){
        if(map_data[i].id==id) return i;
    }
    return SIZE_MAX;
}

bool load_entity_types(const std::filesystem::path& folder_path,GameData& map_data){
    for(const auto& entry:std::filesystem::directory_iterator(folder_path)){
        auto path = entry.path();
        std::ifstream file(path);
        if(!file){
            std::cout<<"Failed to load entity types, can't open file: "<<path<<"\n";
            return false;
        }
        std::string map_id = path.stem().string();
        auto map_index = get_map(map_data,map_id);
        if(map_index==SIZE_MAX){
            std::cout<<"Failed to load entity types, can't match map: "<<map_id<<"\n";
            return false; 
        }
        auto& entity_type = map_data[map_index].entity_info;
        entity_type.resize(10000,{EntityType::UNKNOWN,0u});
        std::string line;
        while(std::getline(file,line)){
            if(line.empty())continue;
            if(line.substr(0,2)=="//")continue;
            auto id = std::stoi(line.substr(0,line.find(' ')));
            auto type = line.substr(line.find('[')+1,line.find(']')-line.find('[')-1);
            if(type=="NPC"){
                entity_type[id].type=EntityType::NPC;
            }else if(type=="STONE_NPC"){
                entity_type[id].type=EntityType::STONE_NPC;
            }else if(type=="SUMMON"){
                entity_type[id].type=EntityType::SUMMON;
            }else if(type=="INVADER"){
                entity_type[id].type=EntityType::INVADER;
            }else if(type=="BOSS"){
                entity_type[id].type=EntityType::BOSS;
            }else if(type=="ELITE"){
                entity_type[id].type=EntityType::ELITE;
            }else if(type=="ENEMY"){
                entity_type[id].type=EntityType::ENEMY;
            }else if(type=="MIMIC"){
                entity_type[id].type=EntityType::MIMIC;
            }else if(type=="LIZARD"){
                entity_type[id].type=EntityType::LIZARD;
            }else if(type=="HOLLOW"){
                entity_type[id].type=EntityType::HOLLOW;
            }else if(type=="STONE"){
                entity_type[id].type=EntityType::STONE;
            }else if(type=="PHANTOM"){
                entity_type[id].type=EntityType::PHANTOM;
            }else{
                entity_type[id].type=EntityType::ENEMY;
            }
        }
    }
    return true;
}

Register find_regist(const MapData& map,u64 regist_id){
    for(size_t i = 0;i<map.regist.data.size();i++){
        if(map.regist.row_info[i].row==regist_id){
            return map.regist.data[i];
        }
    }
    Register invalid_regist;
    invalid_regist.enemy_id=-1;
    return invalid_regist;
}
const Register* find_regist_ptr(const MapData& map,u64 regist_id){
    for(size_t i = 0;i<map.regist.data.size();i++){
        if(map.regist.row_info[i].row==regist_id){
            return &map.regist.data[i];
        }
    }
    return nullptr;
}

void find_original_draw_groups(GameData& map_data){
    for(auto& map:map_data){
        for(size_t i = 0;i<map.generator.data.size();i++){
            auto& row_info = map.generator.row_info[i];
            auto& data     = map.generator.data[i];
            auto regist_id =data.generator_regist_param;
            Register regist = find_regist(map,regist_id);
            if(regist.enemy_id!=-1){
                map.entity_info[row_info.row].draw_group=regist.draw_group;
                map.entity_info[row_info.row].display_group=regist.display_group;
            }else{
                std::cout<<"Failed to find original draw_group\n";
                std::cout<<"Map: "<<map.name<<" entity: "<<row_info.row<<'\n';
            }
        }
    }
}
void find_missing_entity_entries(GameData& map_data){
    for(const auto& map:map_data){
        auto& generator = map.generator;
        std::cout<<map.name<<'\n';
        for(size_t j = 0;j<generator.data.size();j++){
            auto& row_info = generator.row_info[j];
            // auto& data     = generator.data[j];
            if(map.entity_info[row_info.row].type==EntityType::UNKNOWN){
                std::cout<<row_info.row<<",";
            }
        }
        std::cout<<"\n";
    }
}


bool load_generator_files(const std::filesystem::path& folder_path,GameData& map_data){
    std::cout<<"Loading generator params\n";
    for(const auto& entry:std::filesystem::directory_iterator(folder_path)){
        auto path = entry.path();
        if(path.extension()!=".param") continue;
        std::ifstream file(path,std::ifstream::binary);
        if(!file){
            std::cout<<"Failed to load generators, can't open file:"<<path<<"\n";
            return false;
        }
        std::string map_id = path.stem().string();
        map_id = map_id.substr(map_id.find("_m")+1);
        auto map_index = get_map(map_data,map_id);
        if(map_index==SIZE_MAX){
            std::cout<<"Failed to load generators, can't match map: "<<map_id<<"\n";
            return false; 
        }
        // std::cout<<"Reading: "<<map_names[map_id]<<" generator file\n";
        std::stringstream ss;
        ss<<file.rdbuf();
        map_data[map_index].generator = read_param_file<Generator>(ss.str());
    }
    return true;
}
bool load_regist_files(const std::filesystem::path& folder_path,GameData& map_data){
    std::cout<<"Loading regist params\n";
    for(const auto& entry:std::filesystem::directory_iterator(folder_path)){
        auto path = entry.path();
        if(path.extension()!=".param") continue;
        std::ifstream file(path,std::ifstream::binary);
        if(!file){
            std::cout<<"Failed to load regists, can't open file:"<<path<<"\n";
            return false;
        }
        std::string map_id = path.stem().string();
        map_id = map_id.substr(map_id.find("_m")+1);
        auto map_index = get_map(map_data,map_id);
        if(map_index==SIZE_MAX){
            std::cout<<"Failed to load regists, can't match map: "<<map_id<<"\n";
            return false; 
        }
        // std::cout<<"Reading: "<<map_names[map_id]<<" generator file\n";
        std::stringstream ss;
        ss<<file.rdbuf();
        map_data[map_index].regist = read_param_file<Register>(ss.str());
    }
    return true;
}
bool load_location_files(const std::filesystem::path& folder_path,GameData& map_data){
    std::cout<<"Loading location params\n";
    for(const auto& entry:std::filesystem::directory_iterator(folder_path)){
        auto path = entry.path();
        if(path.extension()!=".param") continue;
        std::ifstream file(path,std::ifstream::binary);
        if(!file){
            std::cout<<"Failed to load locations, can't open file:"<<path<<"\n";
            return false;
        }
        std::string map_id = path.stem().string();
        map_id = map_id.substr(map_id.find("_m")+1);
        auto map_index = get_map(map_data,map_id);
        if(map_index==SIZE_MAX){
            std::cout<<"Failed to load locations, can't match map: "<<map_id<<"\n";
            return false; 
        }
        // std::cout<<"Reading: "<<map_names[map_id]<<" generator file\n";
        std::stringstream ss;
        ss<<file.rdbuf();
        map_data[map_index].location = read_param_file<Location>(ss.str());
    }
    return true;
}



//Finds the enemy param entry for the character_id, returns a copy or an empty row with id -1 in failure
EnemyParam find_enemy_param(const ParamFile<EnemyParam>& enemy_params,u64 enemy_id){
    for(size_t i=0;i<enemy_params.data.size();i++){
        if(enemy_params.row_info[i].row==enemy_id){
            return enemy_params.data[i];
        }
    }
    std::cout<<"Enemy param not found, id: "<<enemy_id<<"\n";
    EnemyParam ep;
    ep.id=-1;
    return ep;
}
EnemyParam* find_enemy_param_ptr(ParamFile<EnemyParam>& enemy_params,u64 enemy_id){
    for(size_t i=0;i<enemy_params.data.size();i++){
        if(enemy_params.row_info[i].row==enemy_id){
            return &enemy_params.data[i];
        }
    }
    std::cout<<"Enemy param not found, id: "<<enemy_id<<"\n";
    return nullptr;
}


void test_enemy_regiters_duplicates(const std::vector<ParamFile<Register>>& regists,const std::unordered_map<s32,std::string>& valid_enemies,std::unordered_map<s32,std::string>& valid_bosses){
    std::vector<uint8_t> counts(1000000,0u);
    for(const auto& regist:regists){
        for(size_t i = 0;i<regist.data.size();i++){
            const auto& entry=regist.data[i];
            counts[entry.enemy_id]+=1;
        }
    }
    for(const auto& enemy:valid_enemies){
        int amm = counts[enemy.first];
        if(amm >1){
            std::cout<<"Multiple registers: "<<enemy.first<<" "<<enemy.second<<" "<<amm<<'\n';
        }else if(amm ==0){
            std::cout<<"No registers: "<<enemy.first<<" "<<enemy.second<<" "<<amm<<'\n';
        }else{
            //std::cout<<"Perfect registers: "<<enemy.first<<" "<<enemy.second<<" "<<amm<<'\n';
        }
    }
    for(const auto& enemy:valid_bosses){
        int amm = counts[enemy.first];
        if(amm >1){
            std::cout<<"Multiple registers: "<<enemy.first<<" "<<enemy.second<<" "<<amm<<'\n';
        }else if(amm ==0){
            std::cout<<"No registers: "<<enemy.first<<" "<<enemy.second<<" "<<amm<<'\n';
        }else{
            //std::cout<<"Perfect registers: "<<enemy.first<<" "<<enemy.second<<" "<<amm<<'\n';
        }
    }
}
void test_npc_itemlots(const GameData& map_data){
    //In order for npc swap to work its necessary to change thier item lot
    //Otherwise they wont drop anything given that is controlled by their character
    //id, not their generator
    //This function checked is used to prove my theory that no NPC 
    //uses a item lot for drops in their generators
    for(const auto& map:map_data){
        for(size_t i =0;i<map.generator.data.size();i++){
            auto row = map.generator.row_info[i].row;
            auto& data = map.generator.data[i];
            if(map.entity_info[row].type==EntityType::NPC){
                if(data.item_lot_id[0]!=0||data.item_lot_id[1]!=0){
                    std::cout<<map.name<<" "<<row<<" "<<data.item_lot_id[0]<<" "<<data.item_lot_id[1]<<'\n';
                }
            }
        }
    }
}
void test_map_model_limit(const GameData& map_data){
    std::vector<s32> models;
    for(const auto& map:map_data){
        models.clear();
        const auto& generator = map.generator;
        const auto& regist = map.regist;
        for(size_t i = 0;i<generator.data.size();i++){
            auto regists_row = generator.data[i].generator_regist_param;
            for(size_t j = 0;j<regist.data.size();j++){
                if(regist.row_info[j].row!=regists_row) continue;
                auto model = regist.data[j].enemy_id/100;
                if(!vector_contains(models,model)) models.push_back(model);
                break;
            }
        }
        std::cout<<"Map: "<<map.name<<" model count: "<<models.size()<<'\n';
    }
}
void test_register_display_groups(const GameData& map_data){
    size_t count = 0;
    for(const auto& map:map_data){
        const auto& regist = map.regist;
        for(size_t i = 0;i<regist.data.size();i++){
            if(regist.data[i].display_group!=0){
                std::cout<<"Map: "<<map.name<<" regist: "<<regist.row_info[i].row<<'\n';
                count+=1;
            }
        }
        std::cout<<"Map: "<<map.name<<" display count: "<<count<<'\n';
        count=0;
    }
}
void test_location_generator_parity(const GameData& map_data){
    for(const auto& map:map_data){
        std::cout<<map.name<<" "<<map.location.data.size()<<" "<<map.generator.data.size()<<'\n';
        std::cout<<map.name<<" "<<map.location.row_info.size()<<" "<<map.generator.row_info.size()<<'\n';
    }  
}

bool load_enemy_table(EnemyTable& enemy_table,GameData& map_data){
    std::cout<<"Loading enemy table\n";
    const std::filesystem::path enemies_filepath{paths::enemies/"enemies.txt"};
    const std::filesystem::path bosses_filepath {paths::enemies/"bosses.txt"};
    const std::filesystem::path enemy_prop_path {paths::enemies/"enemy_properties.txt"};
    const std::filesystem::path boss_prop_path  {paths::enemies/"boss_properties.txt"};
    const std::filesystem::path boss_arena_path {paths::enemies/"boss_arena.txt"};
    const std::filesystem::path reposition_path {paths::enemies/"repositioning.txt"};

    std::ifstream enemy_prop_file(enemy_prop_path);
    if(!enemy_prop_file){
        std::cout<<"Failed to load enemy table , can't open file:"<<enemy_prop_path<<"\n";
        return false;
    }
    std::string line;
    std::vector<size_t> enemy_id_to_index(10000,SIZE_MAX);
    while(std::getline(enemy_prop_file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;
        auto columns = parse::split(line,';');//data is ;-delimited: id;name;diff;size
        if(columns.size()<4) continue;
        EnemyType enemy_type;
        parse::read_var(columns[0],enemy_type.id);
        enemy_type.name=parse::trim(columns[1]);
        parse::read_var(columns[2],enemy_type.diff);
        parse::read_var(columns[3],enemy_type.size);
        enemy_id_to_index[enemy_type.id]=enemy_table.enemies.size();
        enemy_table.enemies.push_back(std::move(enemy_type));
    }
    enemy_prop_file.close();
    
    std::ifstream enemy_file(enemies_filepath);
    if(!enemy_file){
        std::cout<<"Failed to load enemy table , can't open file:"<<enemies_filepath<<"\n";
        return false;
    }
    while(std::getline(enemy_file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;
        EnemyVariation variation;
        variation.id = std::stoi(line);//leading number is the id
        variation.name = parse::trim(line.substr(line.rfind(';')+1));//name is the last ;-field
        s32 enemy_id = variation.id/100;
        auto index = enemy_id_to_index[enemy_id];
        if(index==SIZE_MAX){
            std::cout<<"Can't find correct id for enemy: "<<variation.id<<" "<<variation.name<<'\n';
        }else{
            variation.instances = get_enemy_instances(variation.id,map_data);
            if(enemy_id==1500){//Deal with stone knights blocking the way
                for(auto& a:variation.instances){
                    a.regist.spawn_state=1;
                }
            }
            if(variation.instances.empty()){
                std::cout<<"No data found for: "<<variation.id<<" "<<variation.name<<'\n';
            }
            enemy_table.enemies[index].variations.push_back(std::move(variation));
        }
    }
    enemy_file.close();

    for(size_t i = 0;i<enemy_table.enemies.size();i++){
        auto& enemy = enemy_table.enemies[i];
        if(enemy.variations.empty()){
            std::cout<<"No enemy variations for :"<<enemy.id<<" "<<enemy.name<<'\n';
            enemy_table.enemies.erase(enemy_table.enemies.begin()+i);
            i-=1;
        }
    }

    std::ifstream boss_prop_file(boss_prop_path);
    if(!boss_prop_file){
        std::cout<<"Failed to load enemy table , can't open file:"<<boss_prop_path<<"\n";
        return false;
    }
    std::fill(enemy_id_to_index.begin(),enemy_id_to_index.end(),SIZE_MAX);
    while(std::getline(boss_prop_file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;
        auto columns = parse::split(line,';');//data is ;-delimited: id;diff;size;souls;name
        if(columns.size()<5) continue;
        EnemyType enemy_type;
        parse::read_var(columns[0],enemy_type.id);
        parse::read_var(columns[1],enemy_type.diff);
        parse::read_var(columns[2],enemy_type.size);
        parse::read_var(columns[3],enemy_type.souls_held);
        enemy_type.name=parse::trim(columns[4]);
        enemy_id_to_index[enemy_type.id]=enemy_table.bosses.size();
        enemy_table.bosses.push_back(std::move(enemy_type));
    }
    boss_prop_file.close();

    std::ifstream bosses_file(bosses_filepath);
    if(!bosses_file){
        std::cout<<"Failed to load enemy table , can't open file:"<<bosses_filepath<<"\n";
        return false;
    }
    while(std::getline(bosses_file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;
        EnemyVariation variation;
        variation.id = std::stoi(line);//leading number is the id
        variation.name = parse::trim(line.substr(line.rfind(';')+1));//name is the last ;-field
        s32 enemy_id = variation.id/100;
        auto index = enemy_id_to_index[enemy_id];
        if(index==SIZE_MAX){
            std::cout<<"Can't find correct id for enemy: "<<variation.id<<" "<<variation.name<<'\n';
        }else{
            variation.instances = get_enemy_instances(variation.id,map_data);
            if(variation.instances.empty()){
                std::cout<<"No data found for: "<<variation.id<<" "<<variation.name<<'\n';
            }else if(variation.instances.size()>1){
                std::cout<<"Multiple registers found: "<<variation.name<<" "<<variation.instances.size()<<"\n";
                for(const auto v:variation.instances){
                    std::cout<<v.regist.enemy_id<<" "<<v.regist.logic_id<<" "<<v.regist.default_logic_id<<" "<<v.regist.spawn_state<<" "<<(int)v.regist.display_group<<" "<<(int)v.regist.draw_group<<" "<<v.ai_think<<'\n';
                }
                
                if(variation.id==333000)variation.instances.pop_back();//Bad Velstatd instance
                if(variation.id==324000)variation.instances.pop_back();//Bad Belfry Gargoyle instance
    
            }
            if(enemy_id==6191){//Executioner chariot
                for(auto& instance:variation.instances){
                    instance.regist.logic_id=619120;
                }
            }else if(enemy_id==2120){//Guardian dragon
                for(auto& instance:variation.instances){
                    instance.regist.logic_id=212020;
                }
            }else if(enemy_id==3240){//Belfry Gargoyle
                //Swap its ai with the one from Aldias so it reacts immediatly
                for(auto& instance:variation.instances){
                    instance.regist.logic_id=324020;
                    instance.regist.spawn_state=2;
                }
            }else if(enemy_id==3250){//Ruin Sentinel
                //Swap ai with the one from Drangleic Castle
                for(auto& instance:variation.instances){
                    instance.regist.logic_id=325020;
                }
            }else if(enemy_id==6115){//Dragonrider
                for(auto& instance:variation.instances){
                    instance.regist.logic_id=611020;
                    instance.regist.spawn_state=0;
                }
            }else if(enemy_id==6900){//Ivory King 
                //Prevent him from walking out of bounds
                for(auto& instance:variation.instances){
                    instance.regist.spawn_state=1;
                }
            }
            enemy_table.bosses[index].variations.push_back(std::move(variation));
        }
    }
    bosses_file.close();

    for(size_t i = 0;i<enemy_table.bosses.size();i++){
        auto& boss = enemy_table.bosses[i];
        if(boss.variations.empty()){
            std::cout<<"No enemy variations for :"<<boss.id<<" "<<boss.name<<'\n';
            enemy_table.bosses.erase(enemy_table.bosses.begin()+i);
            i-=1;
        }
    }


    std::ifstream arena_file(boss_arena_path);
    if(!arena_file){
        std::cout<<"Failed to load enemy table , can't open file:"<<boss_arena_path<<"\n";
        return false;
    }
    while(std::getline(arena_file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;
        //data is ;-delimited: arena_id;boss#;map_id;gen_id;size;hp;dmg;def;wormhole_id;elana_spawn_id;loc_s;loc_e;name
        auto columns = parse::split(line,';');
        if(columns.size()<13){
            std::cout<<"Failed to parse line:"<<line<<" from:"<<boss_arena_path<<"\n";
            continue;
        }
        BossArena arena;
        parse::read_var(columns[2],arena.map_id);
        //New format has one generator id per row; multi-boss arenas span several rows
        arena.ids.push_back((u64)-1);
        parse::read_var(columns[3],arena.ids.back());
        arena.name=parse::trim(columns[12]);
        parse::read_var(columns[4],arena.size);
        parse::read_var(columns[5],arena.hp_target);
        parse::read_var(columns[6],arena.dmg_target);
        parse::read_var(columns[7],arena.def_target);
        enemy_table.boss_arenas.push_back(std::move(arena));
    }
    arena_file.close();

    std::ifstream reposition_file(reposition_path);
    if(!reposition_file){
        std::cout<<"Failed to load enemy table , can't open file:"<<reposition_path<<"\n";
        return false;
    }
    while(std::getline(reposition_file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;
        //data is ;-delimited: map_id;gen_id;x;y;z
        auto tokens = parse::split(line,';');
        if(tokens.size()<5){
            std::cout<<"Bad row in reposition file: Wrong number of tokens. "<<line<<'\n';
            continue;
        }
        EnemyRepositioning repo;
        u64 map_id=0;
        parse::read_var(tokens[0],map_id);
        parse::read_var(tokens[1],repo.enemy_row);
        parse::read_var(tokens[2],repo.position[0]);
        parse::read_var(tokens[3],repo.position[1]);
        parse::read_var(tokens[4],repo.position[2]);
        enemy_table.reposition.insert(std::make_pair(map_id,repo));
    }
    reposition_file.close();

    {
        std::vector<std::pair<u64,u64>> npcs=
        {
            {10040000,70050007},//Hemerald Herald Majula
            {10040000,75400000},//Melentia Majula
            {10040000,76100002},//Mauhglin Majula
            {10040000,70450000},//Gilligan Majula
            {10040000,76400002},//Lenigrast Majula
            {10040000,76600000},//Carillion Majula
            {10040000,76300000},//Rosabeth Majula
            {10040000,74100000},//Blue covenant guy Majula      
            {10040000,76200000},//Chloanne
            {10020000,72300000},//Milibeth
            {10320000,72500000},//Darkdiver Grandahl
            {10320000,77600000},//Ornifex
            {10160000,76430002},//McDuff
            {10160000,75200000}//Lucatiel
            //{50370000,65800000},//Alsanna -> Disables nearby bonfires?
        };

        for(const auto& [map_id,npc_id]:npcs){
            EnemyInstance npc;
            auto map_index = get_map(map_data,map_id);
            if(map_index!=SIZE_MAX){
                npc.regist = find_regist(map_data[map_index],npc_id);
                if(npc.regist.enemy_id!=-1) enemy_table.npcs.push_back(npc);
            }
        }

    }
    for(const auto& x:map_data){
        if(x.code!="m10_16_00_00")continue;
        for(size_t i = 0;i<x.regist.data.size();i++){
            if(x.regist.row_info[i].row==76800002){
                enemy_table.straid.regist = x.regist.data[i];
                break;
            }
        }
    }
    const std::filesystem::path enemies_file{paths::params/"EnemyParam.param"};
    enemy_table.enemy_params = read_param_file<EnemyParam>(get_file_contents_binary(enemies_file));
    if(enemy_table.enemy_params.header.start_of_data==0) return false;

    auto scaling = read_param_file<ChrRoundDamageParam>(get_file_contents_binary(paths::params/"ChrRoundDamageParam.param"));
    if(scaling.header.start_of_data==0)return false;
    enemy_table.ngp_dmg_scaling.reserve(scaling.row_info.size());
    for(const auto& s:scaling.data){
        enemy_table.ngp_dmg_scaling.push_back(s.out[1].dmg);
    }
    return true;
}

void delete_unused_registers(GameData& map_data){
    for(auto& map:map_data){
        auto& generator = map.generator;
        auto& regist = map.regist;
        size_t deleted_count=0;
        for(size_t j = 0;j<regist.data.size();j++){
            auto row = regist.row_info[j].row;
            bool in_use = false;
            for(const auto& a:generator.data){
                if(a.generator_regist_param==row){
                    in_use=true;
                    break;
                }
            }
            if(!in_use){
                delete_entry(j,map.regist);
                j-=1;
                deleted_count+=1;
            }
        }
        //std::cout<<"Map: "<<map.name<<" deleted regists: "<<deleted_count<<'\n';
    }
}

EnemyType find_enemy_by_id(EnemyTable& enemy_table,s32 character_id){
    for(const auto& e:enemy_table.enemies){
        if(e.id==character_id) return e;
    }
    for(const auto& e:enemy_table.bosses){
        if(e.id==character_id) return e;
    }
    EnemyType et;
    et.id=-1;
    return et;
}
EnemyType* find_enemy_ptr_by_id(EnemyTable& enemy_table,s32 character_id){
    for(auto& e:enemy_table.enemies){
        if(e.id==character_id) return &e;
    }
    for(auto& e:enemy_table.bosses){
        if(e.id==character_id) return &e;
    }
    return nullptr;
}
bool is_boss_id(EnemyTable& enemy_table,s32 id){
    for(const auto& e:enemy_table.bosses){
        if(e.id==id) return true;
    }
    return false;
}

MapSetting get_settings(u64 map_id,const Config& config){
    for(const auto& b:config.map_settings){
        if((uint64_t)b.map_id==map_id){
            return b;
        }
    }
    return {};
}

bool should_enemy_be_randomize(EntityType entity_type,const Config config){
    bool valid = false;
         if(entity_type==EntityType::ENEMY  ) valid=true;
    else if(entity_type==EntityType::ELITE  ) valid=true;
    else if(entity_type==EntityType::PHANTOM) valid=true;
    else if(entity_type==EntityType::SUMMON ) valid=config.replace_summons;
    else if(entity_type==EntityType::INVADER) valid=config.replace_invaders;
    else if(entity_type==EntityType::MIMIC  ) valid=config.randomize_mimics;
    else if(entity_type==EntityType::LIZARD ) valid=config.randomize_lizards;
    else if(entity_type==EntityType::HOLLOW ) valid=!config.remove_invis;
    return valid; 
}

s32 create_new_enemy(EnemyTable& enemy_table,s32 og_enemy_id){
    auto enemy_param_ptr = find_enemy_param_ptr(enemy_table.enemy_params,og_enemy_id);
    if(enemy_param_ptr){
        return (s32)insert_next_free_entry((u64)400000,*enemy_param_ptr,enemy_table.enemy_params);
    }else{
        auto enemy_ptr = find_enemy_ptr_by_id(enemy_table,og_enemy_id);
        if(enemy_ptr) std::cout<<"Failed to find enemy param for: "<<enemy_ptr->name<<"\n";
        return 0;
    }
}

s32 create_new_boss(EnemyTable& enemy_table,s32 boss_index,float hp_mult,float dmg_mult,float def_mult,float souls_mult){
    auto&boss = enemy_table.bosses[boss_index];
    auto boss_id = boss.variations.front().id;
    auto ptr = get_entry_ptr(enemy_table.enemy_params,boss_id);
    if(!ptr){
        return 0;
    }
    auto ep = *ptr;
    s32 new_boss_id = 0;
    if(ep.id!=-1){
        ep.hp=static_cast<s32>((float)ep.hp*hp_mult);
        for(auto& hp:ep.ngp_hp) hp=static_cast<s32>((float)hp*hp_mult);
        ep.dmg_mult=dmg_mult;
        ep.defense=static_cast<s32>((float)ep.defense*def_mult);
        ep.souls_held=static_cast<s32>((float)boss.souls_held*souls_mult);
        // std::cout<<boss.name<<" "<<boss_index<<" "<<ep.souls_held<<" "<<souls_mult<<" "<<boss.souls_held<<'\n';
        //new_boss_id = (s32)insert_next_free_entry((u64)boss_id,ep,enemy_table.enemy_params);
        new_boss_id = (s32)insert_next_free_entry((u64)400000,ep,enemy_table.enemy_params);
    }else{
        std::cout<<"Failed to find enemy param for: "<<boss.name<<"\n";
        std::cout<<"Failed to create new boss: "<<boss.name<<"\n";
    }
    return new_boss_id;
}

float linear_interpolation(float a,float b,float p){
    return (b-a)*p+a;
}

float scale_hp(float hp,float target,float scale_factor){
    if(hp<target) return hp;
    return std::powf(hp/target,1.f-scale_factor)*target;
}
float scale_dmg(float dmg,float target,float scale_factor){
    if(dmg>target) return 1.f;
    float dmg_ratio =  dmg/target;
    return linear_interpolation(1.f,std::powf(dmg_ratio,2.75f),scale_factor);
}

bool balance_enemy(EnemyTable& enemy_table,s32 enemy_id,float hp_target,float hp_scaling,float dmg_target,float dmg_scaling){
    EnemyParam* ptr = get_entry_ptr(enemy_table.enemy_params,enemy_id);
    if(!ptr) return false;//???
    float og_hp   = ptr->hp;
    float og_dmg  = ptr->dmg_mult;
    s32 og_souls  = ptr->souls_held;
    // std::cout<<ptr<<" "<<og_hp<<" "<<og_dmg<<" "<<og_souls<<'\n';

    float npg_dmg = enemy_table.ngp_dmg_scaling[ptr->dmg_table]/100.f;
    auto hp_mult  = scale_hp(ptr->hp,hp_target,hp_scaling)/ptr->hp;
    auto dmg_mult = scale_dmg(npg_dmg,dmg_target,dmg_scaling);
    ptr->hp*=hp_mult;
    ptr->dmg_mult*=dmg_mult;
    float souls_mult=hp_mult*dmg_mult;
    ptr->souls_held*=souls_mult;
    //std::cout<<ptr->souls_held<<" "<<souls_mult<<'\n';
    //auto name = find_enemy_ptr_by_id(enemy_table,ptr->id)->name;
    // std::cout<<ptr->id<<" "<<name<<"->"<<map_name<<" "<<hp_target<<" "<<dmg_target<<'\n';
    // std::cout<<"HP "<<og_hp<<"->"<<ptr->hp<<" "<<hp_mult<<'\n';
    // std::cout<<"DMG "<<npg_dmg<<" "<<og_dmg<<"->"<<ptr->dmg_mult<<" "<<dmg_mult<<'\n';
    // std::cout<<"SOULS "<<og_souls<<"->"<<ptr->souls_held<<" "<<souls_mult<<'\n';
    return true;
}
bool randomize_enemies(GameData& map_data,EnemyTable& enemy_table,const Config& config){
    //Bless this mess
    u64 regist_start_row = 1000000000u;
    std::mt19937_64 random_generator;
    std::mt19937_64 boss_chance_generator(config.seed);
    std::mt19937_64 deck_shuffler(config.seed);
    std::unordered_map<s32,s32> enemy_id_mapping;
    std::vector<size_t> boss_index;
    boss_index.reserve(32);

    float hp_scaling = config.enemy_hp_scaling/100.f;
    float dmg_scaling = config.enemy_dmg_scaling/100.f;

    //Gather the valid indexes of the enemy table
    std::vector<size_t> allowed_enemies_index;
    allowed_enemies_index.reserve(enemy_table.enemies.size());
    for(size_t i = 0;i<enemy_table.enemies.size();i++){
        if(!vector_contains(config.banned_enemies,(size_t)enemy_table.enemies[i].id)){
            allowed_enemies_index.push_back(i);
        }
    }

    std::vector<size_t> allowed_bosses_index;
    allowed_bosses_index.reserve(enemy_table.bosses.size());
    for(size_t i = 0;i<enemy_table.bosses.size();i++){
        if(!vector_contains(config.banned_enemies,(size_t)enemy_table.bosses[i].id)){
            allowed_bosses_index.push_back(i);
        }
    }
    bool can_bosses_spawn = config.roaming_boss;
    bool boss_only = can_bosses_spawn&&config.roaming_boss_chance==100;
    if(allowed_bosses_index.empty()){
        if(boss_only){
            std::cout<<"WARNING: All bosses are banned on boss only run, randomization not performed\n";
            return false;
        }
        std::cout<<"WARNING: All bosses are banned, roaming bosses disabled\n";
        can_bosses_spawn=false;

    }
    if(allowed_enemies_index.empty()&&!boss_only&&!config.shuffle_enemies){
        std::cout<<"WARNING: All enemies are banned, randomization not performed\n";
        return false;
    }

    //SHUFFLE MODE: instead of drawing random enemies, take the enemies that are
    //already placed and redistribute them via a permutation, so the multiset of
    //enemies in the game is preserved (every enemy still appears the same number
    //of times, just in different spots). collect_original() grabs the concrete
    //EnemyInstance (register + ai) currently at a slot; helper below builds the
    //shuffled slot->instance assignment.
    auto collect_original=[&](MapData& m,size_t j,EnemyInstance& out)->bool{
        const auto& gen_data=m.generator.data[j];
        const Register* r=find_regist_ptr(m,gen_data.generator_regist_param);
        if(!r) return false;
        out.ai_think=gen_data.ai_think_id;
        out.regist=*r;
        return true;
    };
    //Keyed by (map_index,slot_index). Populated globally here when shuffling the
    //whole game; populated per-map inside the loop when shuffling each map alone.
    std::map<std::pair<size_t,size_t>,EnemyInstance> shuffle_assignment;
    if(config.shuffle_enemies&&config.shuffle_global){
        std::vector<std::pair<size_t,size_t>> refs;
        std::vector<EnemyInstance> pool;
        for(size_t mi=0;mi<map_data.size();mi++){
            auto& map=map_data[mi];
            MapSetting settings=get_settings(map.id,config);
            if(!settings.randomize||settings.enemy_limit==0) continue;
            for(size_t j=0;j<map.generator.row_info.size();j++){
                auto et=map.entity_info[map.generator.row_info[j].row].type;
                if(!should_enemy_be_randomize(et,config)) continue;
                EnemyInstance inst;
                if(!collect_original(map,j,inst)) continue;
                refs.push_back({mi,j});
                pool.push_back(std::move(inst));
            }
        }
        std::mt19937_64 shuffle_gen(config.seed);
        std::shuffle(pool.begin(),pool.end(),shuffle_gen);
        for(size_t k=0;k<refs.size();k++) shuffle_assignment[refs[k]]=pool[k];
        std::cout<<"Global enemy shuffle: "<<pool.size()<<" enemies redistributed across the game\n";
    }

    //Population report: tally the original enemies at replaceable slots so we can
    //confirm a shuffle keeps the multiset intact (and show how randomize changes it).
    std::map<s32,int> orig_hist,placed_hist;
    for(size_t mi2=0;mi2<map_data.size();mi2++){
        auto& m=map_data[mi2];
        MapSetting s=get_settings(m.id,config);
        if(!s.randomize||s.enemy_limit==0) continue;
        for(size_t j=0;j<m.generator.row_info.size();j++){
            auto et=m.entity_info[m.generator.row_info[j].row].type;
            if(!should_enemy_be_randomize(et,config)) continue;
            EnemyInstance inst;
            if(collect_original(m,j,inst)) orig_hist[inst.regist.enemy_id]++;
        }
    }

    for(auto& map:map_data){
        size_t mi = static_cast<size_t>(&map - map_data.data());
        MapSetting settings = get_settings(map.id,config);
        if(!settings.randomize)continue;
        if(settings.enemy_limit==0) continue;
        auto& generator = map.generator;
        bool can_bosses_spawn_zone = can_bosses_spawn;
        if(can_bosses_spawn&&settings.enemy_limit==1&&!boss_only){
            std::cout<<"WARNING: Need at least 2 enemy limit if not using 100% boss replace chance for wandering bosses to spawn.If you want the same one boss use 100% boss replace chance\n";
            can_bosses_spawn_zone=false;
        }
        struct EnemyEntry{
            size_t index;
            s32 boss_id;
            EnemyInstance enemy;
            bool replace;
            bool boss;
        };
        std::vector<EnemyEntry> enemy_slots;
        enemy_slots.resize(generator.row_info.size());
        size_t replace_count=0;
        for(size_t j = 0;j<generator.row_info.size();j++){
            auto entity_type = map.entity_info[generator.row_info[j].row].type;
            bool replace = should_enemy_be_randomize(entity_type,config);
            if(replace) replace_count+=1;
            enemy_slots[j].replace=replace;
            enemy_slots[j].boss=false;
            enemy_slots[j].index=0;
        }
        if(replace_count==0){
            std::cout<<"WARNING: Can't replace anything in: "<<map.name<<", skipping\n";
            continue;
        }
        if(config.shuffle_enemies){
            //Assign each replaceable slot one of the shuffled original enemies.
            //Global shuffle was precomputed above; per-map shuffle is built here
            //(seeded per map so each area is independent).
            const std::map<std::pair<size_t,size_t>,EnemyInstance>* assign=&shuffle_assignment;
            std::map<std::pair<size_t,size_t>,EnemyInstance> local_assignment;
            if(!config.shuffle_global){
                std::vector<size_t> slot_js;
                std::vector<EnemyInstance> pool;
                for(size_t j=0;j<enemy_slots.size();j++){
                    if(!enemy_slots[j].replace) continue;
                    EnemyInstance inst;
                    if(!collect_original(map,j,inst)) continue;
                    slot_js.push_back(j);
                    pool.push_back(std::move(inst));
                }
                std::mt19937_64 shuffle_gen(config.seed+hash_str_uint32(map.name));
                std::shuffle(pool.begin(),pool.end(),shuffle_gen);
                for(size_t k=0;k<slot_js.size();k++) local_assignment[{mi,slot_js[k]}]=pool[k];
                assign=&local_assignment;
            }
            for(size_t j=0;j<enemy_slots.size();j++){
                auto& slot=enemy_slots[j];
                if(!slot.replace) continue;
                auto it=assign->find({mi,j});
                if(it==assign->end()){ slot.replace=false; continue; }//no original enemy to move here
                slot.enemy=it->second;
                slot.boss=false;
                slot.index=0;
            }
        }else{
        //Precalculate some variables and create the necessary bosses
        size_t different_enemies = 0;
        std::vector<s32> new_bosses_ids(settings.enemy_limit,0);
        if(boss_only){
            boss_index = rng::choose_n_elements(allowed_bosses_index,settings.enemy_limit,false,boss_chance_generator);
            for(auto& entry:enemy_slots){
                if(!entry.replace)continue;
                size_t random_index = rng::vindex(boss_index,boss_chance_generator);
                if(new_bosses_ids[random_index]==0){
                    s32 new_boss_id = create_new_boss(enemy_table,boss_index[random_index],1.f,1.f,1.f,1.f);
                    if(new_boss_id==0) return false;
                    new_bosses_ids[random_index]=new_boss_id;
                }
                entry.index=boss_index[random_index];
                entry.boss_id=new_bosses_ids[random_index];
                entry.boss=true;
            }
        }else if (can_bosses_spawn_zone){
            size_t different_bosses = (size_t)std::ceilf((float)settings.enemy_limit*((float)config.roaming_boss_chance/100.f));
            boss_index = rng::choose_n_elements(allowed_bosses_index,different_bosses,false,boss_chance_generator);
            //Theres a chance that even if n bosses are used the rolls can spawn less
            size_t different_used_bosses = 0;
            for(auto& entry:enemy_slots){
                bool good_luck = rng::roll(config.roaming_boss_chance,100,boss_chance_generator);
                if(!entry.replace||!good_luck)continue;
                size_t random_index = rng::vindex(boss_index,boss_chance_generator);
                if(new_bosses_ids[random_index]==0){
                    s32 new_boss_id = create_new_boss(enemy_table,boss_index[random_index],1.f,1.f,1.f,1.f);
                    if(new_boss_id==0) return false;
                    new_bosses_ids[random_index]=new_boss_id;
                    different_used_bosses+=1;
                }
                entry.index=boss_index[random_index];
                entry.boss_id=new_bosses_ids[random_index];
                entry.boss=true;
            }
            different_enemies=settings.enemy_limit-different_used_bosses;
        }else{
            different_enemies=settings.enemy_limit;
        }
        if(different_enemies>allowed_enemies_index.size()){
            std::cout<<"WARNING: Not enough allowed enemies to fill area types.\n";
            std::cout<<different_enemies<<" types and "<<allowed_enemies_index.size()<<" allowed enemies\n";
        }
        //Select the enemy types for this zone
        //Need to salt the seed for each different map to keep each map independent
        //Dont know if this is an ok way to it
        random_generator.seed(config.seed+hash_str_uint32(map.name));
        std::vector<size_t> enemies_id = rng::choose_n_elements(allowed_enemies_index,different_enemies,false,random_generator);
        if(!enemies_id.empty()){
            bool single_deck = config.enemy_shuffling==0;
            bool fit_deck    = config.enemy_shuffling==1;
            bool large_deck  = config.enemy_shuffling==2;
            bool random_deck = config.enemy_shuffling==3;
            //Generate the deck
            std::vector<size_t> enemy_deck;
            if(single_deck){
                enemy_deck=enemies_id;
            }else if(fit_deck || large_deck){
                size_t fit = ((replace_count/enemies_id.size()) + 1 );
                if(large_deck) fit*=3;
                enemy_deck.resize(fit*enemies_id.size());
                for(size_t i = 0;i<fit;i++){
                    std::copy(enemies_id.begin(),enemies_id.end(),enemy_deck.begin()+i*enemies_id.size());
                }
                std::shuffle(enemy_deck.begin(),enemy_deck.end(),deck_shuffler);
            }else{
                if(!random_deck){
                    std::cout<<"Unkown shuffling mode, using random\n";
                }
                enemy_deck.resize(replace_count);
                for(auto& card:enemy_deck){
                    card = rng::element(enemies_id,deck_shuffler);
                }
            }
            //Use the deck to select enemies
            size_t deck_index=0;
            for(auto& slot:enemy_slots){
                if(!slot.replace) continue;
                auto enemy_index = enemy_deck[deck_index];
                // if(map.name=="Shrine of Amana"){
                //     std::cout<<enemy_index<<'\n';
                // }
                deck_index+=1;
                if(deck_index==enemy_deck.size()){//Guarantees an even distribution of enemies
                    //This should only happen on single deck scenario
                    std::shuffle(enemy_deck.begin(),enemy_deck.end(),deck_shuffler);
                    deck_index=0;
                }
                if(!slot.boss) slot.index=enemy_index;
            }
        }
        
        //Select enemy instances 
        for(auto& slot:enemy_slots){
            if(!slot.replace)continue;
            if(slot.boss){
                const auto& boss_variation = enemy_table.bosses[slot.index].variations.front();
                random_generator.discard(1);//Make it so replacing the enemy for a boss doesn't change the generator state
                slot.enemy = rng::element(boss_variation.instances,random_generator);
                slot.enemy.regist.enemy_id=slot.boss_id;//Replace by the created boss
            }else{
                const auto& variations = enemy_table.enemies[slot.index].variations;
                const auto& instances  = rng::element(variations,random_generator).instances;
                slot.enemy = rng::element(instances,random_generator);
            }
        }
        }//end shuffle/randomize selection
        //Tally what actually got placed (before scaling rewrites enemy ids)
        for(auto& slot:enemy_slots){
            if(slot.replace&&!slot.boss) placed_hist[slot.enemy.regist.enemy_id]++;
        }
        //Scaling
        if(config.enemy_scaling&&map.enemy_scaling<2000){//Only scale certain zones
            enemy_id_mapping.clear();
            float hp_target = (float)map.enemy_scaling;
            float dmg_target = map.dmg_scaling; 
            for(auto& slot : enemy_slots){
                if(!slot.replace)continue;
                if(slot.boss){
                    auto iter = enemy_id_mapping.find(slot.boss_id);
                    if(iter==enemy_id_mapping.end()){//Balance boss if not already balanced
                        balance_enemy(enemy_table,slot.enemy.regist.enemy_id,hp_target,hp_scaling,dmg_target,dmg_scaling);
                        enemy_id_mapping[slot.boss_id]=slot.boss_id;
                    }
                }else{
                    auto enemy_id = slot.enemy.regist.enemy_id;
                    auto iter = enemy_id_mapping.find(enemy_id);
                    if(iter==enemy_id_mapping.end()){//Create new enemy and balance
                        auto new_enemy_id = create_new_enemy(enemy_table,enemy_id);
                        if(new_enemy_id==0) return false; 
                        enemy_id_mapping[enemy_id]=new_enemy_id;
                        slot.enemy.regist.enemy_id=new_enemy_id;
                        balance_enemy(enemy_table,slot.enemy.regist.enemy_id,hp_target,hp_scaling,dmg_target,dmg_scaling);
                    }else{
                        slot.enemy.regist.enemy_id=iter->second;
                    }
                }
            }
        }
        //Change the enemies
        for(size_t j = 0;j<generator.data.size();j++){
            auto& slot = enemy_slots[j];
            if(!slot.replace)continue;
            auto& gen_data = generator.data[j];
            
            if(slot.boss){ //Makes the bosses work outside their arena
                gen_data.activation_event_id[0]=105501;
            }
            if(!config.respawn_roaming_boss&&slot.boss){
                gen_data.spawn_limit=1;
                gen_data.spawn_limit_clear=1;
            }else if(gen_data.spawn_limit==0){ //Make the enemy respawn the correct amount of times
                auto original_enemy_regist = find_regist_ptr(map,gen_data.generator_regist_param);
                if(original_enemy_regist){
                    auto original_param = find_enemy_param_ptr(enemy_table.enemy_params,original_enemy_regist->enemy_id);
                    if(original_param){
                        gen_data.spawn_limit=original_param->spawn_limit;
                        gen_data.spawn_limit_clear=original_param->spawn_limit;
                    }
                }
            }
            

            //Change the enemy generator to use the new register and ai
            gen_data.ai_think_id=slot.enemy.ai_think;
            gen_data.generator_regist_param=(u32)regist_start_row;
            
            //Change the draw group so it actually shows up
            auto& entity_info = map.entity_info[generator.row_info[j].row];
            slot.enemy.regist.draw_group=entity_info.draw_group;
            slot.enemy.regist.display_group=entity_info.display_group;

            //Add the register of the enemy
            add_entry(regist_start_row,slot.enemy.regist,map.regist);
            regist_start_row+=1;
        }
    }

    //--- Enemy population report ---
    size_t total_placed=0;
    for(const auto& kv:placed_hist) total_placed+=kv.second;
    std::cout<<"--- Enemy population report ---\n";
    std::cout<<(config.shuffle_enemies?"[shuffle] ":"[randomize] ")
             <<"placed enemies: "<<total_placed
             <<" | distinct enemy types  original: "<<orig_hist.size()
             <<"  result: "<<placed_hist.size()<<"\n";
    if(config.shuffle_enemies){
        std::cout<<"Population identical to the original (every enemy appears the same number of times): "
                 <<((orig_hist==placed_hist)?"YES":"NO")<<"\n";
    }

    return true;
}

void remove_invaders_summons_invis(GameData& map_data,const Config& config){
    for(auto& map:map_data){
        MapSetting settings = get_settings(map.id,config);
        if(!settings.randomize) continue;
        auto& generator = map.generator;
        size_t deleted_count=0;
        for(size_t j = 0;j<generator.data.size();j++){
            auto entity_type = map.entity_info[generator.row_info[j].row].type;
            bool need_to_delete = false;
                 if(entity_type==EntityType::INVADER&&config.remove_invaders) need_to_delete=true;
            else if(entity_type==EntityType::SUMMON &&config.remove_summons)  need_to_delete=true;
            else if(entity_type==EntityType::HOLLOW &&config.remove_invis)    need_to_delete=true;
            if(need_to_delete){
                delete_entry(j,map.generator);
                j-=1;
                deleted_count+=1;
            }
        }
    }
}
void npc_cloning(GameData& map_data,EnemyTable& enemy_table,const Config& config){
    u64 regist_start_row = 1100000000u;
    std::mt19937_64 random_generator;
    random_generator.seed(config.seed);
    auto npc_index = rng::vindex(enemy_table.npcs,random_generator);
    for(auto& map:map_data){
        auto& generator = map.generator;
        //Change all NCPS to use the same model
        for(size_t j = 0;j<generator.data.size();j++){
            auto& entity_info = map.entity_info[generator.row_info[j].row];
            auto entity_type = entity_info.type;
            bool valid_entity = false;
            if(entity_type==EntityType::NPC)valid_entity=true;
            if(!valid_entity)continue;
            //Make a copy cause we need to change the draw group
            auto random_enemy = enemy_table.npcs[npc_index];
            if(map.code=="m10_16_00_00"){
                random_enemy=enemy_table.straid;
            }
            //Change the enemy
            //Dont change ai for NPCS
            //Changing their item lot is necessary though
            for(size_t k =0;k<map.regist.data.size();k++){
                if(map.regist.row_info[k].row==generator.data[j].generator_regist_param){
                    auto id = map.regist.data[k].enemy_id;
                    auto e_params = find_enemy_param(enemy_table.enemy_params,id);
                    // std::cout<<"Setting up: "<<id<<" "<<e_params.item_lot<<'\n';
                    //40 means no item
                    if(e_params.item_lot!=40) generator.data[j].item_lot_id[0]=e_params.item_lot;
                    break;
                }
            }
            generator.data[j].generator_regist_param=(u32)regist_start_row;
            //Change the draw group so it actually shows up
            random_enemy.regist.draw_group=entity_info.draw_group;
            random_enemy.regist.display_group=entity_info.display_group;
            //Add the register of the enemy
            add_entry(regist_start_row,random_enemy.regist,map.regist);
            regist_start_row+=1;
        }
    }
}

bool enable_multiboss(const BossArena& arena,const Config& config){
    if(arena.ids.size()<2)   return false;
    if(!config.multiboss.on) return false;
    if(arena.name=="Skeleton Lords")   return config.multiboss.skeleton_lords;
    if(arena.name=="Belfry Gargoyles") return config.multiboss.belfry_gargoyles;
    if(arena.name=="Twin Dragonrider") return config.multiboss.twin_dragonrider;
    if(arena.name=="Throne Defenders") return config.multiboss.throne_watchers;
    if(arena.name=="Ruin Sentinels")   return config.multiboss.ruin_sentinels;
    if(arena.name=="Gank squad")       return config.multiboss.graverobbers;
    if(arena.name=="Lud & Zallen")     return config.multiboss.lud_and_zallen;
    return false;
}

struct BossHolder{
    struct BossReplacementData{
        size_t boss_table_index;
        size_t arena_index;
        size_t arena_boss_index;
        bool skip;
    };
    size_t huge=0,big=0,mid=0;
    std::vector<size_t> huge_index,big_index,mid_index;

    bool skip_mid=false,skip_big=false,skip_all=false;
    std::vector<BossReplacementData> rando_data;
};

BossHolder calculate_boss_holder(EnemyTable& enemy_table,const Config& config){
    BossHolder holder;
    size_t bosses_to_randomize = 0;
    for(const auto& arena:enemy_table.boss_arenas){
        if(!get_settings(arena.map_id,config).randomize)continue;
        size_t boss_count = 1;
        if(enable_multiboss(arena,config)){
            boss_count=arena.ids.size();
        }
        bosses_to_randomize+=boss_count;
        if     (arena.size>=3) holder.huge+=boss_count;
        else if(arena.size==2) holder.big +=boss_count;
        else                   holder.mid +=boss_count;
    }
    auto& bosses = enemy_table.bosses;
    for(size_t i = 0;i<bosses.size();i++){
        auto& b = bosses[i];
        if(vector_contains(config.banned_enemies,(size_t)b.id)) continue;
        if(b.size>=3)      holder.huge_index.push_back(i);
        else if(b.size==2) holder.big_index.push_back(i);
        else               holder.mid_index.push_back(i);
    }
    if(holder.mid_index.empty()&&holder.mid>0){
        holder.skip_mid=true;
        std::cout<<"WARNING: No medium size bosses available, medium size boss arenas will be skipped during randomization\n";
    }
    if(holder.mid_index.empty()&&holder.big_index.empty()&&holder.big>0){
        holder.skip_big=true;
        std::cout<<"WARNING: No big/medium size bosses available, big size boss arenas will be skipped during randomization\n";
    }
    if(holder.mid_index.empty()&&holder.big_index.empty()&&holder.big_index.empty()){
        holder.skip_all=true;
        std::cout<<"WARNING: No bosses available, boss arenas will be skipped during randomization\n";
    }
    return holder;
}

BossHolder generate_boss_deck(EnemyTable& enemy_table,const Config& config,std::mt19937_64& random_generator){
    auto holder = calculate_boss_holder(enemy_table,config);
    if(holder.skip_all) return holder;

    holder.big_index.insert(holder.big_index.end(), holder.mid_index.begin(), holder.mid_index.end());
    holder.huge_index.insert(holder.huge_index.end(), holder.big_index.begin(), holder.big_index.end());
    std::vector<size_t>* deck;
    for(size_t  j = 0;j<enemy_table.boss_arenas.size();j++){
        const auto& arena = enemy_table.boss_arenas[j];
        if(!get_settings(arena.map_id,config).randomize)continue;
        if(arena.size>=3){
            deck = &holder.huge_index;
        }else if(arena.size==2){
            deck = &holder.big_index;
        }else {
            deck = &holder.mid_index;
        }
        BossHolder::BossReplacementData replacement;
        replacement.arena_index = j;
        if(deck->empty() || (arena.size==2 && holder.skip_big) || (arena.size<=1 && holder.skip_mid)){
            replacement.skip=true;
            for(size_t i = 0;i<arena.ids.size();i++){
                holder.rando_data.push_back(replacement);
            }
        }else{
            bool multiboss = enable_multiboss(arena,config);
            replacement.skip=false;
            replacement.boss_table_index = rng::element(*deck,random_generator);
            for(size_t i = 0;i<arena.ids.size();i++){
                replacement.arena_boss_index = i;
                holder.rando_data.push_back(replacement);
                if(multiboss&&((i+1)<arena.ids.size())){
                    replacement.boss_table_index = rng::element(*deck,random_generator);
                }else{
                    random_generator.discard(1);
                }
            }
        }
    }
    return holder;
}

std::vector<u64> get_random_boss_ids(const BossArena& arena,EnemyTable& enemy_table,std::mt19937_64& random_generator,bool multiboss){
    std::vector<u64> replacement_ids;
    size_t boss_count=arena.ids.size();
    std::vector<EnemyType>& bosses=enemy_table.bosses;
    while(replacement_ids.size()<boss_count){
        size_t boss_index = rng::vindex(bosses,random_generator);
        size_t tries = 0;
        while(bosses[boss_index].size>arena.size){//Put bosses where they fit
            boss_index = rng::vindex(bosses,random_generator);
            if((tries++)>10000)break;//Give up
        }
        if(multiboss){
            replacement_ids.push_back(boss_index);
        }else{
            for(size_t i = 0;i<boss_count;i++) replacement_ids.push_back(boss_index);
        }
    }
    return replacement_ids;
}

//Boss shuffle: like the enemy shuffle, but for arena bosses. Instead of dropping
//random bosses into arenas, this permutes the bosses that already occupy the
//arenas, so every boss still appears exactly once - just in a different arena.
//The permutation respects the vanilla size constraint (boss.size <= arena.size),
//which is always satisfiable because the original placement itself is one valid
//assignment. Bosses are placed largest-first into a random eligible arena, which
//guarantees the smaller (less constrained) bosses never starve a bigger one.
BossHolder generate_boss_shuffle(GameData& map_data,EnemyTable& enemy_table,const Config& config,std::mt19937_64& random_generator){
    BossHolder holder;
    auto& bosses = enemy_table.bosses;
    //Map a boss "type" (enemy_id/100) to its index in the boss table. Arena
    //generators reference specific variations (e.g. Dragonrider 611510) that may
    //differ from the variation listed in bosses.txt (611520), but both share the
    //type 6115, which is how the boss table is keyed.
    std::unordered_map<s32,size_t> type_to_boss;
    for(size_t i=0;i<bosses.size();i++)
        type_to_boss[bosses[i].id]=i;

    struct Slot{size_t arena_index;size_t arena_boss_index;int capacity;};
    std::vector<Slot> slots;
    std::vector<size_t> pool;//original boss_table_index for each slot, to be permuted
    std::vector<BossHolder::BossReplacementData> skipped;
    for(size_t j=0;j<enemy_table.boss_arenas.size();j++){
        const auto& arena=enemy_table.boss_arenas[j];
        if(!get_settings(arena.map_id,config).randomize)continue;
        auto map_index=get_map(map_data,arena.map_id);
        for(size_t i=0;i<arena.ids.size();i++){
            size_t orig=SIZE_MAX;
            const char* why="ok";
            if(map_index<map_data.size()){
                auto* g=get_entry_ptr(map_data[map_index].generator,arena.ids[i]);
                if(g){
                    auto* r=find_regist_ptr(map_data[map_index],g->generator_regist_param);
                    if(r){
                        auto it=type_to_boss.find(r->enemy_id/100);
                        if(it!=type_to_boss.end()) orig=it->second; else why="enemy_id not a boss";
                    }else why="no regist";
                }else why="no generator row";
            }else why="map not loaded";
            if(getenv("DS2_BOSS_DEBUG")&&orig==SIZE_MAX)
                std::cout<<"  [boss-skip] arena='"<<arena.name<<"' map="<<arena.map_id<<" gen="<<arena.ids[i]<<" : "<<why<<"\n";
            BossHolder::BossReplacementData rd;
            rd.arena_index=j; rd.arena_boss_index=i; rd.boss_table_index=0;
            if(orig==SIZE_MAX||vector_contains(config.banned_enemies,(size_t)bosses[orig].id)){
                rd.skip=true;//couldn't identify the original boss, leave the arena vanilla
                skipped.push_back(rd);
            }else{
                slots.push_back({j,i,arena.size});
                pool.push_back(orig);
            }
        }
    }

    //Permute pool over slots, largest boss first into a random eligible arena.
    std::vector<size_t> order(pool.size());
    std::iota(order.begin(),order.end(),(size_t)0);
    std::sort(order.begin(),order.end(),[&](size_t a,size_t b){
        return bosses[pool[a]].size>bosses[pool[b]].size;
    });
    std::vector<bool> filled(slots.size(),false);
    std::vector<size_t> assign(slots.size(),SIZE_MAX);
    for(size_t oi:order){
        size_t boss_index=pool[oi];
        int bsize=bosses[boss_index].size;
        std::vector<size_t> eligible;
        for(size_t s=0;s<slots.size();s++)
            if(!filled[s]&&slots[s].capacity>=bsize) eligible.push_back(s);
        if(eligible.empty())//shouldn't happen, but never lose a boss
            for(size_t s=0;s<slots.size();s++) if(!filled[s]) eligible.push_back(s);
        size_t chosen=eligible[rng::vindex(eligible,random_generator)];
        assign[chosen]=boss_index;
        filled[chosen]=true;
    }
    for(size_t s=0;s<slots.size();s++){
        BossHolder::BossReplacementData rd;
        rd.arena_index=slots[s].arena_index;
        rd.arena_boss_index=slots[s].arena_boss_index;
        rd.boss_table_index=assign[s];
        rd.skip=false;
        holder.rando_data.push_back(rd);
    }
    for(auto& rd:skipped) holder.rando_data.push_back(rd);
    std::map<size_t,int> orig_hist,placed_hist;
    for(size_t x:pool) orig_hist[x]++;
    for(size_t a:assign) if(a!=SIZE_MAX) placed_hist[a]++;
    std::cout<<"Boss shuffle: "<<slots.size()<<" bosses redistributed across arenas";
    if(!skipped.empty()) std::cout<<" ("<<skipped.size()<<" arenas left vanilla - original boss not identified)";
    std::cout<<"\n";
    std::cout<<"Boss population identical to the original (every boss still appears once): "
             <<((orig_hist==placed_hist)?"YES":"NO")<<"\n";
    return holder;
}

void randomize_bosses(GameData& map_data,EnemyTable& enemy_table,const Config& config){
    std::stringstream log;
    common::boss_log.clear();
    u64 regist_start_row = 1200000000u;
    std::vector<EnemyType>& bosses=enemy_table.bosses;
    std::mt19937_64 random_generator(config.seed);
    BossHolder holder = config.shuffle_bosses
        ? generate_boss_shuffle(map_data,enemy_table,config,random_generator)
        : generate_boss_deck(enemy_table,config,random_generator);
    if(holder.skip_all){
        std::cout<<"SKIPPING BOSS RANDOMIZATION\n";
        return;
    }
    bool twins = config.boss_balance.easy_twins;
    bool scale_skelelords = config.boss_balance.easy_skelly;
    bool scale_bosses = config.scale_bosses;
    bool rat_rework = config.boss_balance.remaster_rat;
    int rats_to_spawn = config.rats_clones;
    float hp_scale = config.boss_hp_scaling/100.f;
    float dmg_scale = config.boss_dmg_scaling/100.f;
    for(const auto& entry:holder.rando_data){
        const auto& arena = enemy_table.boss_arenas[entry.arena_index];
        log<<"ARENA: "<<arena.name<<" ";
        if(entry.skip){
            log<<"ARENA:"<<arena.name<<" GEN:"<<entry.arena_boss_index<<" SKIPPED\n";
            continue;
        }
        auto map_index = get_map(map_data,arena.map_id);
        if(map_index>=map_data.size()){
            log<<"Can't find correct map with: "<<arena.map_id<<'\n';
            continue;
        }
        auto& map = map_data[map_index];
        auto row = arena.ids[entry.arena_boss_index];
        auto generator = get_entry_ptr(map.generator,row);
        if(!generator){
            log<<"Can't find correct generator: "<<row<<'\n';
            continue;
        }
        auto& entity_info = map.entity_info[row];
        const auto& boss = bosses[entry.boss_table_index];

        log<<row<<" REPLACEMENT:"<<boss.id<<" "<<boss.name<<'\n';
        const auto& variation = boss.variations.front();
        auto random_enemy = rng::element(variation.instances,random_generator);
        if(arena.name=="Twin Dragonrider"&&twins&&row==864){ //864 is the bow guy
            auto ep = find_enemy_param(enemy_table.enemy_params,random_enemy.regist.enemy_id);
            // std::cout<<ep.ng_hp<<" "<<ep.behavior_id<<" "<<ep.id<<" "<<ep.dmg_mult<<"\n";
            if(ep.id!=-1){
                ep.hp=static_cast<s32>(std::pow((float)ep.hp,0.33333f)*50.f);
                ep.dmg_mult*=0.35f;
                // std::cout<<ep.ng_hp<<" "<<ep.behavior_id<<" "<<ep.id<<" "<<ep.dmg_mult<<"\n";
                for(auto& ngp_hp:ep.ngp_hp){
                    ngp_hp=static_cast<s32>(std::pow((float)ngp_hp,0.33333f)*50.f);
                }
                auto new_id = insert_next_free_entry(random_enemy.regist.enemy_id,ep,enemy_table.enemy_params);
                random_enemy.regist.enemy_id=(s32)new_id;
            }
        }else if(arena.name=="Skeleton Lords"&&scale_skelelords){
            auto ep = find_enemy_param(enemy_table.enemy_params,random_enemy.regist.enemy_id);
            // std::cout<<"Skele lords "<<ep.ng_hp<<" "<<ep.behavior_id<<" "<<ep.id<<"\n";
            float scaling[3]={1.f,0.5f,0.25f};
            float scale = scaling[boss.diff];
            if(ep.id!=-1){
                ep.hp=static_cast<s32>((float)ep.hp*scale);
                ep.dmg_mult*=scale;
                for(auto& ngp_hp:ep.ngp_hp){
                    ngp_hp=static_cast<s32>((float)ngp_hp*scale);
                }
                auto new_id = insert_next_free_entry(random_enemy.regist.enemy_id,ep,enemy_table.enemy_params);
                random_enemy.regist.enemy_id=(s32)new_id;
            }
        }else if(arena.name=="Royal Rat Vanguard"&&rat_rework){
            auto rat_enemy = random_enemy;
            //Swap the rats for clones of the boss
            auto rats_new_id = create_new_boss(enemy_table,entry.boss_table_index,0.05f,0.15f,0.7f,0.f);
            if(rats_new_id==0) continue;
            for(size_t z = 0;z<map.generator.data.size();z++){
                auto mrow = map.generator.row_info[z].row;
                if(mrow<9000||mrow>9011)continue;
                auto& mentity_info = map.entity_info[mrow];
                map.generator.data[z].ai_think_id=rat_enemy.ai_think;
                map.generator.data[z].generator_regist_param=(u32)regist_start_row;
                if(mrow<9000+rats_to_spawn){
                    rat_enemy.regist.draw_group=mentity_info.draw_group;
                    rat_enemy.regist.display_group=mentity_info.display_group;
                }else{//Remove this guys
                    rat_enemy.regist.draw_group=0;
                    rat_enemy.regist.display_group=0;
                }
                rat_enemy.regist.enemy_id=rats_new_id;
                add_entry(regist_start_row,rat_enemy.regist,map.regist);
                regist_start_row+=1;
            }

            auto ptr = get_entry_ptr(enemy_table.enemy_params,random_enemy.regist.enemy_id);
            if(!ptr) continue;
            float og_dmg = enemy_table.ngp_dmg_scaling[ptr->dmg_table]/100.f;
            float hp_scaling=linear_interpolation(ptr->hp,arena.hp_target,1.f)/ptr->hp;
            float dmg_ratio = og_dmg/arena.dmg_target;
            float dmg_scaling= linear_interpolation(1.f,std::powf(dmg_ratio,3.f),1.f);
            auto boss_new_id = create_new_boss(enemy_table,entry.boss_table_index,hp_scaling,dmg_scaling,1.f,0.f);
            if(boss_new_id==0) continue;
            random_enemy.regist.enemy_id=boss_new_id;


        }else if(scale_bosses){
            auto ptr = get_entry_ptr(enemy_table.enemy_params,random_enemy.regist.enemy_id);
            if(!ptr) continue;
            float og_dmg = enemy_table.ngp_dmg_scaling[ptr->dmg_table]/100.f;
            float hp_scaling=linear_interpolation(ptr->hp,arena.hp_target,hp_scale)/ptr->hp;
            float dmg_ratio = og_dmg/arena.dmg_target;
            float dmg_scaling= linear_interpolation(1.f,std::powf(dmg_ratio,2.66f),dmg_scale);
            float og_def = ptr->defense;
            float def_scaling = linear_interpolation(og_def,arena.def_target,hp_scale)/ptr->defense;
            auto boss_new_id = create_new_boss(enemy_table,entry.boss_table_index,hp_scaling,dmg_scaling,def_scaling,0.f);
            //Grab ptr again after creating new boss for possible reallocation
            // ptr = get_entry_ptr(enemy_table.enemy_params,random_enemy.regist.enemy_id);
            // auto hhh = get_entry_ptr(enemy_table.enemy_params,boss_new_id);
            // std::cout<<"BOSS: "<<boss.name<<"->"<<arena.name<<" "<<random_enemy.regist.enemy_id<<'\n';
            // std::cout<<"HP "<<ptr->hp<<"->"<<hhh->hp<<" "<<arena.hp_target<<'\n';
            // std::cout<<"DMG "<<ptr->dmg_mult<<"->"<<hhh->dmg_mult<<" "<<dmg_scaling<<'\n';
            // std::cout<<og_dmg<<" "<< arena.dmg_target<<"\n";
            
            if(boss_new_id==0) continue;
            random_enemy.regist.enemy_id=boss_new_id;
        }
        
        

        if(arena.name=="Vendrick"){
            //Make fume knight and aldia not be underground so the fight can be triggered
            if(boss.id==6750||boss.id==6920||boss.id==6070){
                random_enemy.regist.spawn_state=0;
            }
        }

        
        
        generator->ai_think_id=random_enemy.ai_think;
        generator->generator_regist_param=(u32)regist_start_row;

        //Change the draw group so it actually shows up
        random_enemy.regist.draw_group=entity_info.draw_group;
        random_enemy.regist.display_group=entity_info.display_group;

        //Make sure bosses stay death when they are replaced with non-bosses (Dragonrider, Sentries, Chariot,Gargoyles)
        //Getting the same enemy entry multiple times on the same enemy operation kinda bad, simplifies code tho
        auto spawn_ptr = get_entry_ptr(enemy_table.enemy_params,random_enemy.regist.enemy_id);
        if(spawn_ptr){
            spawn_ptr->spawn_limit=1; 
            //Gives whoever is in Mytha's arena poison immunity so they dont die if player doesnt burn
            //the mill, as the boss may die before going into the arena messing up the progression
            if(arena.name=="Mytha the Baneful Queen"){
                spawn_ptr->poison_def=100;
            }
        }

        // random_enemy.regist.draw_goup=1;
        add_entry(regist_start_row,random_enemy.regist,map.regist);
        regist_start_row+=1;
    }
    if(config.write_cheatsheet){
        common::boss_log=log.str();
    }
}

void set_belfry_rush(GameData& map_data,const Config& config){
    for(auto&map:map_data){
        if(map.id==10160000){
            if(!get_settings(map.id,config).randomize)continue;
            auto& generator = map.generator;
            u32 event_id = 116020093;//116020092 is the gargoyles fog gate
            for(size_t j = 0;j<generator.data.size();j++){
                auto& gen_data = generator.data[j];
                auto row = generator.row_info[j].row;
                if(row==8000){
                    gen_data.death_event_id=event_id;
                }else if(row==8001){
                    gen_data.death_event_id=event_id+1;
                    gen_data.activation_event_id[0]=event_id;
                }else if(row==8002){
                    gen_data.death_event_id=event_id+2;
                    gen_data.activation_event_id[0]=event_id+1;
                }else if(row==8003){
                    gen_data.death_event_id=event_id+3;
                    gen_data.activation_event_id[0]=event_id+2;
                }else if(row==8004){
                    gen_data.activation_event_id[0]=event_id+3;
                }
            }
        }
    }
}

void easy_congregation(GameData& map_data,const Config& config){
    std::vector<u64> congregation_ids{2520,2521,2522,2530,2531,2532,2533,2534};
    for(auto& map:map_data){
        if(map.id!=10140000)continue;
        MapSetting settings = get_settings(map.id,config);
        if(!settings.randomize) continue;
        auto& generator = map.generator;
        size_t deleted_count=0;
        for(size_t j = 0;j<generator.data.size();j++){
            auto row = generator.row_info[j].row;
            if(vector_contains(congregation_ids,row)){
                delete_entry(j,map.generator);
                j-=1;
                deleted_count+=1;
            }
        }
    }
}

void reposition_enemies(GameData& map_data,EnemyTable& enemy_table,const Config& config){
    const auto& repos = enemy_table.reposition;
    for(auto& map:map_data){
        MapSetting settings = get_settings(map.id,config);
        if(!settings.randomize) continue;
        auto range = repos.equal_range(map.id);
        for (auto it = range.first; it != range.second; ++it){
            // std::cout << it->first << ' ' << it->second.enemy_row << '\n';
            EnemyRepositioning repo_data = it->second;
            auto& location = map.location;
            for(size_t j = 0;j<location.data.size();j++){
                if(location.row_info[j].row==(u64)repo_data.enemy_row){
                    // std::cout<<"Repositioned\n";
                    location.data[j].position[0]=repo_data.position[0];
                    location.data[j].position[1]=repo_data.position[1];
                    location.data[j].position[2]=repo_data.position[2];
                    break;
                }
            }
        }
    }
}

void full_random(GameData& map_data,EnemyTable& enemy_table,const Config& config){
    if(config.randomize_enemies){
        randomize_enemies(map_data,enemy_table,config);
    }
    if(config.remove_invaders||config.remove_summons||config.remove_invis){
        remove_invaders_summons_invis(map_data,config);
    }
    if(config.replace_npcs){
        npc_cloning(map_data,enemy_table,config);
    }
    if(config.randomize_bosses){
        randomize_bosses(map_data,enemy_table,config);
        if(config.boss_balance.belfry_rush){
            set_belfry_rush(map_data,config);
        }
        if(config.boss_balance.easy_congregation){
            easy_congregation(map_data,config);
        }
        reposition_enemies(map_data,enemy_table,config);
    }
}

bool load_map_data(GameData& map_data){
    const std::filesystem::path generator_folder  {paths::params/"generator"};
    const std::filesystem::path location_folder   {paths::params/"generator_location"};
    const std::filesystem::path regist_folder     {paths::params/"generator_regist"};
    const std::filesystem::path entity_type_folder{"map_enemy_types"};
    
    if(!load_generator_files(generator_folder,map_data)) return false;
    if(!load_regist_files(regist_folder,map_data)) return false;
    if(!load_location_files(location_folder,map_data)) return false;
    if(!load_entity_types(paths::enemy_types,map_data)) return false;
    find_original_draw_groups(map_data);
    return true;
}

bool write_final_params(GameData& data,EnemyTable& enemy_table,bool devmode){
    const std::string generator_prefix{"generatorparam_"};
    const std::string location_prefix{"generatorlocation_"};
    const std::string register_prefix{"generatorregistparam_"};
    const std::string extension{".param"};

    if(!std::filesystem::exists(paths::out_folder)){
        std::filesystem::create_directories(paths::out_folder);
    }
    for(auto& map:data){
        auto generator_out_path = paths::out_folder/(generator_prefix+map.code+extension);
        write_to_file_binary(generator_out_path,write_param_file(map.generator));
        auto regist_out_path = paths::out_folder/(register_prefix+map.code+extension);
        write_to_file_binary(regist_out_path,write_param_file(map.regist));
        auto location_out_path = paths::out_folder/(location_prefix+map.code+extension);
        write_to_file_binary(location_out_path,write_param_file(map.location)); 
    }   
    write_to_file_binary(paths::out_folder/"EnemyParam.param",write_param_file(enemy_table.enemy_params));

    if(devmode){
        const std::filesystem::path out_folder_test{"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Dark Souls II Scholar of the First Sin\\Game\\mods\\mod_testing\\Param"};
        for(auto& map:data){
            auto generator_out_path = out_folder_test/(generator_prefix+map.code+extension);
            write_to_file_binary(generator_out_path,write_param_file(map.generator));
            auto regist_out_path = out_folder_test/(register_prefix+map.code+extension);
            write_to_file_binary(regist_out_path,write_param_file(map.regist));
            auto location_out_path = out_folder_test/(location_prefix+map.code+extension);
            write_to_file_binary(location_out_path,write_param_file(map.location));
        }   
        write_to_file_binary(std::filesystem::path{"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Dark Souls II Scholar of the First Sin\\Game\\mods\\mod_testing\\Param\\EnemyParam.param"},write_param_file(enemy_table.enemy_params));
    }
    return true;
}

void restore_zone_limit_defaults(Config& config){
    for(auto& map:config.map_settings){
        if     (map.map_id==10160000||map.map_id==10150000) map.enemy_limit=8;
        else if(map.map_id==10100000)                       map.enemy_limit=9;
        else                                                map.enemy_limit=10;
        map.randomize=true;
    }
}

std::string generate_config_file(Config& config){
    std::stringstream ss;
    ss<<"#VERSION 0\n";
    ss<<"#INV_REPLACE "<<config.replace_invaders<<"\n";
    ss<<"#INV_REMOVE " <<config.remove_invaders<<"\n";
    ss<<"#SUM_REPLACE "<<config.replace_summons<<"\n";
    ss<<"#SUM_REMOVE " <<config.remove_summons<<"\n";
    ss<<"#NPC_CLONING "<<config.replace_npcs<<"\n";
    ss<<"#SHUFFLE_TYPE "<<config.enemy_shuffling<<'\n';
    ss<<"#SHUFFLE_ENEMIES "<<config.shuffle_enemies<<'\n';
    ss<<"#SHUFFLE_GLOBAL "<<config.shuffle_global<<'\n';
    ss<<"#SHUFFLE_BOSSES "<<config.shuffle_bosses<<'\n';
    ss<<"#CHEATSHEET "<<config.write_cheatsheet<<'\n';

    ss<<"#ENEMY_RANDO "<<config.randomize_enemies<<"\n";
    ss<<"#ENEMY_MIMIC "<<config.randomize_mimics<<"\n";
    ss<<"#ENEMY_LIZARD "<<config.randomize_lizards<<"\n";
    ss<<"#INVIS_ENEMY "<<config.remove_invis<<"\n";
    ss<<"#ROAMING_BOSS "<<config.roaming_boss<<"\n";
    ss<<"#ROAMING_CHANCE "<<config.roaming_boss_chance<<"\n";
    ss<<"#ROAMING_RESPAWN "<<config.respawn_roaming_boss<<'\n';
    ss<<"#ENEMY_SCALING "<<config.enemy_scaling<<'\n';
    ss<<"#HP_SCALE "<<config.enemy_hp_scaling<<'\n';
    ss<<"#DMG_SCALE "<<config.enemy_dmg_scaling<<'\n';

    ss<<"#BOSS_SCALING "<<config.scale_bosses<<'\n';
    ss<<"#BOSS_HP_SCALE "<<config.boss_hp_scaling<<'\n';
    ss<<"#BOSS_DMG_SCALE "<<config.boss_dmg_scaling<<'\n';

    ss<<"#BOSS_RANDO "  <<config.randomize_bosses<<"\n";
    ss<<"#BELFRY_RUSH " <<config.boss_balance.belfry_rush<<"\n";
    ss<<"#EASY_CONGRE " <<config.boss_balance.easy_congregation<<"\n";
    ss<<"#EASY_TWINS "  <<config.boss_balance.easy_twins<<"\n";
    ss<<"#EASY_SKELLYS "<<config.boss_balance.easy_skelly<<"\n";
    ss<<"#REMAKE_RAT "  <<config.boss_balance.remaster_rat<<"\n";
    ss<<"#RAT_CLONES "  <<config.rats_clones<<"\n";


    ss<<"#MULTIBOSS "<<config.multiboss.on<<"\n";
    ss<<"#MB_SKE "<<config.multiboss.skeleton_lords<<"\n";
    ss<<"#MB_GAR "<<config.multiboss.belfry_gargoyles<<"\n";
    ss<<"#MB_RUI "<<config.multiboss.ruin_sentinels<<"\n";
    ss<<"#MB_TWI "<<config.multiboss.twin_dragonrider<<"\n";
    ss<<"#MB_THR "<<config.multiboss.throne_watchers<<"\n";
    ss<<"#MB_GRA "<<config.multiboss.graverobbers<<"\n";
    ss<<"#MB_LUD "<<config.multiboss.lud_and_zallen<<"\n";
    ss<<"#SEED "<<config.seed<<'\n';

    ss<<"#BANNED [";
    size_t banned_count = config.banned_enemies.size();
    for(size_t i =0;i<banned_count;i++){
        ss<<config.banned_enemies[i];
        if(i+1<banned_count)ss<<',';
    }
    ss<<"]\n";

    for(const auto& a:config.map_settings){
        ss<<"#M"<<a.map_id<<" "<<a.enemy_limit<<" "<<a.randomize<<'\n';
    }
    return ss.str();
}

void write_configfile(Config& config){
    std::ofstream file(paths::configfile);
    if(!file){
        std::cout<<"Failed to write enemy configuration file, can't open file:"<<paths::configfile<<"\n";
    }else{
        file<<generate_config_file(config);
        file.close();
    }
}

bool read_configfile(Config& config){
    config.randomize_enemies=true; 
    config.randomize_mimics=true;
    config.randomize_lizards=true;
    config.remove_invis=false;
    config.enemy_shuffling=1;
    config.shuffle_enemies=false;
    config.shuffle_global=true;
    config.shuffle_bosses=false;
    config.randomize_bosses=true;
    

    config.boss_balance.belfry_rush=true;
    config.boss_balance.easy_congregation=true;
    config.boss_balance.easy_skelly=false;
    config.boss_balance.easy_twins=true;
    config.boss_balance.remaster_rat=true;
    config.rats_clones = 5;
    config.roaming_boss=false;
    config.roaming_boss_chance=1;
    config.respawn_roaming_boss=true;
    config.enemy_scaling=true;
    config.enemy_hp_scaling=70;
    config.enemy_dmg_scaling=70;
    config.scale_bosses=true;
    config.boss_hp_scaling=70;
    config.boss_dmg_scaling=70;

    config.multiboss.on=true;
    config.multiboss.skeleton_lords=false;
    config.multiboss.belfry_gargoyles=false;
    config.multiboss.ruin_sentinels=false;
    config.multiboss.twin_dragonrider=false;
    config.multiboss.throne_watchers=true;
    config.multiboss.graverobbers=true;
    config.multiboss.lud_and_zallen=true;

    config.replace_invaders=true;
    config.remove_invaders=false;
    config.replace_summons=false;
    config.remove_summons=true;
    config.replace_npcs=true;
    config.write_cheatsheet=true;
    config.seed=rng::integer<u64>(0u,999999999999999u,rng::m_gen);
    config.banned_enemies = {2130,2131,2261,6000};
    for(const auto& entry:common::map_names){
        MapSetting s;
        s.enemy_limit=10;
        s.map_id=(int)std::get<0>(entry);
        s.map_name=std::get<2>(entry);
        s.randomize=true;
        config.map_settings.push_back(s);
    }
    restore_zone_limit_defaults(config);
    std::ifstream file(paths::configfile);
    std::string line;
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::string_view view = line;
        if(view.substr(0,2)=="//")continue;
        if(view.front()!='#')continue;
        auto tokens = parse::split(line,' ');
        if(tokens.size()<2)continue;
        auto& command = tokens[0];
        if(command=="#BANNED"){
            if(tokens[1].front()!='['&&tokens[1].back()!=']'){
                std::cerr<<"Bad banned enemies row on config file: "<<line<<"\n";
                continue;
            }
            tokens[1].remove_prefix(1);
            tokens[1].remove_suffix(1);
            auto banned_entries = parse::split(tokens[1],',');
            config.banned_enemies.clear();
            config.banned_enemies.reserve(150);
            for(const auto& entry:banned_entries){
                size_t entry_value = 0;
                if(parse::read_var(entry,entry_value)){
                    config.banned_enemies.push_back(entry_value);
                }
                    
            }
            continue;
        }
        uint64_t value1=0;
        if(!parse::read_var(tokens[1],value1)){
            std::cerr<<"Error reading config file line: "<<line<<'\n';
            continue;
        }
        
             if(command=="#INV_REPLACE")config.replace_invaders=value1;
        else if(command=="#INV_REMOVE" )config.remove_invaders=value1;
        else if(command=="#SUM_REPLACE")config.replace_summons=value1;
        else if(command=="#SUM_REMOVE" )config.remove_summons=value1;
        else if(command=="#NPC_CLONING")config.replace_npcs=value1;
        else if(command=="#SHUFFLE_TYPE")config.enemy_shuffling=(u32)value1;
        else if(command=="#CHEATSHEET")config.write_cheatsheet=value1;

        else if(command=="#ENEMY_RANDO" )config.randomize_enemies=value1;
        else if(command=="#ENEMY_MIMIC" )config.randomize_mimics=value1;
        else if(command=="#ENEMY_LIZARD")config.randomize_lizards=value1;
        else if(command=="#INVIS_ENEMY")config.remove_invis=value1;
        else if(command=="#SHUFFLE_ENEMIES")config.shuffle_enemies=value1;
        else if(command=="#SHUFFLE_GLOBAL")config.shuffle_global=value1;
        else if(command=="#SHUFFLE_BOSSES")config.shuffle_bosses=value1;
        else if(command=="#ROAMING_BOSS")config.roaming_boss=value1;
        else if(command=="#ROAMING_CHANCE")config.roaming_boss_chance=(int)value1;
        else if(command=="#ROAMING_RESPAWN")config.respawn_roaming_boss=value1;
        else if(command=="#ENEMY_SCALING")config.enemy_scaling=value1;
        else if(command=="#HP_SCALE")config.enemy_hp_scaling=(int)value1;
        else if(command=="#DMG_SCALE")config.enemy_dmg_scaling=(int)value1;
        else if(command=="#BOSS_SCALING")config.scale_bosses=value1;
        else if(command=="#BOSS_HP_SCALE")config.boss_hp_scaling=(int)value1;
        else if(command=="#BOSS_DMG_SCALE")config.boss_dmg_scaling=(int)value1;

        else if(command=="#BOSS_RANDO")  config.randomize_bosses=value1;
        else if(command=="#BELFRY_RUSH") config.boss_balance.belfry_rush=value1;
        else if(command=="#EASY_CONGRE") config.boss_balance.easy_congregation=value1;
        else if(command=="#EASY_TWINS")  config.boss_balance.easy_twins=value1;
        else if(command=="#EASY_SKELLYS")config.boss_balance.easy_skelly=value1;
        else if(command=="#REMAKE_RAT")  config.boss_balance.remaster_rat=value1;
        else if(command=="#RAT_CLONES")  config.rats_clones=(int)value1;

        else if(command=="#MULTIBOSS")config.multiboss.on=value1;
        else if(command=="#MB_SKE")   config.multiboss.skeleton_lords=value1;
        else if(command=="#MB_GAR")   config.multiboss.belfry_gargoyles=value1;
        else if(command=="#MB_RUI")   config.multiboss.ruin_sentinels=value1;
        else if(command=="#MB_TWI")   config.multiboss.twin_dragonrider=value1;
        else if(command=="#MB_THR")   config.multiboss.throne_watchers=value1;
        else if(command=="#MB_GRA")   config.multiboss.graverobbers=value1;
        else if(command=="#MB_LUD")   config.multiboss.lud_and_zallen=value1;
        else if(command=="#SEED")     config.seed=value1;
        for(const auto& entry:common::map_names){
            auto map_id=std::get<0>(entry);
            if(command=="#M"+std::to_string(map_id)){
                for(auto& a:config.map_settings){
                    if((uint64_t)a.map_id==map_id){
                        a.enemy_limit=(int)value1;
                        if(tokens.size()<3)break;
                        uint64_t value2=1;
                        std::from_chars(tokens[2].data(),tokens[2].data()+tokens[2].size(),value2);
                        a.randomize=value2;
                        break;
                    }
                }
            }
        }
    }

 
    return true;
}


void write_cheatsheet(Config& config){
    if(!std::filesystem::exists(paths::cheatsheet_folder)){
        std::filesystem::create_directories(paths::cheatsheet_folder);
    }
    std::ofstream cheatsheet{paths::cheatsheet_folder/("enemies"+time_string_now()+".txt")};
    if(cheatsheet){
        std::stringstream ss;
        cheatsheet<<"---CONFIGURATION FILE---\n";
        cheatsheet<<generate_config_file(config);
        if(config.randomize_bosses){
            cheatsheet<<"\n\n\n---BOSS CHEATSHEET---\n";
            cheatsheet<<common::boss_log;
        }
    }else{
        std::cout<<"Failed to write enemy cheatsheet\n";
    }
    cheatsheet.close();
}

std::vector<EnemyIdName> get_enemytable(Data& data){
    std::vector<EnemyIdName> table;
    table.reserve(data.enemy_table->enemies.size());
    for(const auto& a:data.enemy_table->enemies){
        table.push_back({a.name,a.id});
    }
    return table;
}
std::vector<EnemyIdName> get_bosstable(Data& data){
    std::vector<EnemyIdName> table;
    table.reserve(data.enemy_table->bosses.size());
    for(const auto& a:data.enemy_table->bosses){
        table.push_back({a.name,a.id});
    }
    return table;
}

bool load_data(Data& data){
    std::cout<<"Loading enemy randomizer data\n";
    Stopwatch clock;
    data.game_data = new GameData;
    data.config.valid=false;
    read_configfile(data.config);
    load_map_names(*data.game_data);
    if(!load_map_data(*data.game_data)){
        std::cout<<"Failed to load map data\n";
        return false;
    }
    data.enemy_table = new EnemyTable;
    if(!load_enemy_table(*data.enemy_table,*data.game_data)){
        std::cout<<"Failed to load enemy table\n";
        return false;
    }
    auto time = clock.passed()/1000;
    std::cout<<"Successful enemy randomizer load in: "<<time<<"ms\n";
    // test_location_generator_parity(*data.game_data);
    data.config.valid=true;
    return true;
}
bool randomize(Data& data,bool devmode){
    auto& config = data.config;
    if(!config.valid){
        std::cout<<"Enemy randomizer loading went wrong, enemy randomizer skipped\n";
        return false;
    }
    //Make copy in case of multiple randomizations in same session
    //This was an annoying bug to track
    auto data_copy = *data.game_data;
    auto enemy_copy = *data.enemy_table;
    full_random(data_copy,enemy_copy,config);
    delete_unused_registers(data_copy);
    write_final_params(data_copy,enemy_copy,devmode);
    if(config.write_cheatsheet){
        write_cheatsheet(config);
    }
    return true;
}


bool copy_directory_files(const std::filesystem::path& from,const std::filesystem::path& to){
    bool success=true;
    for(const auto& entry: std::filesystem::directory_iterator(from)){
        if(entry.is_regular_file()){
            success&=std::filesystem::copy_file(entry.path(),to/entry.path().filename(),std::filesystem::copy_options::overwrite_existing);
        }
    }
    return success;
}

bool restore_default_params(bool devmode){
    const std::string extension{".param"};
    std::filesystem::path out = paths::out_folder;
    if(devmode) out = "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Dark Souls II Scholar of the First Sin\\Game\\mods\\mod_testing\\Param";
    //Cppref wasnt clear whether this thing returns false or throws when it fails so idk
    bool success=true;
    try{
        success&=copy_directory_files(paths::params/"generator",out);
        success&=copy_directory_files(paths::params/"generator_location",out);
        success&=copy_directory_files(paths::params/"generator_regist",out);
        success&=std::filesystem::copy_file(paths::params/"EnemyParam.param",out/"EnemyParam.param",std::filesystem::copy_options::overwrite_existing);
    }catch (std::filesystem::filesystem_error& e){
        std::cout << "Could not restore defaults: " << e.what() << '\n';
        return false;
    }
    if(!success){
        std::cout << "Could not restore defaults.\n";
    }
    return success; 
}


void free_stuff(Data& data){
    if(data.game_data){
        delete data.game_data;
    }
    if(data.enemy_table){
        delete data.enemy_table;
    }
}

};