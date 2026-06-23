#include "modules/item_rando.hpp"
#include "modules/param_editor.hpp"
#include "modules/utils.hpp"

namespace item_rando{
//This thing is used to place the keys so no softlocks happen
namespace solver{
    constexpr size_t max_keys=4;
    struct Door{
        uint32_t from,to;
        uint32_t keys[max_keys];
    };
    struct Key{
        std::string name;
        uint64_t total_amount;
    };
    struct Room{
        std::string name;
        uint64_t capacity;
    };

    struct Graph{
        uint64_t n_nodes;
        std::vector<Door> blockades;
        std::vector<Key> keys;
        std::vector<Room> rooms;
    };

    struct InnerConstraint{
        uint32_t node;
        uint32_t keys[max_keys];
    };
    struct OuterConstraints{
        std::vector<InnerConstraint> paths;
    };
    struct Placement{
        std::vector<uint64_t> node;
        uint64_t left;
    };
    struct GraphState{
        std::vector<OuterConstraints> cons;
        std::vector<Placement> placements;
    };
    GraphState generate_state(const Graph& graph){
        GraphState state;
        if(graph.n_nodes==0){
            std::cout<<"WARNING: Graph has 0 nodes\n";
        }
        state.cons.resize(graph.n_nodes);
        for(const auto& block:graph.blockades){
            if(block.to>=state.cons.size()||block.from>=state.cons.size()){
                std::cout<<"WARNING: Blockade outside of range, from:"<<block.from<<" to:"<<block.to<<'\n';
                continue;
            }
            auto& node = state.cons[block.to];
            InnerConstraint inner;
            inner.node = block.from;
            for(size_t j = 0;j<max_keys;j++){
                inner.keys[j]=block.keys[j];
            }   
            node.paths.push_back(inner);
        }
        state.placements.resize(graph.keys.size());
        for(size_t i = 0;i<graph.keys.size();i++){
            state.placements[i].left = graph.keys[i].total_amount;
        }
        return state;
    }

    std::vector<size_t> get_valid_nodes(const Graph& graph,const GraphState& state,size_t key_id){
        Stopwatch clock;
        std::vector<Door> wait_list;
        std::vector<Door> active_list;
        std::vector<bool> valid_nodes;
        valid_nodes.resize(graph.n_nodes,false);
        //Mark starting node as valid and expand it
        valid_nodes[0]=true;
        for(const auto& b:graph.blockades){
            if(b.from==0) wait_list.push_back(b);
        }
        //std::cout<<"Start algorithm\n";
        bool done = false;
        while(!done){
            active_list=wait_list;
            wait_list.clear();
            done=true;
            while(!active_list.empty()){
                auto active_block = active_list.back();
                //std::cout<<"Pop "<<active_block.from<<" "<<active_block.to<<"\n";
                active_list.pop_back();
                if(valid_nodes[active_block.to]) continue;
                bool all_valid = true;
                bool invalid = false;
                for(size_t j = 0;j<max_keys;j++){
                    auto key = active_block.keys[j];
                    if(key==key_id){
                        all_valid=false;
                        invalid = true;
                        break;
                    }
                    auto& key_nodes = state.placements[key].node;
                    bool repeats=key_nodes.empty();
                    for(const auto& a :key_nodes){
                        repeats|=valid_nodes[a];
                    }
                    all_valid&=repeats;
                }
                if(invalid){
                    //std::cout<<"Invalid\n";
                }else if(all_valid){
                    //std::cout<<"Its valid\n";
                    valid_nodes[active_block.to]=true;
                    for(const auto& block:graph.blockades){
                        if(block.from==active_block.to) active_list.push_back(block);
                    }
                    done=false;//Keep iterating
                }else{
                    //std::cout<<"Not valid\n";
                    wait_list.push_back(active_block);
                }
            }
        }
        std::vector<size_t> compressed_valid;
        for(size_t i =0;i<valid_nodes.size();i++){
            if(valid_nodes[i]) compressed_valid.push_back(i);
        }
        return compressed_valid;
        
    }

    //This is a simple solver, and its not part of the randomizer
    void solve(const Graph& graph,GraphState& state){
        std::vector<size_t> key_deck;
        for(size_t i = 1;i<graph.keys.size();i++){//Skip null key
            key_deck.push_back(i);
        }
        std::mt19937_64 generator(34);
        std::shuffle(key_deck.begin(),key_deck.end(),generator);
        for(const auto& key_id:key_deck){
            auto valid_nodes = get_valid_nodes(graph,state,key_id);
            if(valid_nodes.size()==0){
                std::cout<<"Can't place, no valid nodes\n";
            }else{
                auto random_node = rng::element(valid_nodes,generator);
                // for(const auto& possible_node:valid_nodes){
                //     std::cout<<possible_node<<",";
                // }
                //std::cout<<"Placed in node: "<<random_node<<'\n';
                state.placements[key_id].node.push_back((uint32_t)random_node);
            }
        }
        //std::cout<<"Solved\n";
    }

    bool check_solution(const Graph& graph,const GraphState& state){
        size_t left_to_visit = graph.n_nodes-1;
        std::vector<bool> visited(graph.n_nodes,false);
        visited[0]=true;
        std::vector<bool> keys_obtained(graph.keys.size(),false);
        keys_obtained[0]=true;
        std::vector<Door> doors;
        std::vector<Door> stall_doors;
        for(const auto& a:graph.blockades){
            if(a.from==0)stall_doors.push_back(a);
        }
        for(size_t i = 0;i<state.placements.size();i++){
            for(const auto& node:state.placements[i].node){
                if(node==0)keys_obtained[i] = true;
            }
        }
        bool done=false;
        while(!done){
            doors=stall_doors;
            stall_doors.clear();
            done=true;
            while(!doors.empty()){
                auto door = doors.back();
                doors.pop_back();
                if(visited[door.to])continue;
                bool can_open = true;
                for(size_t j = 0;j<max_keys;j++){
                    auto key = door.keys[j];
                    can_open&=keys_obtained[key];
                }
                if(can_open){
                    visited[door.to]=true;
                    left_to_visit-=1;
                    if(left_to_visit==0) return true;
                    for(const auto& block:graph.blockades){
                        if(block.from==door.to) doors.push_back(block);
                    }
                    for(size_t i = 0;i<state.placements.size();i++){
                        for(const auto& node:state.placements[i].node){
                            if(node==door.to)keys_obtained[i] = true;
                        }
                    }
                    done=false;//Keep iterating
                }else{
                    stall_doors.push_back(door);
                }
            }
        }
        for(size_t i = 0 ;i<visited.size();i++){
            if(!visited[i]){
                std::cout<<"Cant visit: "<<graph.rooms[i].name<<'\n';
            }
        }
        return false;

    }

    void print_solution(const Graph& graph,const GraphState& state){
        for(size_t i = 1;i<state.placements.size();i++){
            if(state.placements[i].node.empty()){
                std::cout<<graph.keys[i].name<<" not placed\n";
            }else{
                std::cout<<graph.keys[i].name<<" in ";
                for(const auto& a:state.placements[i].node){
                    std::cout<<graph.rooms[a].name;
                    if(&a!=&state.placements[i].node.back()) std::cout<<',';
                    else        std::cout<<"\n";
                }
            }
        }

    }

    bool test_graph(const Graph& graph){
        for(int i = 0;i<1000;i++){
            auto state = generate_state(graph);
            solve(graph,state);
            // print_solution(graph,state);
            if(!check_solution(graph,state)){
                std::cout<<"FAILED\n";
                return false;
            }
        }
        std::cout<<"PASSED\n";
        return true;
    }

    size_t room_by_name(const Graph& graph,const std::string& name){
        for(size_t i = 0;i<graph.rooms.size();i++){
            if(graph.rooms[i].name==name){
                return i ;
            }
        }
        std::cout<<"Can't find room with name: "<<name<<'\n';
        return SIZE_MAX;
    }

    bool parse_from_file(const std::filesystem::path& file_path,Graph& graph){
        if(!std::filesystem::exists(file_path)){
            std::cout<<"File doesn't not exists "<<std::filesystem::absolute(file_path)<<'\n';
            return false;
        }
        std::ifstream file(file_path);
        if(!file){
            std::cout<<"Cannot open file: "<<file_path<<'\n';
            return false;
        }
        std::string line;
        bool keys=false,rooms=false,doors=false;
        graph.keys.push_back({"NULL",1});
        while(std::getline(file,line)){
            std::string_view sview = line;
            if(sview.front()=='#'){
                keys = rooms = doors=false;
                if(sview=="#KEYS") keys=true;
                else if(sview=="#ROOMS") rooms=true;
                else if(sview=="#DOORS") doors=true;
                // std::cout<<keys<<" "<<rooms<<" "<<doors<<"\n";
            }else if(sview.substr(0,2)=="//"||sview.size()<2){
                continue;
            }else{
                if(keys){
                    auto tokens = parse::split(sview,',');
                    if(tokens.size()!=2){
                        std::cout<<"Error reading key: "<<sview<<'\n';
                    }else{
                        Key key;
                        key.name = tokens[0];
                        parse::read_var(tokens[1],key.total_amount);
                        graph.keys.push_back(std::move(key));
                    }
                }else if(rooms){
                    Room room;
                    room.name = sview;
                    room.capacity=99999;
                    // std::cout<<"Read room: "<<room.name<<"\n";
                    graph.rooms.push_back(std::move(room));
                }else if(doors){
                    auto tokens = parse::split(sview,',');
                    if(tokens.size()!=3){
                        std::cout<<"Error reading door: "<<sview<<'\n';
                    }else{
                        Door door;
                        door.from = door.to = 999999;
                        for(size_t i = 0;i<graph.rooms.size();i++){
                            if(graph.rooms[i].name==tokens[0]){
                                door.from=(uint32_t)i;
                            }
                            if(graph.rooms[i].name==tokens[1]){
                                door.to=(uint32_t)i;
                            }
                        }
                        if(door.from==999999){
                            std::cout<<"Error reading door: "<<sview<<" no related room: "<<tokens[0]<<'\n';
                            return false;
                        }
                        if(door.to==999999){
                            std::cout<<"Error reading door: "<<sview<<" no related room: "<<tokens[1]<<'\n';
                            return false;
                        }
                        if(tokens[2].size()>=2){
                            tokens[2].remove_prefix(1);
                            tokens[2].remove_suffix(1);
                        }
                        auto key_tokens = parse::split(tokens[2],';');
                        if(key_tokens.size()>max_keys){
                            std::cout<<"Too many keys, max keys is: "<<max_keys<<" found "<< key_tokens.size()<<"\n";
                            std::cout<<"Only taking first for from: "<<sview<<'\n';
                            return false;
                        }
                        for(size_t j = 0;j<max_keys;j++){
                            door.keys[j]=0;//Default to NULL key
                            if(j<key_tokens.size()){
                                //std::cout<<"Key "<<j<<" "<<key_tokens[j]<<'\n';
                                bool found = false;
                                for(size_t i = 0;i<graph.keys.size();i++){
                                    if(graph.keys[i].name==key_tokens[j]){
                                        door.keys[j]=(uint32_t)i;
                                        found = true;
                                        break;
                                    }
                                }
                                if(!found){
                                    std::cout<<"Couldnt match key : "<<key_tokens[j]<<" from: "<<sview<<'\n';
                                    return false;
                                }
                            }
                        }
                        graph.blockades.push_back(std::move(door));
                    }
                }
            }
        }
        graph.n_nodes = graph.rooms.size();
        return true;
    }
    
    bool solver_test1(){
        solver::Graph graph;
        graph.n_nodes=2;
        graph.blockades.push_back({0u,1u,{1u,0u,0u,0u}});
        graph.keys.push_back({"Null",0});//Empty for null stuff
        graph.keys.push_back({"Red key",1});
        return solver::test_graph(graph);
    }
    bool solver_test2(){
        solver::Graph graph;
        graph.n_nodes=4;
        graph.keys.push_back({"Null",0});//Empty for null stuff
        graph.keys.push_back({"Red key",1});
        graph.keys.push_back({"Yellow key",1});
        graph.keys.push_back({"Blue key",1});
        graph.blockades.push_back({0u,1u,{1u,0u,0u,0u}});
        graph.blockades.push_back({0u,2u,{2u,0u,0u,0u}});
        graph.blockades.push_back({0u,3u,{3u,0u,0u,0u}});
        return solver::test_graph(graph);
    }
    bool solver_test3(){
        solver::Graph graph;
        graph.n_nodes=5;
        graph.keys.push_back({"Null",0});//Empty for null stuff
        graph.keys.push_back({"Red key",1});
        graph.keys.push_back({"Blue key",1});
        graph.keys.push_back({"Green key",1});
        graph.keys.push_back({"Yellow key",1});
        graph.keys.push_back({"Magenta key",1});
        graph.blockades.push_back({0u,1u,{5u,0u,0u,0u}});
        graph.blockades.push_back({1u,4u,{1u,0u,0u,0u}});
        graph.blockades.push_back({1u,2u,{2u,0u,0u,0u}});
        graph.blockades.push_back({1u,3u,{4u,0u,0u,0u}});
        graph.blockades.push_back({2u,3u,{3u,0u,0u,0u}});
        graph.blockades.push_back({3u,2u,{3u,0u,0u,0u}});
        return solver::test_graph(graph);
    }
    bool parser_test(){
        solver::Graph graph;
        if(solver::parse_from_file("build/ParserTest.txt",graph)){
            auto state = solver::generate_state(graph);
            solver::solve(graph,state);
            solver::print_solution(graph,state);
        }
        return false;
    }


}

namespace paths{
    const std::filesystem::path configfile = "ir_config.txt";
    const std::filesystem::path items ="data/item_rando/Items";
    const std::filesystem::path lots ="data/item_rando/ItemLots";
    const std::filesystem::path params ="data/item_rando/Params";
    const std::filesystem::path out_folder ="Param";
    const std::filesystem::path cheatsheet_folder ="cheatsheets/items";
    const std::filesystem::path dev_path{"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Dark Souls II Scholar of the First Sin\\Game\\mods\\mod_testing\\Param"};
}
enum class LotType{Shop,Other,Char};
enum class Infusion:u8{None=0,Fire,Magic,Lightning,Dark,Poison,Bleed,Raw,Enchanted,Mundane};
enum class InfusionType:s32{None=0,NoElemental,NoPoisonBleed,NoBleed,DarkMagic,DarkLighting,Elemental,All,Basic};
enum class WeaponType:s32{Common,Bow,Greatbow,Crossbow,Flames,Chime,Staff,Shield};
enum class GearPiece:u32{
    Head=1,Chest=2,Arms=4,Legs=8,Weapon=16,Catalyst=32,Shield=64,Spell=128,Ring=256,Bows=512,
    Left           = Weapon|Shield,
    LeftCat        = Weapon|Shield|Catalyst,
    RightBow       = Weapon|Bows,
    RightShield    = Weapon|Shield,
    RightShieldBow = Weapon|Shield|Bows
};
bool operator&(const GearPiece& rhs,const GearPiece& lhs){return ((u32)rhs&(u32)lhs);}

std::string infusion_to_string(Infusion inf){
    switch (inf)
    {
        case Infusion::None: return "";
        case Infusion::Fire: return "Fire";
        case Infusion::Magic: return "Magic";
        case Infusion::Lightning: return "Lightning";
        case Infusion::Dark: return "Dark";
        case Infusion::Poison: return "Poison";
        case Infusion::Bleed: return "Bleed";
        case Infusion::Raw: return "Raw";
        case Infusion:: Enchanted: return "Enchanted";
        case Infusion:: Mundane: return "Mundane";
        default: return "";
    }
}
float vitality_to_equipment_load(unsigned int vitality){
    if(vitality>=99) return 120.f;
    else if(vitality>=70&&vitality<=98) return 116.f+(vitality-70)*0.25f;
    else if(vitality>=50&&vitality<=69) return 96.f +(vitality-50)*0.5f;
    else if(vitality>=30&&vitality<=49) return 76.f +(vitality-30)*1.f;
    return 38.5f+vitality*1.5f; 
}
WeaponType weapon_type_from_id(s32 weapon_id){
    auto inrange=[](s32 value,s32 low,s32 high){return value>=low&&value<=high;};
    if(inrange(weapon_id,3800000,3999999)) return WeaponType::Staff;
    if(inrange(weapon_id,4000000,4199999)) return WeaponType::Chime;
    if(inrange(weapon_id,4200000,4299999)) return WeaponType::Bow;
    if(inrange(weapon_id,4400000,4499999)) return WeaponType::Greatbow;
    if(inrange(weapon_id,4600000,4699999)) return WeaponType::Crossbow;
    if(inrange(weapon_id,5400000,5499999)) return WeaponType::Flames;
    if(inrange(weapon_id,11000000,11999999)) return WeaponType::Shield;
    return WeaponType::Common;
}

using ItemDropQuantity = std::vector<s32>;
struct SpecialLot{
    LotType type;
    s32 id;
    s32 item_id;
    s32 amount;
    f32 chance;
};
struct LotData{
    u64 lot_id{0};
    s32 item_id{0};
    f32 chance{0.f};
    u8 amount{0};
    u8 infinite{false};
    u8 reinforcement{0u};
    Infusion infusion{0u};
};
struct Item{
    s32 id;
    s32 quantity;
};
struct WeaponSpecs{
    s32 id{0};
    s32 max_lvl{0};
    InfusionType infusion_type{InfusionType::None};
    s32 strength{0},dexterity{0},intelligence{0},faith{0};
    float weight{0.f};
};
struct ArmorSpecs{
    s32 id{0};
    s32 strength{0},dexterity{0},intelligence{0},faith{0};
    float weight{0.f}; 
};
struct SpellSpecs{
    s32 id{0};
    s32 intelligence{0},faith{0};
    s32 spell_type{0};
    s32 slots{0};
};
struct GearSpecs{
    s32 id{0};
    GearPiece gear_type{0};
    u16 strength{0},dexterity{0},intelligence{0},faith{0};
    s32 max_reinforce_lvl{0};
    InfusionType infusion_type{InfusionType::None};
    float weight{0.f};
};
struct ClassGear{
    s32 head{21001100},chest{21001101},arms{21001102},legs{21001103};
    s32 right_hand{3400000},left_hand{3400000};
    s32 ring{-1};
    s32 spell{-1};
    s32 extra_arrow{-1};
    s32 extra_bolt{-1};
    // bool need_arrows{false},need_bolts{false},need_greatarrow{false};
};
struct ClassSpecs{
    s32 id;
    std::string name;
    u16 vigor,end,att,vit,str,dex,intll,fth,adp;
    float weight{0.f},max_weight{0.f};
    ClassGear gear;
};
struct GameItems{
    std::vector<Item> keys;
    std::vector<Item> armor;
    std::vector<Item> rings;
    std::vector<Item> spells;
    std::vector<Item> weapons;
    std::vector<Item> consumables;
    std::vector<Item> unused_items;
    std::vector<Item> special_lots;
    std::unordered_map<s32,std::string> names;
    std::vector<ItemDropQuantity> item_drop_quantity;
    std::vector<size_t> item_drop_index;//This vector is parallel to the consumables one
    std::vector<GearSpecs> gear_specs;
};
struct ShopSlot{
    s32 lot_id;
    s32 item_id;
    float price_mult;
    u8 quantity;
    bool infinite;
};
struct Shops{
    std::vector<ShopSlot> ornifex_trades;
    std::vector<ShopSlot> straid_trades;
    std::vector<ShopSlot> common;
    std::vector<s32> to_remove;
    std::unordered_map<s32,std::string> original_items;
};
struct ItemRandoData{
    std::vector<LotData> lots;
    std::vector<s32> unmissable_lots;
    std::vector<s32> missable_lots;
    std::vector<s32> nochange_lots;
    std::unordered_map<s32,std::string> lot_name_other;

    std::vector<SpecialLot> key_lots;

    std::vector<LotData> chr_lots;
    std::vector<LotData> enemy_drop_lots;
    std::vector<s32> chr_remove;
    std::vector<s32> safe_chr_drop;
    std::vector<s32> enemy_lots;
    std::unordered_map<s32,std::string> enemy_names;
    std::unordered_map<s32,std::string> lot_name;
    
    std::unordered_map<std::string,std::vector<s32>> location_lots; 
    std::vector<std::pair<s32,s32>> equivalents;
    GameItems items;
    Shops shops;
    std::vector<ClassSpecs> classes;
    std::vector<std::vector<Item>> starting_gifts;
};

//Bunch of loading functions should compress some functions
bool load_equivalents(ItemRandoData& data){
    std::filesystem::path path = paths::lots/"EquivalentLots.txt";
    std::ifstream file(path);
    if(!file){
        std::cout<<"Can't open file "<<path<<'\n';
        return false;
    }
    data.equivalents.reserve(100);
    std::string line;
    while(std::getline(file,line)){
        auto tokens = parse::split(line,',');
        if(tokens.size()!=2){
            std::cout<<"Bad line in: "<<path<<" "<<line<<'\n';
            continue;
        }
        data.equivalents.push_back({});
        parse::read_var(tokens[0],data.equivalents.back().first);
        parse::read_var(tokens[1],data.equivalents.back().second);
    }
    //Only works well when there are no repeats, which there shouldnt be
    for(const auto& ids:data.equivalents){
        vector_find_swap_pop(data.unmissable_lots,ids.second);
        vector_find_swap_pop(data.missable_lots,ids.second);
        vector_find_swap_pop(data.safe_chr_drop,ids.second);
    }

    return true;
}
bool load_location_lots(ItemRandoData& data){
    std::filesystem::path path = paths::lots/"FloorItems.txt";
    std::filesystem::path event_path = paths::lots/"EventLots.txt";
    std::ifstream file;

    if(!open_file(file,path)) return false;
    std::string line;
    std::string last_location = "";

    while(std::getline(file,line)){
        std::string_view view = line;
        if(view.substr(0,2)=="//") continue;
        if(view.front()=='#'){
            last_location = view.substr(1);
        }else{
            if(last_location.empty()) continue;
            auto tokens = parse::split(view,',');
            for(const auto& t:tokens){
                s32 value=0;
                if(parse::read_var(t,value)){
                    if(vector_contains(data.unmissable_lots,value)){
                        data.location_lots[last_location].push_back(value); 
                    }
                }
            }
        }
    }
    if(!open_file(file,event_path)) return false;
    while(std::getline(file,line)){
        std::string_view view = line;
        if(view.substr(0,2)=="//") continue;
        auto tokens = parse::split(view,',');
        if(tokens.size()!=3){
            std::cout<<"Bad line: "<<line<<" in "<<event_path<<'\n';
            continue;
        }
        s32 value=0;
        if(parse::read_var(tokens.front(),value)){
            if(vector_contains(data.unmissable_lots,value)){
                data.location_lots[std::string(tokens[1])].push_back(value); 
            }
        }
    }
    // for(auto& location:data.location_lots){
    //     for(size_t i=0;i<location.second.size();i++){
    //         auto lot = location.second[i];
    //         if(!vector_contains(data.unmissable_lots,lot)){
    //             vector_find_swap_pop(location.second,lot);
    //             --i;
    //             std::cout<<"Remove lot: "<<lot<<" is not unmissable\n";
    //         }
    //     }
    // }
    return true;
}
bool load_lots(ItemRandoData& data){
    data.missable_lots.reserve(400);
    data.safe_chr_drop.reserve(600);
    data.unmissable_lots.reserve(950);
    data.lot_name_other.reserve(2048);
    std::filesystem::path chr_lots_path   = paths::lots/"CharacterLots.txt";
    std::filesystem::path other_lots_path = paths::lots/"OtherLots.txt";
    std::ifstream file;
    std::string line;
    if(!open_file(file,other_lots_path))return false;
    bool missable=false,unmissable=false,nochange=false;
    while(std::getline(file,line)){
        if(line.empty()||line.substr(0,2)=="//")continue;
        if(line.front()=='#'){
            missable=unmissable=nochange=false;
            if(line=="#UNMISSABLE") unmissable=true;
            else if(line=="#MISSABLE") missable=true;
            else if(line=="#DONTCHANGE") nochange=true;
            else std::cout<<"Unknown category: "<<line<<" in file: "<<other_lots_path<<'\n';
        }else{
            auto tokens = parse::split(line,';');
            if(tokens.size()!=2){
                std::cout<<"Bad line: |"<<line<<"| in "<<other_lots_path<<'\n';
                continue;
            }
            s32 value=0;
            if(parse::read_var(tokens.front(),value)){
                if(missable){
                    data.missable_lots.push_back(value);
                }else if(unmissable){
                    data.unmissable_lots.push_back(value);
                }else if(nochange){
                    data.nochange_lots.push_back(value);
                }
                data.lot_name_other[value]=tokens[1];
            }
        }
    }
    if(!open_file(file,chr_lots_path))return false;
    bool chr=false,enemy=false,remove=false;
    while(std::getline(file,line)){
        if(line.empty()||line.substr(0,2)=="//")continue;
        if(line.front()=='#'){
            chr=enemy=remove=false;
            if(line=="#GUARANTEED_DROP") chr=true;
            else if(line=="#ENEMY_DROPS") enemy=true;
            else if(line=="#REMOVE") remove=true;
            else{
                std::cout<<"Unknown chr category: "<<line<<'\n';
            }
        }else{
            auto tokens = parse::split(line,',');
            if(tokens.size()!=3){
                std::cout<<"Bad line: |"<<line<<"| in "<<chr_lots_path<<'\n';
                continue;
            }
            s32 value=0;
            if(!parse::read_var(tokens.front(),value)){
                std::cout<<"Bad line: |"<<line<<"| in "<<chr_lots_path<<'\n';
                continue;
            }
            if(chr){
                data.safe_chr_drop.push_back(value);
                data.lot_name[value]=tokens[1];
            }else if(enemy){
                s32 enemy_id = value/10000;
                data.enemy_names[enemy_id]=tokens[1];
                data.enemy_lots.push_back(value);
            }else if(remove){
                data.chr_remove.push_back(value);
            }else{
                std::cout<<"No category set: "<<line<<'\n';
            }
            
        }
    }
    vector_remove_duplicates(data.missable_lots);
    vector_remove_duplicates(data.unmissable_lots);
    vector_remove_duplicates(data.safe_chr_drop);
    return true;
}
bool load_key_lots(ItemRandoData& data){
    std::filesystem::path path = paths::lots/"KeyItemLots.txt";
    std::ifstream file;
    if(!open_file(file,path))return false;
    data.key_lots.reserve(128);
    std::string line;
    s32 key_id = 0;
    while(std::getline(file,line)){
        auto tokens = parse::split(line,',');
        if(tokens[0].front()=='#'){
            if(tokens.size()!=2){
                std::cout<<"Bad line in: "<<path<<" "<<line<<'\n';
                return false;
            }
            tokens[0].remove_prefix(1);
            key_id=0;
            if(!parse::read_var(tokens[0],key_id)) return false;
        }else if(key_id!=0){
            data.key_lots.push_back({});
            auto& lot = data.key_lots.back();
            if     (tokens[0]=="Shop")  lot.type=LotType::Shop;
            else if(tokens[0]=="Other") lot.type=LotType::Other;
            else if(tokens[0]=="Chr")   lot.type=LotType::Char;
            else{
                std::cout<<"Unknown lot type in "<<path<<"\n";
                return false;
            }
            lot.item_id=key_id;
            if(!parse::read_var(tokens[1],lot.id)){
                return false;
            }
            if(!parse::read_var(tokens[2],lot.amount)){
                return false;
            }
            if(lot.type!=LotType::Shop){
                if(!parse::read_var(tokens[3],lot.chance)){
                    return false;
                }
            }
        }else{
            std::cout<<"Bad line in: "<<path<<" "<<line<<'\n';
            std::cout<<"I dont know what key items belongs to this lot\n";
            return false;
        }
    }
    return true;
}
void load_item_quantities(GameItems& items){
    auto& v =items.item_drop_quantity;
    v.push_back({1,2,3});    //Normal
    v.push_back({1});        //Single
    v.push_back({5});        //Pack5
    v.push_back({10,20,30}); //Proj
    v.push_back({3,5,7});    //Bigpack
}
bool load_items(GameItems& items){
    load_item_quantities(items);
    std::filesystem::path path = paths::items/"Item.txt";
    std::ifstream file;
    std::string line;
    std::vector<Item>* item_ptr=nullptr;
    if(!open_file(file,path))return false;
    items.names.reserve(2048);
    while(std::getline(file,line)){
        if(line.substr(0,2)=="//")continue;//Comment
        if(line.front()=='#'){
            if(line=="#KEYS") item_ptr=&items.keys;
            else if(line=="#ITEMS") item_ptr=&items.consumables;
            else if(line=="#RINGS") item_ptr=&items.rings;
            else if(line=="#WEAPONS") item_ptr=&items.weapons;
            else if(line=="#SPELLS") item_ptr=&items.spells;
            else if(line=="#ARMOR") item_ptr=&items.armor;
            else if(line=="#UNUSED_ITEMS") item_ptr=&items.unused_items;
            else if(line=="#SPECIAL_LOTS") item_ptr=&items.special_lots;
            else{
                std::cout<<"Unknown item category: "<<line<<'\n';
            }
        }else{
            if(!item_ptr){
                std::cout<<"Item without category: "<<line<<'\n';
                continue;
            }
            auto tokens = parse::split(line,',');
            int expected = 3;
            if(item_ptr==&items.consumables){
                expected=4;
            }
            if(tokens.size()!=expected){
                std::cout<<"Bad line: |"<<line<<"| in "<<path<<'\n';
                continue;
            }
            Item item;
            parse::read_var(tokens[0],item.id);
            parse::read_var(tokens[1],item.quantity);
            if(item_ptr==&items.consumables){
                size_t item_drop_type=0;
                     if(tokens[2]=="Normal")  item_drop_type=0;
                else if(tokens[2]=="Single")  item_drop_type=1;
                else if(tokens[2]=="Pack5")   item_drop_type=2;
                else if(tokens[2]=="Proj")    item_drop_type=3;
                else if(tokens[2]=="Bigpack") item_drop_type=4;
                else std::cout<<"Unknown item drop type: "<<tokens[2]<<" in line"<<line<<'\n';
                items.item_drop_index.push_back(item_drop_type);
                items.names[item.id]=tokens[3];
            }else{
                items.names[item.id]=tokens[2];
            }
            item_ptr->push_back(std::move(item));
        }
    }
    return true;
}
bool load_weapon_data(GameItems& items){
    std::filesystem::path path = paths::items/"WeaponData.txt";
    std::ifstream file;
    if(!open_file(file,path))return false;
    std::string line;
    // items.weapon_specs.reserve(512);
    items.gear_specs.reserve(2048);
    while(std::getline(file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;//Comment
        auto tokens = parse::split(line,',');
        if(tokens.size()!=8){
            std::cout<<"Bad line in "<<path<<'\n'<<line<<'\n';
            continue;
        }
        //WeaponSpecs specs;
        GearSpecs specs;
        parse::read_var(tokens[0],specs.id);
        bool found = false;//Make sure we only store weapons we allow in the rando
        for(size_t i =0;i<items.weapons.size();i++){
            if(items.weapons[i].id==specs.id){
                found=true;
                break;
            }
        }
        if(!found)continue;
        auto weapon_type = weapon_type_from_id(specs.id);
        if(weapon_type==WeaponType::Chime||weapon_type==WeaponType::Flames||weapon_type==WeaponType::Staff){
            specs.gear_type=GearPiece::Catalyst;
        }else if(weapon_type==WeaponType::Shield){
            specs.gear_type=GearPiece::Shield;
        }else if(weapon_type==WeaponType::Bow||weapon_type==WeaponType::Crossbow||weapon_type==WeaponType::Greatbow){
            specs.gear_type=GearPiece::Bows;
        }else{
            specs.gear_type=GearPiece::Weapon;
        }

        parse::read_var(tokens[1],specs.strength);
        parse::read_var(tokens[2],specs.dexterity);
        parse::read_var(tokens[3],specs.intelligence);
        parse::read_var(tokens[4],specs.faith);
        parse::read_var(tokens[5],specs.weight);
             if(tokens[6]=="All")             specs.infusion_type=InfusionType::All;
        else if(tokens[6]=="Basic")           specs.infusion_type=InfusionType::Basic;
        else if(tokens[6]=="Lighting dark")   specs.infusion_type=InfusionType::DarkLighting;
        else if(tokens[6]=="Magic dark")      specs.infusion_type=InfusionType::DarkMagic;
        else if(tokens[6]=="Elemental")       specs.infusion_type=InfusionType::Elemental;
        else if(tokens[6]=="No bleed")        specs.infusion_type=InfusionType::NoBleed;
        else if(tokens[6]=="No elemental")    specs.infusion_type=InfusionType::NoElemental;
        else if(tokens[6]=="None")            specs.infusion_type=InfusionType::None;
        else if(tokens[6]=="No poison bleed") specs.infusion_type=InfusionType::NoPoisonBleed;
        else{
            std::cout<<"Infusion type not recognized: "<<tokens[6]<<" in line "<<line<<'\n';
        }
        if(!parse::read_var(tokens[7],specs.max_reinforce_lvl)){
            std::cout<<"Reinforcement lvl not valid: "<<tokens[7]<<" in line "<<line<<'\n';
        }
        items.gear_specs.push_back(specs);
    }
    return true;
}
bool load_armor_data(GameItems& items){
    std::filesystem::path path = paths::items/"ArmorData.txt";
    std::ifstream file;
    if(!open_file(file,path))return false;
    std::string line;
    // items.armor_specs.reserve(512);
    while(std::getline(file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;//Comment
        auto tokens = parse::split(line,',');
        if(tokens.size()!=6){
            std::cout<<"Bad line in "<<path<<'\n'<<line<<'\n';
            continue;
        }
        GearSpecs specs;
        parse::read_var(tokens[0],specs.id);
        //For some reason armor item id and armor id have a difference of 100000000
        //And the item id is the one used for classes
        specs.id+=10000000;
        bool found = false;//Make sure we only store armor we allow in the rando
        for(size_t i =0;i<items.armor.size();i++){
            if(items.armor[i].id==specs.id){
                found=true;
                break;
            }
        }
        if(!found)continue;
        if(specs.id%10==0)specs.gear_type=GearPiece::Head;
        else if(specs.id%10==1)specs.gear_type=GearPiece::Chest;
        else if(specs.id%10==2)specs.gear_type=GearPiece::Arms;
        else specs.gear_type=GearPiece::Legs;
        parse::read_var(tokens[1],specs.strength);
        parse::read_var(tokens[2],specs.dexterity);
        parse::read_var(tokens[3],specs.intelligence);
        parse::read_var(tokens[4],specs.faith);
        parse::read_var(tokens[5],specs.weight);
        items.gear_specs.push_back(specs);
    }
    return true;
}
bool load_spell_data(GameItems& items){
    std::filesystem::path path = paths::items/"SpellData.txt";
    std::ifstream file;
    if(!open_file(file,path))return false;
    std::string line;
    // items.spell_specs.reserve(512);
    while(std::getline(file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;//Comment
        auto tokens = parse::split(line,',');
        if(tokens.size()!=5){
            std::cout<<"Bad line in "<<path<<'\n'<<line<<'\n';
            continue;
        }
        // SpellSpecs specs;
        GearSpecs specs;
        parse::read_var(tokens[0],specs.id);
        if(specs.id>35310000||specs.id<31010000)continue;
        specs.gear_type=GearPiece::Spell;
        // parse::read_var(tokens[1],specs.spell_type);
        parse::read_var(tokens[2],specs.intelligence);
        parse::read_var(tokens[3],specs.faith);
        // parse::read_var(tokens[4],specs.slots);
        items.gear_specs.push_back(specs);
    }
    return true;
}
bool load_ring_data(GameItems& items){
    std::filesystem::path path = paths::items/"RingData.txt";
    std::ifstream file;
    if(!open_file(file,path))return false;
    std::string line;
    // items.spell_specs.reserve(512);
    while(std::getline(file,line)){
        if(line.empty())continue;
        if(line.substr(0,2)=="//")continue;//Comment
        auto tokens = parse::split(line,',');
        if(tokens.size()!=2){
            std::cout<<"Bad line in "<<path<<'\n'<<line<<'\n';
            continue;
        }
        // SpellSpecs specs;
        GearSpecs specs;
        parse::read_var(tokens[0],specs.id);
        if(specs.id<10000)continue;//Remove bad rings
        specs.gear_type=GearPiece::Ring;
        parse::read_var(tokens[1],specs.weight);
        items.gear_specs.push_back(specs);
    }
    return true;
}

bool load_classes(std::vector<ClassSpecs>& classes){
    classes.reserve(8);
    classes.push_back({20,"Warrior",7,6,5,6,15,11,5,5,5});
    classes.push_back({30,"Knight",12,6,4,7,11,8,3,6,9});
    classes.push_back({50,"Bandit",9,7,2,11,9,14,1,8,3});
    classes.push_back({70,"Cleric",10,3,10,8,11,5,4,12,4});
    classes.push_back({80,"Sorcerer",5,6,12,5,3,7,14,4,8});
    classes.push_back({90,"Explorer",7,6,7,9,6,6,5,5,12});
    classes.push_back({100,"Swordsman",4,8,6,4,9,16,7,5,6});
    classes.push_back({110,"Deprived",6,6,6,6,6,6,6,6,6});
    return true;
}
bool load_shop_items(Shops& shop){
    std::filesystem::path path = paths::lots/"ShopsData.txt";
    std::ifstream file;
    std::string line;
    bool straid=false,ornifex=false,common=false,remove=false;
    if(!open_file(file,path))return false;
    while(std::getline(file,line)){
        if(line.substr(0,2)=="//")continue;//Comment
        if(line.front()=='#'){
            straid=ornifex=common=remove=false;
            if(line=="#COMMON") common=true;
            else if(line=="#STRAID_TRADE") straid=true;
            else if(line=="#ORNIFEX_TRADE") ornifex=true;
            else if(line=="#REMOVE"||line=="#NGPLUS") remove=true;
            else if(line=="#FREE_ORNIFEX") common=false;//Do nothing
            else{
                std::cout<<"Unknown item category: "<<line<<'\n';
            }
        }else{
            auto tokens = parse::split(line,',');
            if(tokens.size()!=2){
                std::cout<<"Bad line: |"<<line<<"| in "<<path<<'\n';
                continue;
            }
            ShopSlot shop_slot;
            parse::read_var(tokens[0],shop_slot.lot_id);
            shop.original_items[shop_slot.lot_id]=tokens[1];
            if(straid) shop.straid_trades.push_back(std::move(shop_slot));
            else if(ornifex) shop.ornifex_trades.push_back(std::move(shop_slot));
            else if(common) shop.common.push_back(std::move(shop_slot));
            else if(remove) shop.to_remove.push_back(shop_slot.lot_id);
        }
    }
    return true;
}
bool load_gear_data(GameItems& items){
    if(!load_weapon_data(items))return false;
    if(!load_armor_data(items))return false;
    if(!load_spell_data(items))return false;
    if(!load_ring_data(items))return false;
    return true;
}

std::string generate_config_file(ItemRandoConfig& config){
    std::stringstream ss;
    ss<<"#VERSION 0\n";
    ss<<"#SEED "<<config.seed<<'\n';
    ss<<"#WEIGHT_LIMIT "<<config.weight_limit<<'\n';
    ss<<"#UNLOCK_SHOP "<<config.unlock_common_shop<<'\n';
    ss<<"#UNLOCK_STRAID "<<config.unlock_straid_trades<<'\n';
    ss<<"#UNLOCK_ORNIFEX "<<config.unlock_ornifex_trades<<'\n';
    ss<<"#INFINITE_SHOP "<<config.infinite_shop_slots<<'\n';
    ss<<"#MELENTIA_GEMS "<<config.melentia_lifegems<<'\n';
    ss<<"#INFUSE_WEAPONS "<<config.infuse_weapons<<'\n';
    ss<<"#RANDO_CLASSES "<<config.randomize_classes<<'\n';
    ss<<"#RANDO_GIFTS "<<config.randomize_gifts<<'\n';
    ss<<"#CLASS_TWOHAND "<<config.allow_twohanding<<'\n';
    ss<<"#CLASS_UNUSABLE "<<config.allow_unusable<<'\n';
    ss<<"#CLASS_SHIELDWEAPON "<<config.allow_shield_weapon<<'\n';
    ss<<"#CLASS_CATALYSTS "<<config.allow_catalysts<<'\n';
    ss<<"#CLASS_BOWS "<<config.allow_bows<<'\n';
    ss<<"#EARLY_BLACKSMITH "<<config.early_blacksmith<<'\n';
    ss<<"#CHEATSHEET "<<config.write_cheatsheet<<'\n';
    ss<<"#RANDO_KEYS "<<config.randomize_key_items<<'\n';
    return ss.str();
}

void write_config_file(ItemRandoConfig& config){
    std::ofstream file(paths::configfile);
    if(!file){
        std::cout<<"Failed to write item configuration file, can't open file:"<<paths::configfile<<"\n";
    }else{
        file<<generate_config_file(config);
        file.close();
    }
}
void read_config_file(ItemRandoConfig& config){
    config.seed=rng::integer<u64>(0u,999999999999999u,rng::m_gen);
    config.weight_limit=70u;
    config.unlock_common_shop=false;
    config.unlock_straid_trades=true;
    config.unlock_ornifex_trades=true;
    config.infinite_shop_slots=true;
    config.melentia_lifegems=false;
    config.infuse_weapons=true;
    config.randomize_classes=true;
    config.randomize_gifts=true;
    config.allow_twohanding=true;
    config.allow_unusable=false;
    config.allow_shield_weapon=false;
    config.allow_catalysts=true;
    config.allow_bows=true;
    config.full_rando_classes=true;
    config.early_blacksmith=false;
    config.write_cheatsheet=true;
    config.randomize_key_items=true;
    config.valid=false;

    std::ifstream file;
    if(!open_file(file,paths::configfile)){
        std::cout<<"No item rando configuration file found, using default settings\n";
        return;
    }
    std::string line;
    while(std::getline(file,line)){
        if(line.empty())continue;
        std::string_view view = line;
        if(view.substr(0,2)=="//")continue;
        if(view.front()!='#')continue;
        auto tokens = parse::split(line,' ');
        if(tokens.size()<2)continue;
        auto& command = tokens[0];
        uint64_t value1=0;
        uint64_t version=0;
        if(!parse::read_var(tokens[1],value1)){
            std::cerr<<"Error reading item config file line: "<<line<<'\n';
            continue;
        }
             if(command=="#VERSION")           version=value1;
        else if(command=="#SEED")              config.seed=value1;
        else if(command=="#WEIGHT_LIMIT")      config.weight_limit=value1;
        else if(command=="#UNLOCK_SHOP")       config.unlock_common_shop=value1;
        else if(command=="#UNLOCK_STRAID")     config.unlock_straid_trades=value1;
        else if(command=="#UNLOCK_ORNIFEX")    config.unlock_ornifex_trades=value1;
        else if(command=="#INFINITE_SHOP")     config.infinite_shop_slots=value1;
        else if(command=="#MELENTIA_GEMS")     config.melentia_lifegems=value1;
        else if(command=="#INFUSE_WEAPONS")    config.infuse_weapons=value1;
        else if(command=="#RANDO_CLASSES")     config.randomize_classes=value1;
        else if(command=="#RANDO_GIFTS")       config.randomize_gifts=value1;
        else if(command=="#CLASS_TWOHAND")     config.allow_twohanding=value1;
        else if(command=="#CLASS_UNUSABLE")    config.allow_unusable=value1;
        else if(command=="#CLASS_SHIELDWEAPON")config.allow_shield_weapon=value1;
        else if(command=="#CLASS_CATALYSTS")   config.allow_catalysts=value1;
        else if(command=="#CLASS_BOWS")        config.allow_bows=value1;
        else if(command=="#EARLY_BLACKSMITH")  config.early_blacksmith=value1;
        else if(command=="#CHEATSHEET")        config.write_cheatsheet=value1;
        else if(command=="#RANDO_KEYS")        config.randomize_key_items=value1;
        else{
            std::cout<<"Unknow option in "<<paths::configfile<<" "<<command<<'\n';
        }
    }
}

bool load_randomizer_data(IRData& irdata){
    std::cout<<"Loading item randomizer data\n";
    Stopwatch clock;
    read_config_file(irdata.config);
    irdata.data = new ItemRandoData;
    auto& data = *irdata.data;
    if(!load_lots(data))             return false;
    if(!load_equivalents(data))      return false;
    if(!load_location_lots(data))    return false;
    if(!load_key_lots(data))         return false;
    if(!load_items(data.items))      return false;
    if(!load_shop_items(data.shops)) return false;
    if(!load_gear_data(data.items))  return false;
    if(!load_classes(data.classes))  return false;
    
    irdata.config.valid=true;
    auto t = clock.passed();
    std::cout<<"Successful item rando load in: "<<t/1000<<"ms\n";
    return true;
}


//Filling auxiliary functions
void fill_with_items(std::vector<Item>& item_vector,std::vector<Item*>& out_vector){
    for(auto& a:item_vector){
        for(s32 i = 0;i<a.quantity;i++){
            out_vector.push_back(&a);
        }
    }
}
void place_enemy_items(ItemRandoData& data,std::vector<s32>& enemies,std::unordered_multimap<s32,s32>& enemy_id_lots,std::vector<Item*>& items){
    for(const auto& id:enemies){
        if(items.empty())break;
        auto element = items.back();
        items.pop_back();
        LotData lot;
        lot.amount=1;
        lot.infinite=false;
        lot.chance=5.f;
        lot.item_id=element->id;
        auto range = enemy_id_lots.equal_range(id);
        for (auto it = range.first; it != range.second; ++it){
            lot.lot_id=it->second;
            data.enemy_drop_lots.push_back(lot);
        }
    }
}
void place_item_kind(std::vector<Item>& item,std::vector<std::pair<Item*,s32>>& items,float percentage,std::mt19937_64& gen){
    std::vector<Item*> count;
    count.reserve(item.size());
    for(auto& a:item){
        if(a.quantity>0)count.push_back(&a);
    }
    std::shuffle(count.begin(),count.end(),gen);
    size_t place_number = static_cast<size_t>(count.size()*percentage);
    for(size_t i =0;i<place_number;i++){
        items.push_back({count[i],1});
    }
}

template<typename T>
void vector_remove_index(std::vector<T>& vector,size_t index){
    if(index>=vector.size()) return;
    vector.erase(vector.begin()+index);
}

bool remove_lot(std::vector<s32>& lots,s32 lot_to_remove){
    for(size_t i = 0;i<lots.size();i++){
        if(lot_to_remove==lots[i]){
            vector_remove_index(lots,i);
            return true;
        }
    }
    return false;
}
//Where the magic happens
void remove_key_items(ItemRandoData& data,ItemRandoConfig& config){
    auto& keys = data.items.keys;
    //Remove items from the item pool so they dont get placed again later
    s32 last_key_removed = 0;
    for(const auto& key_lot:data.key_lots){
        auto key = key_lot.item_id;
        if(key==last_key_removed) continue;//Avoid removing same item multiple times
        bool removed=false;
        for(size_t i = 0;i<keys.size();i++){
            if(key==keys[i].id){
                //std::cout<<"removed: "<<key<<" "<<keys[i].quantity<<'\n';
                vector_remove_index(keys,i);
                removed=true;
                last_key_removed=key;
                break;
            }
        }
        if(!removed){
            std::cout<<key<<" item not removed??\n";
        }
        // else{
        //     std::cout<<"removed item: "<<key<<" "<<key_lot.id<<'\n';
        // }
    }
    for(const auto& lot:data.key_lots){
        bool removed=false;
        if(lot.type==LotType::Shop){
            for(size_t i =0;i<data.shops.common.size();i++){
                if(lot.id==data.shops.common[i].lot_id){
                    vector_remove_index(data.shops.common,i);
                    removed=true;
                    break;
                }
            }
        }else if(lot.type==LotType::Other){
            if(vector_find_swap_pop(data.unmissable_lots,lot.id))    removed=true;
            else if(vector_find_swap_pop(data.missable_lots,lot.id)) removed=true;
            if(removed){
                LotData lot_data;
                lot_data.amount  =lot.amount;
                lot_data.chance  =lot.chance;
                lot_data.lot_id  =lot.id;
                lot_data.item_id =lot.item_id;
                lot_data.infinite=1u;
                data.lots.push_back(lot_data);
            }
        }else if(lot.type==LotType::Char){
            if(vector_find_swap_pop(data.safe_chr_drop,lot.id))   removed=true;
            else if(vector_find_swap_pop(data.enemy_lots,lot.id)) removed=true;
            else if(vector_find_swap_pop(data.chr_remove,lot.id)) removed=true;
            if(removed){
                LotData lot_data;
                lot_data.amount  =lot.amount;
                lot_data.chance  =lot.chance;
                lot_data.lot_id  =lot.id;
                lot_data.item_id =lot.item_id;
                lot_data.infinite=1u;
                data.enemy_drop_lots.push_back(lot_data);
            }
        }
    }
}
bool place_graph_key_items(ItemRandoData& lots,ItemRandoConfig& config){
    solver::Graph graph;
    if(!solver::parse_from_file(paths::lots/"Drangleic.txt",graph)){
        std::cout<<"Failed to parse graph file\n";
        return false;
    }
    std::mt19937_64 generator(config.seed);
    auto state = solver::generate_state(graph);
    std::vector<size_t> key_deck;
    for(size_t i = 1;i<graph.keys.size();i++){//Set static keys
        auto key = graph.keys[i].name;
        if(key=="Kill Fume Knight"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"Active Brume Tower"));
        }else if(key=="Kill Lost Sinner"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"Sinner Rise"));
        }else if(key=="Kill Iron King"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"Iron Keep"));
        }else if(key=="Kill The Rotten"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"The Gutter"));    
        }else if(key=="Kill Freja"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"Tseldora"));    
        }else if(key=="Kill Giant Lord"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"Giant Lord Memory"));    
        }else if(key=="Kill Vendrick"){
            state.placements[i].node.push_back(solver::room_by_name(graph,"Undead Crypt"));
        }else{
            key_deck.push_back(i);
        }
    }
    //All of these are always accesible so they are always valid nodes
    std::vector<size_t> early_blacksmith_loc={
        solver::room_by_name(graph,"Things Betwixt"),
        solver::room_by_name(graph,"Majula"),
        solver::room_by_name(graph,"Heides Tower"),
        solver::room_by_name(graph,"Unseen path to Heide"),
        solver::room_by_name(graph,"No Mans Warf"),
        solver::room_by_name(graph,"Forest Fallen Giants")
    };
    std::shuffle(key_deck.begin(),key_deck.end(),generator);
    for(const auto& key_id:key_deck){
        std::vector<size_t> valid_nodes;
        if(config.early_blacksmith&&graph.keys[key_id].name=="Lenigrast's Key"){
            valid_nodes = early_blacksmith_loc;
        }else{
            valid_nodes = get_valid_nodes(graph,state,key_id);
        }
        if(valid_nodes.size()==0){
            std::cout<<"Can't place, no valid nodes\n";
            return false;
        }
        auto name = graph.keys[key_id].name;
        s32 item_id = 0;
        for(const auto& [entry_id,entry_name]:lots.items.names){
            if(entry_name==name){
                item_id = entry_id;
                for(size_t j = 0;j<lots.items.keys.size();j++){
                    if(lots.items.keys[j].id==item_id){
                        lots.items.keys.erase(lots.items.keys.begin()+j);
                        break;
                    }
                }
                break;
            }
        }
        if(!item_id){
            std::cout<<"Cant find key: "<<name<<'\n';
            return false;
        }
        s32 lot_id = 0;
        size_t placing_node = 0;
        while(true){
            placing_node = rng::element(valid_nodes,generator);
            auto& room_lots = lots.location_lots[graph.rooms[placing_node].name];
            if(!room_lots.empty()){
                lot_id = rng::element(room_lots,generator);
                if(!vector_find_swap_pop(room_lots,lot_id)){
                    std::cout<<"Waht?\n";
                }
                if(!vector_find_swap_pop(lots.unmissable_lots,lot_id)){
                    std::cout<<"Cant remove item, not found: "<<lot_id<<'\n';
                }
                break;
            }else{
                vector_find_swap_pop(valid_nodes,placing_node);
                if(valid_nodes.empty()){
                    std::cout<<"No space for item: "<<name<<'\n';
                    return false;
                }
            }
        }
        LotData ld;
        ld.item_id =item_id;
        if(ld.item_id==60537000){//Branches
            ld.amount=25u;
        }else if(ld.item_id==60536000){//Pharros
            ld.amount=50u;
        }else{
            ld.amount=1u;
        }

        ld.chance  =100.f;
        ld.lot_id  =lot_id;
        ld.infinite=1u;
        lots.lots.push_back(std::move(ld));
        state.placements[key_id].node.push_back((uint32_t)placing_node);
    }
    //solver::print_solution(graph,state);
    if(!check_solution(graph,state)){
        std::cout<<"ERROR: INVALID SOLUTION\n";
        return false;
    }
    return true;

}
bool place_rest_keys(ItemRandoData& lots,ItemRandoConfig& config){
    std::mt19937_64 generator(config.seed);
    for(const auto& key:lots.items.keys){
        for(s32 i =0;i<key.quantity;i++){
            if(lots.unmissable_lots.empty()){
                std::cout<<"No more unmissable lots to place keys\n";
                return false;
            }
            auto lot_id = rng::element(lots.unmissable_lots,generator);
            LotData ld;
            ld.amount  =1u;
            ld.chance  =100.f;
            ld.lot_id  =lot_id;
            ld.item_id =key.id;
            ld.infinite=1u;
            lots.lots.push_back(std::move(ld));
            vector_find_swap_pop(lots.unmissable_lots,lot_id);
        }
    }
    return true;
}
void place_shop_items(ItemRandoData& lots,ItemRandoConfig& config){
    auto& shops = lots.shops; 
    std::mt19937_64 generator(config.seed);
    // std::cout<<"Straid: "<<shops.straid_trades.size()<<" Ornifex:"<<shops.ornifex_trades.size()<<" Common: "<<shops.common.size()<<'\n';
    // std::cout<<"Weapons:"<<lots.items.weapons.size()<<'\n';
    // std::cout<<"Rings:"<<lots.items.rings.size()<<'\n';
    // std::cout<<"Spells:"<<lots.items.spells.size()<<'\n';
    // std::cout<<"Armor:"<<lots.items.armor.size()<<'\n';
    std::vector<Item*> wars;//Weapon,armor,rings,spells
    fill_with_items(lots.items.weapons,wars);
    fill_with_items(lots.items.armor,wars);
    fill_with_items(lots.items.rings,wars);
    fill_with_items(lots.items.spells,wars);
    std::shuffle(wars.begin(),wars.end(),generator);
    for(auto& a:shops.straid_trades){
        if(wars.empty()) break;
        auto element = wars.back();
        a.item_id=element->id;
        element->quantity-=1;
        a.quantity=255u;
        a.infinite=true;
        a.price_mult=rng::real(0.5f,1.5f,generator);
        wars.pop_back();
    }    
    for(auto& a:shops.ornifex_trades){
        if(wars.empty()) break;
        auto element = wars.back();
        a.item_id=element->id;
        element->quantity-=1;
        a.quantity=255u;
        a.infinite=true;
        a.price_mult=rng::real(0.5f,1.5f,generator);
        wars.pop_back();
    }
    size_t original_size=shops.ornifex_trades.size();
    for(size_t i = 0;i<original_size;i++){
        auto& trade = shops.ornifex_trades[i];
        ShopSlot free_version = trade;
        free_version.price_mult=0.f;
        free_version.lot_id+=1000;//This is how they are arranged
        shops.ornifex_trades.push_back(free_version);
    }
    std::vector<size_t> shop_index(shops.common.size());
    for(size_t i = 0;i<shop_index.size();i++) shop_index[i]=i;
    std::shuffle(shop_index.begin(),shop_index.end(),generator);

    if(config.infinite_shop_slots){
        std::vector<s32> infinite_items={    
        60970000,60975000,60980000,60990000,61000000,61030000,60151000,60350000,60240000,60250000,60260000,60270000,60280000,60290000,60570000,60910000,60920000,60430000,60760000,60770000,60780000,60790000,60800000,60810000,60820000,60830000,60930000,60940000,60950000,60960000,60870000,60880000,60900000,60010000};
        for(const auto& item:infinite_items){
            if(shop_index.empty())break;        
            auto& slot = shops.common[shop_index.back()];
            shop_index.pop_back();
            slot.infinite=true;
            slot.quantity=255;
            slot.item_id=item;
            slot.price_mult = rng::real(0.5f,1.5f,generator);
        }
    }

    float split = 0.6f;
    size_t wars_split = static_cast<size_t>(split*shop_index.size());
    for(size_t i = 0;i<wars_split;i++){
        if(shop_index.empty()||wars.empty())break;        
        auto& slot = shops.common[shop_index.back()];
        shop_index.pop_back();
        auto element = wars.back();
        wars.pop_back();
        slot.item_id=element->id;
        element->quantity-=1;  
        slot.infinite=false;
        slot.quantity=1;
        slot.price_mult = rng::real(0.5f,1.5f,generator);
    }
    struct ShopConsumable{s32 id;s32 min,max;float price_min,price_max;};
    std::vector<ShopConsumable> consumables{
        {50880000,2,5,0.5,1.5},{50885000,2,5,0.5,1.5},{51010000,1,3,0.5,1.5},
        {51020000,1,3,0.5,1.5},{60010000,5,10,0.5,1.5},{60020000,2,5,0.5,1.5},
        {60030000,1,3,0.5,1.5},{60035000,1,3,0.5,1.5},{60036000,1,3,0.5,1.5},
        {60040000,2,5,0.5,1.5},{60050000,2,5,0.5,1.5},{60060000,2,5,0.5,1.5},
        {60070000,5,10,0.5,1.5},{60090000,5,10,0.5,1.5},{60100000,5,10,0.5,1.5},
        {60105000,1,3,0.5,1.5},{60110000,2,10,0.5,1.5},{60120000,2,10,0.5,1.5},
        {60151000,2,10,0.5,1.5},{60160000,2,5,0.5,1.5},{60170000,2,5,0.5,1.5},
        {60180000,2,5,0.5,1.5},{60190000,2,5,0.5,1.5},{60200000,2,5,0.5,1.5},
        {60210000,2,5,0.5,1.5},{60230000,2,5,0.5,1.5},{60235000,2,5,0.5,1.5},
        {60236000,2,5,0.5,1.5},{60237000,2,5,0.5,1.5},{60238000,2,5,0.5,1.5},
        {60239000,1,3,0.5,1.5},{60240000,5,10,0.5,1.5},{60250000,2,10,0.5,1.5},
        {60260000,2,10,0.5,1.5},{60270000,2,10,0.5,1.5},{60280000,2,10,0.5,1.5},
        {60290000,2,10,0.5,1.5},{60310000,2,10,0.5,1.5},{60320000,2,10,0.5,1.5},
        {60350000,2,10,0.5,1.5},{60370000,5,10,0.5,1.5},{60410000,1,5,0.5,1.5},
        {60430000,3,5,0.5,1.5},{60450000,3,20,0.5,1.5},{60511000,1,2,0.5,1.5},
        {60530000,3,20,0.5,1.5},{60531000,3,20,0.5,1.5},{60540000,5,20,0.5,1.5},
        {60550000,5,20,0.5,1.5},{60560000,5,20,0.5,1.5},{60570000,5,20,0.5,1.5},
        {60575000,5,20,0.5,1.5},{60580000,5,20,0.5,1.5},{60590000,5,20,0.5,1.5},
        {60595000,5,20,0.5,1.5},{60600000,5,20,0.5,1.5},{60610000,5,20,0.5,1.5},
        {60620000,5,20,0.5,1.5},{60625000,1,1,0.1,0.5},{60630000,1,1,1.0,2.0},
        {60640000,1,1,1.0,4.0},{60650000,1,1,1.0,8.0},{60660000,1,1,1.0,10.0},
        {60670000,1,1,1.0,20.0},{60680000,1,1,1.0,30.0},{60690000,1,1,1.0,50.0},
        {60700000,1,1,1.0,80.0},{60710000,1,1,1.0,100.0},{60720000,1,1,1.0,200.0},
        {60760000,20,30,0.5,1.5},{60770000,20,30,0.5,1.5},{60780000,20,30,0.5,1.5},
        {60790000,20,30,0.5,1.5},{60800000,20,30,0.5,1.5},{60810000,20,30,0.5,1.5},
        {60820000,20,30,0.5,1.5},{60830000,20,30,0.5,1.5},{60850000,20,30,0.5,1.5},
        {60870000,20,30,0.5,1.5},{60880000,20,30,0.5,1.5},{60900000,20,30,0.5,1.5},
        {60910000,20,30,0.5,1.5},{60920000,20,30,0.5,1.5},{60930000,20,30,0.5,1.5},
        {60940000,20,30,0.5,1.5},{60950000,20,30,0.5,1.5},{60960000,20,30,0.5,1.5},
        {60970000,1,7,0.5,1.5},{60975000,1,5,0.5,1.5},{60980000,1,3,0.5,1.5},
        {60990000,1,1,0.5,1.5},{61000000,1,3,0.5,1.5},{61030000,1,3,0.5,1.5},
        {61060000,1,5,0.5,1.5},{61070000,1,5,0.5,1.5},{61080000,1,5,0.5,1.5},
        {61090000,1,5,0.5,1.5},{61100000,1,5,0.5,1.5},{61110000,1,5,0.5,1.5},
        {61130000,1,5,0.5,1.5},{61140000,1,5,0.5,1.5},{61150000,1,5,0.5,1.5},
        {61160000,1,5,0.5,1.5},{60527000,1,1,0.5,3.f}
    };

    for(auto index:shop_index){
        auto& slot = shops.common[index];
        auto& consumable = rng::element(consumables,generator);
        slot.price_mult = rng::real(0.f,1.f,generator)*(consumable.price_max-consumable.price_min)+consumable.price_min;
        slot.infinite=false;
        slot.quantity=static_cast<u8>(rng::integer(consumable.min,consumable.max,generator));
        slot.item_id=consumable.id;
    }
}
void place_dyna_tillo_items(ItemRandoData& lots,ItemRandoConfig& config){
    s32 initial_lot_id = 50000000;
    std::mt19937_64 generator(config.seed);
    std::vector<Item*> wars;//Weapon,armor,rings,spells
    fill_with_items(lots.items.weapons,wars);
    fill_with_items(lots.items.armor,wars);
    fill_with_items(lots.items.rings,wars);
    fill_with_items(lots.items.spells,wars);
    std::shuffle(wars.begin(),wars.end(),generator);

    //All 4 items have 4 drop categories
    //Different items use id differentiated by 100
    //Different categories use id differentiated by 1
    //All 4 categories use the same items but different chance
    for(s32 i = 0;i<4;i++){
        s32 items_ids[10];
        items_ids[0]=rng::element(lots.items.consumables,generator).id;
        items_ids[1]=rng::element(lots.items.consumables,generator).id;
        items_ids[2]=rng::element(lots.items.consumables,generator).id;
        items_ids[3]=rng::element(lots.items.consumables,generator).id;
        for(size_t j = 4;j<10;j++){
            if(wars.empty())break;
            auto ptr = rng::element(wars,generator);
            items_ids[j] = ptr->id;
            ptr->quantity-=1;
        }
        LotData lot;
        lot.amount=1;
        lot.infinite=true;
        for(size_t h =0;h<10;h++){
            lot.item_id=items_ids[h];
            for(s32 j = 0;j<400;j+=100){
                lot.chance=rng::real(0.5f,3.f,generator);
                lot.lot_id=initial_lot_id+i+j;
                lots.lots.push_back(lot);
            }
        }
    }
    //Rubbish after destroying a wooden chest
    LotData lot;
    lot.amount=1;
    lot.infinite=true;
    lot.chance=1.0f;
    lot.item_id=60510000;
    lot.lot_id=50001000;
    lots.lots.push_back(lot);
}
void place_items(ItemRandoData& data,ItemRandoConfig& config){
    auto& items = data.items;
    std::mt19937_64 generator(config.seed);
    //After placing all keys, there is no need to differentiate
    for(const auto& unmissable:data.unmissable_lots){
        data.missable_lots.push_back(unmissable);
    }
    //Use pointer to real item so we can keep track of which items have already been placed
    std::vector<std::pair<Item*,s32>> cc;
    cc.reserve(3000);//Ballpark
    for(size_t i=0;i<items.consumables.size();i++){
        auto& mitem = items.consumables[i];
        auto& possible_quantities = items.item_drop_quantity[items.item_drop_index[i]];
        s32 remaining = mitem.quantity;
        while(remaining>0){
            s32 quantity = rng::element(possible_quantities,generator);
            quantity = std::min(remaining,quantity);
            remaining-=quantity;
            cc.push_back({&mitem,quantity});
        }
    }
    //TODO CHANGE FOR SPECIAL DROPS
    for(auto& a:items.special_lots){
        cc.push_back({&a,a.quantity});
    }
    place_item_kind(items.rings,cc,0.9f,generator);
    place_item_kind(items.spells,cc,0.9f,generator);
    place_item_kind(items.armor,cc,0.8f,generator);
    place_item_kind(items.weapons,cc,0.7f,generator);

    //@CAUTION NEED TO MAKE SURE THERE ARE ENOUGH ITEMS TO PUT IN EVERY LOT
    auto lots_to_fill = data.missable_lots.size()+data.safe_chr_drop.size();
    while(lots_to_fill>cc.size()){
        auto& random_item = rng::element(items.consumables,generator);
        cc.push_back({&random_item,1});
    }

    std::shuffle(cc.begin(),cc.end(),generator);
    std::shuffle(data.missable_lots.begin(),data.missable_lots.end(),generator);
    std::shuffle(data.safe_chr_drop.begin(),data.safe_chr_drop.end(),generator);
    while(!cc.empty()){
        for(size_t i = 0;i<data.missable_lots.size();i++){
            if(cc.empty()) break;
            auto element = cc.back();
            cc.pop_back();
            element.first->quantity-=element.second;
            LotData lot;
            lot.amount=static_cast<u8>(element.second);
            lot.chance=100.f;
            lot.infinite=true;
            lot.item_id=element.first->id;
            lot.lot_id=data.missable_lots[i];
            data.lots.push_back(std::move(lot));
        }
        for(size_t i = 0;i<data.safe_chr_drop.size();i++){
            if(cc.empty()) break;
            auto element = cc.back();
            cc.pop_back();
            element.first->quantity-=element.second;
            LotData lot;
            lot.amount=static_cast<u8>(element.second);
            lot.chance=100.f;
            lot.infinite=false;
            lot.item_id=element.first->id;
            lot.lot_id=data.safe_chr_drop[i];
            data.chr_lots.push_back(std::move(lot));
        }
    }
}
void place_enemy_drops(ItemRandoData& data,ItemRandoConfig& config){
    std::mt19937_64 generator(config.seed);
    //Kind of a mess
    std::vector<Item*> rings;
    fill_with_items(data.items.rings,rings);
    std::shuffle(rings.begin(),rings.end(),generator);
    std::vector<Item*> spells;
    fill_with_items(data.items.spells,spells);
    std::shuffle(spells.begin(),spells.end(),generator);
    std::vector<Item*> armor;
    fill_with_items(data.items.armor,armor);
    std::shuffle(armor.begin(),armor.end(),generator);
    std::vector<Item*> weapons;
    fill_with_items(data.items.weapons,weapons);
    std::shuffle(weapons.begin(),weapons.end(),generator);

    std::vector<s32> enemies;
    std::unordered_multimap<s32,s32> enemy_id_lots;
    enemies.reserve(data.enemy_names.size());
    for(const auto& [id,name]:data.enemy_names){
        enemies.push_back(id);
        for(const auto& lot_id:data.enemy_lots){
            s32 enemy_id = lot_id/10000;
            if(id==enemy_id){
                enemy_id_lots.insert({id,lot_id});
            }
        }
    }
    //Place armaments
    place_enemy_items(data,enemies,enemy_id_lots,rings);
    place_enemy_items(data,enemies,enemy_id_lots,spells);
    place_enemy_items(data,enemies,enemy_id_lots,armor);
    place_enemy_items(data,enemies,enemy_id_lots,weapons);
    //Place consumables
    for(const auto& id:enemies){
        if(rng::integer(0,99,generator)<40) continue;
        auto element = rng::element(data.items.consumables,generator);        
        LotData lot;
        lot.amount=1;
        lot.infinite=true;
        lot.chance=10.f;
        lot.item_id=element.id;
        auto range = enemy_id_lots.equal_range(id);
        for (auto it = range.first; it != range.second; ++it){
            lot.lot_id=it->second;
            data.enemy_drop_lots.push_back(lot);
        }
    }
}
void randomize_weapon_infusion(ItemRandoData& data,ItemRandoConfig& config){
    const std::vector<Infusion> dark_lightin{Infusion::Dark,Infusion::Lightning};
    const std::vector<Infusion> dark_magic{Infusion::Dark,Infusion::Magic};
    const std::vector<Infusion> elemental{Infusion::Bleed,Infusion::Dark,Infusion::Fire,Infusion::Lightning,Infusion::Magic,Infusion::Poison};
    const std::vector<Infusion> all{Infusion::Bleed,Infusion::Dark,Infusion::Enchanted,Infusion::Fire,Infusion::Lightning,Infusion::Magic,Infusion::Mundane,Infusion::Poison,Infusion::Raw};
    const std::vector<Infusion> no_bleed{Infusion::Dark,Infusion::Enchanted,Infusion::Fire,Infusion::Lightning,Infusion::Magic,Infusion::Mundane,Infusion::Poison,Infusion::Raw};
    const std::vector<Infusion> no_poison_bleed{Infusion::Dark,Infusion::Enchanted,Infusion::Fire,Infusion::Lightning,Infusion::Magic,Infusion::Mundane,Infusion::Raw};
    const std::vector<Infusion> no_elemental{Infusion::Enchanted,Infusion::Mundane,Infusion::Raw};
    std::mt19937_64 generator(config.seed);
    auto change_weapon=[&](LotData& lot){
        if(lot.item_id<1000000||lot.item_id>11850000) return;
        s32 weapon_id = lot.item_id;
        size_t index = SIZE_MAX;
        for(size_t i = 0;i<data.items.gear_specs.size();i++){
            if(weapon_id==data.items.gear_specs[i].id){
                index=i;
                break;
            }
        }
        if(index==SIZE_MAX) return;
        auto& weapon_specs = data.items.gear_specs[index];
        auto roll = rng::integer(0,999,generator);
        if(weapon_specs.max_reinforce_lvl>5){//Weapons that go to +10
                 if(roll>990) lot.reinforcement=4;
            else if(roll>950) lot.reinforcement=3;
            else if(roll>900) lot.reinforcement=2;
            else if(roll>800) lot.reinforcement=1;
        }else if(weapon_specs.max_reinforce_lvl>0){//Weapons that go to +5
                 if(roll>998) lot.reinforcement=4;
            else if(roll>990) lot.reinforcement=3;
            else if(roll>950) lot.reinforcement=2;
            else if(roll>900) lot.reinforcement=1;
        }
        auto infusion_roll = rng::integer(0,99,generator);
        if(infusion_roll>89){
            switch(weapon_specs.infusion_type){
                case InfusionType::All:          lot.infusion= rng::element(all,generator);break;
                case InfusionType::NoBleed:      lot.infusion= rng::element(no_bleed,generator);break;
                case InfusionType::Elemental:    lot.infusion= rng::element(elemental,generator);break;
                case InfusionType::DarkMagic:    lot.infusion= rng::element(dark_magic,generator);break;
                case InfusionType::NoElemental:  lot.infusion= rng::element(no_elemental,generator);break;
                case InfusionType::DarkLighting: lot.infusion= rng::element(dark_lightin,generator);break;
                case InfusionType::NoPoisonBleed:lot.infusion= rng::element(no_poison_bleed,generator);break;
                default: lot.infusion = Infusion::None;
            }
        }
    };
    for(auto& lot:data.lots) change_weapon(lot);
    for(auto& lot:data.chr_lots) change_weapon(lot);
    
}
void randomize_classes(ItemRandoData& data,ItemRandoConfig& config){
    auto& classes = data.classes;
    auto& gear = data.items.gear_specs;
    //Remove King's Ring from item pool
    //40510000 -> King's Ring id
    for(size_t i = 0;i<gear.size();i++){
        if(gear[i].id==40510000){
            vector_remove_index(gear,i);
            break;
        }
    }
    enum class Equipment{Head,Chest,Hand,Feet,RHand,LHand,Spell,Ring};
    std::vector<Equipment> equipment{Equipment::Head,Equipment::Chest,Equipment::Hand,Equipment::Feet,Equipment::RHand,Equipment::LHand,Equipment::Spell,Equipment::Ring};
    std::mt19937_64 generator(config.seed);
    std::vector<size_t> valid_index;
    auto valid_gear=[&generator,&gear,&valid_index](GearPiece gear_type,ClassSpecs& specs,s32& gear_piece){
        valid_index.clear();
        float weight_allowed = specs.max_weight-specs.weight;
        for(size_t i =0;i<gear.size();i++){
            auto& piece = gear[i];
            bool w   = piece.weight<weight_allowed;
            bool str = specs.str>=piece.strength;
            bool dex = specs.dex>=piece.dexterity;
            bool fth = specs.fth>=piece.faith;
            bool intll = specs.intll>=piece.intelligence;
            bool valid_piece = gear_type&piece.gear_type; 
            if(valid_piece&&str&&dex&&intll&&fth&&w) valid_index.push_back(i);
        }
        if(!valid_index.empty()){
            auto& piece = gear[rng::element(valid_index,generator)];
            gear_piece=piece.id;
            specs.weight+=piece.weight;
        }
    };
    //@WARNING There is a possibility that you don't get any weapon if the prior equipment is too heavy
    if(config.full_rando_classes){
        for(auto& mclass:classes){
            std::shuffle(equipment.begin(),equipment.end(),generator);
            auto specs = mclass;
            if(config.allow_unusable){
                specs.str=99;
                specs.dex=99;
                specs.fth=99;
                specs.intll=99;
            }
            specs.weight=0.f;
            specs.max_weight=vitality_to_equipment_load(mclass.vit);
            specs.max_weight*=(float)config.weight_limit/100.f;
            for(const auto& e:equipment){
                if(e==Equipment::Head){
                    valid_gear(GearPiece::Head,specs,mclass.gear.head);
                }else if(e==Equipment::Chest){
                    valid_gear(GearPiece::Chest,specs,mclass.gear.chest);
                }else if(e==Equipment::Hand){
                    valid_gear(GearPiece::Arms,specs,mclass.gear.arms);
                }else if(e==Equipment::Feet){
                    valid_gear(GearPiece::Legs,specs,mclass.gear.legs);
                }else if(e==Equipment::RHand){
                    if(config.allow_twohanding) specs.str*=2;
                    
                    if(config.allow_shield_weapon&&config.allow_bows){
                        valid_gear(GearPiece::RightShieldBow,specs,mclass.gear.right_hand);
                    }else if(config.allow_shield_weapon){
                        valid_gear(GearPiece::RightShield,specs,mclass.gear.right_hand);
                    }else if(config.allow_bows){
                        valid_gear(GearPiece::RightBow,specs,mclass.gear.right_hand);
                    }else{
                        valid_gear(GearPiece::Weapon,specs,mclass.gear.right_hand);
                    }
                    //Even if the function doesnt give any weapons you still get a valid type
                    auto weapon_type = weapon_type_from_id(mclass.gear.right_hand);
                    if(weapon_type==WeaponType::Bow) mclass.gear.extra_arrow = 60760000;//Wood arrow
                    else if(weapon_type==WeaponType::Crossbow) mclass.gear.extra_bolt = 60910000;//Wood bolt
                    else if(weapon_type==WeaponType::Greatbow) mclass.gear.extra_arrow = 60850000;//Iron greatarrow

                    if(config.allow_twohanding) specs.str/=2;
                }else if(e==Equipment::LHand){
                    if(rng::real(0.f,1.f,generator)>0.5f) continue;//No second weapon for you
                    if(config.allow_catalysts){
                        valid_gear(GearPiece::LeftCat,specs,mclass.gear.left_hand);
                    }else {
                        valid_gear(GearPiece::Left,specs,mclass.gear.left_hand);
                    }
                }else if(e==Equipment::Spell){
                    if(rng::real(0.f,1.f,generator)>0.5f) continue;//No spell for you
                    valid_gear(GearPiece::Spell,specs,mclass.gear.spell);
                }else if(e==Equipment::Ring){
                    if(rng::real(0.f,1.f,generator)>0.5f) continue;//No ring for you
                    valid_gear(GearPiece::Ring,specs,mclass.gear.ring);
                }
            }
        }
    }
}
void randomize_starting_gifts(ItemRandoData& data,ItemRandoConfig& config){
    using ItemPack = std::vector<Item>;
    std::vector<ItemPack> gifts={
        {{60010000,25}},//Lifegems
        {{60020000,10}},//Big lifegems
        {{60350000,30}},//Homeward bones
        {{60590000,50}},//Poison Throwing Knife
        {{60820000,75}},//Poison arrow
        {{60940000,75}},//Lighting bolt
        {{60575000,10},{60570000,30}},//Firebombs
        {{60720000,1}},//Soul of great hero
        {{60700000,1}},//Soul of brave warrior
        {{60660000,1}},//Soul of soldier
        {{60151000,20}},//Human effigy
        {{60160000,10},{60170000,10},{60180000,10}},//Resistance blurrs
        {{60240000,4},{60250000,4},{60260000,4}},//Magic, fire, lighting resins
        {{60270000,4},{60280000,4},{60290000,4}},//Dark, poison, bleed resins
        {{60525000,2}},//Estus flask shards
        {{60526000,1}},//Sublime bone dust
        {{50885000,5}},//Small smooth and silky stone
        {{50880000,5}},//Smooth and silky stone
        {{60511000,5}},//Petrified something
        {{60430000,10},{60420000,50}},//Flame buttefly and torch
        {{60239000,3}},//Brightbugs
        {{60410000,10},{60036000,10}},//Repair powder and dried roots
        {{51010000,10},{51020000,10}},//Simpleton and skeptic spice
        {{60040000,10},{60050000,10},{60060000,10}},//Magic use restore herbs
        {{60310000,30}},//Green blossom
        {{60070000,30}},//Poison moss
        {{60105000,5}},//Divine blessing
        {{60538000,3}},//Fire seed
        {{61030000,5}},//Petrified dragon bone
        {{61000000,5}},//Twingking titanite
        {{60990000,1}},//Titanite slab
        {{60980000,6}},//Titanite chunk
        {{60975000,6}},//Large titanite shard
        {{60970000,6}},//Titanite shard
        {{61060000,1},{61070000,1},{61080000,1},{61090000,1}},//Infusion stones
        {{61100000,1},{61110000,1},{61130000,1},{61140000,1}},//Infusion stones
        {{61150000,1},{61160000,1}}//Infusion stones
    };
    std::mt19937_64 generator(config.seed);
    data.starting_gifts.resize(7);
    for(auto& entry:data.starting_gifts){
        auto index = rng::vindex(gifts,generator);
        if(index>=gifts.size())continue;
        entry = gifts[index];
        std::swap(gifts[index],gifts.back());
        gifts.pop_back();
    }
}

//Writing stuff
struct IdCount{size_t index;int count=0;};
void write_lots(ParamFile<ItemLot>& param,std::unordered_map<s32,IdCount>& id_to_index_count,std::vector<LotData>& lots){
    for(const auto& a:lots){
        auto ref = id_to_index_count.find((u32)a.lot_id);
        if(ref==id_to_index_count.end()){
            std::cout<<"WARNING: Unknown item lot:"<<a.lot_id<<". Ignoring this lot\n";
            continue;//Unknown item lot?
        }
        IdCount& index_count = ref->second;
        if(index_count.count==10){
            std::cout<<"WARNING: Item overflow, lot:"<<a.lot_id<<" has more than 10 items. Ignoring new items\n";
            continue;//Item overflow
        }
        auto& lot = param.data[index_count.index];
        auto j = index_count.count;
        lot.amount[j] = a.amount;
        lot.chance[j] = a.chance;
        lot.item_id[j] = a.item_id;
        lot.infinite[j] = a.infinite;
        lot.infusion[j] = (u8)a.infusion;
        lot.reinforcement[j] = a.reinforcement;
        index_count.count+=1;
    }
}
void write_shop_lots(ParamFile<ShopItem>& shop_data,std::unordered_map<s32,size_t>& id_to_index,const std::vector<ShopSlot>& slots,bool enable_all){
    for(const auto& a:slots){
        auto& lot = shop_data.data[id_to_index[a.lot_id]];
        lot.quantity=a.quantity;
        if(a.infinite) lot.quantity=255;
        lot.item_id=a.item_id;
        lot.price_rate=a.price_mult;
        lot.disable_flag=-1;
        if(enable_all) lot.enable_flag=-1;
    }
}
void write_item_params(ItemRandoData& rando_data,ItemRandoConfig& config,bool devmode){
    const std::string chr_param = "ItemLotParam2_Chr.param";
    const std::string shop_param = "ShopLineupParam.param";
    const std::string other_param = "ItemLotParam2_Other.param";
    const std::string classes_param = "PlayerStatusParam.param";

    std::filesystem::path chr_path     = paths::params/chr_param;
    std::filesystem::path shop_path    = paths::params/shop_param;
    std::filesystem::path other_path   = paths::params/other_param;
    std::filesystem::path classes_path = paths::params/classes_param;

    ParamFile<ShopItem> shop_data = read_param_file<ShopItem>(get_file_contents_binary(shop_path));
    std::unordered_map<s32,size_t> id_to_index;
    id_to_index.reserve(shop_data.data.size());
    for(size_t i = 0;i<shop_data.data.size();i++){
        id_to_index[(u32)shop_data.row_info[i].row]=i;
    }
    write_shop_lots(shop_data,id_to_index,rando_data.shops.common,config.unlock_common_shop);
    write_shop_lots(shop_data,id_to_index,rando_data.shops.straid_trades,config.unlock_straid_trades);
    write_shop_lots(shop_data,id_to_index,rando_data.shops.ornifex_trades,config.unlock_ornifex_trades);
    for(const auto& a:rando_data.shops.to_remove){
        auto& lot = shop_data.data[id_to_index[a]];
        lot.enable_flag=1;//Items won't show up with this
    }
    if(config.melentia_lifegems){
        ShopItem item;
        item.item_id=60010000;
        item.unk=500;
        item.enable_flag=203850;
        item.disable_flag=-1;
        item.material_id=0;
        item.duplicate_id=0;
        item.unk2=0;
        item.price_rate=1.f;
        item.quantity=255;
        //75400615 Melentia last shop item
        auto new_row = insert_next_free_entry(75400615,item,shop_data);
        if(!new_row){
            std::cout<<"Failed to add Melentia infinite lifegems\n";
        }
    }
    ParamFile<ItemLot> other = read_param_file<ItemLot>(get_file_contents_binary(other_path));
    std::unordered_map<s32,IdCount> id_to_index_count;
    id_to_index_count.reserve(other.data.size());

    for(size_t i = 0;i<other.data.size();i++){
        auto row = other.row_info[i].row;
        if(vector_contains(rando_data.nochange_lots,(s32)row)) continue;//Don't overwrite estus flask
        id_to_index_count[(u32)row]={i,0};
        auto& lot = other.data[i];
        for(size_t j = 0;j<10;j++){
            lot.item_id[j]=10;
            lot.amount[j]=0u;
            lot.chance[j]=0.f;
            lot.infusion[j]=0;
            lot.reinforcement[j]=0;
        }
    }
    write_lots(other,id_to_index_count,rando_data.lots);
    for(const auto& a:rando_data.equivalents){
        auto original_index = id_to_index_count[a.first].index;
        auto copy_index = id_to_index_count[a.second].index;
        other.data[copy_index]=other.data[original_index];
    }

    ParamFile<ItemLot> chr = read_param_file<ItemLot>(get_file_contents_binary(chr_path));
    id_to_index_count.clear();
    id_to_index_count.reserve(chr.data.size());
    for(size_t i = 0;i<chr.data.size();i++){
        s32 row = chr.row_info[i].row;
        if(vector_contains(rando_data.nochange_lots,row)) continue;//Don't overwrite estus flask
        id_to_index_count[(u32)chr.row_info[i].row]={i,0};
        auto& lot = chr.data[i];
        for(size_t j = 0;j<10;j++){
            lot.item_id[j]=10;
            lot.amount[j]=0u;
            lot.chance[j]=0.f;
            lot.infusion[j]=0;
            lot.reinforcement[j]=0;
        }
    }
    write_lots(chr,id_to_index_count,rando_data.chr_lots);
    write_lots(chr,id_to_index_count,rando_data.enemy_drop_lots);


    ParamFile<PlayerStatus> classes = read_param_file<PlayerStatus>(get_file_contents_binary(classes_path));
    if(config.randomize_classes){
        for(size_t i =0;i<classes.data.size();i++){
            auto row   = classes.row_info[i].row;
            auto& data = classes.data[i];
            for(const auto& mclass:rando_data.classes){
                if(mclass.id==row){
                    data.head_armor  = mclass.gear.head;
                    data.chest_armor = mclass.gear.chest;
                    data.hands_armor = mclass.gear.arms;
                    data.legs_armor  = mclass.gear.legs;
                    data.ring_id[0]  = mclass.gear.ring;
                    data.spell_id[0] = mclass.gear.spell;
                    data.right_weapon[0] = mclass.gear.right_hand;
                    data.right_weapon_reinforcement[0] = 0;
                    data.left_weapon[0] = mclass.gear.left_hand;
                    data.left_weapon_reinforcement[0] = 0;
                    //No extra items or weapons
                    data.right_weapon[1] = 3400000;
                    data.left_weapon[1]  = 3400000;
                    for(size_t j=0;j<10;j++){
                        data.item_id[j]=-1;
                        data.item_amount[j]=0;
                    }
                    //Everyone gets 10 lifegems tho
                    data.item_id[0]=60010000;
                    data.item_amount[0]=10;
                    //Also fix the arrow situation
                    data.arrow_id[0]=data.arrow_id[1]=data.bolt_id[0]=data.bolt_id[1]=-1;
                    data.arrow_amount[0]=data.arrow_amount[1]=data.bolt_amount[0]=data.bolt_amount[1]=0;
                    if(mclass.gear.extra_arrow>0){
                        data.arrow_id[0]=mclass.gear.extra_arrow;
                        data.arrow_amount[0]=100;
                    }else if(mclass.gear.extra_bolt>0){
                        data.bolt_id[0]=mclass.gear.extra_bolt;
                        data.bolt_amount[0]=100;
                    }
                    break;
                }
            }
        }
    }
    if(config.randomize_gifts){
        for(size_t i =0;i<classes.data.size();i++){
            auto row   = classes.row_info[i].row;
            auto& data = classes.data[i];
            if(row>=510&&row<=570){
                size_t gift_index = (row-510)/10;
                auto& gifts = rando_data.starting_gifts[gift_index];
                data.ring_id[0]=-1;//Remove the life ring from that one lot
                for(size_t j=0;j<10;j++){
                    if(j<gifts.size()){
                        data.item_id[j]=gifts[j].id;
                        data.item_amount[j]=(u16)gifts[j].quantity;
                    }else{
                        data.item_id[j]=-1;
                        data.item_amount[j]=0;
                    }
                }
            }
        }
    }

    if(!std::filesystem::exists(paths::out_folder)){
        std::filesystem::create_directories(paths::out_folder);
    }
    write_to_file_binary(paths::out_folder/shop_param,write_param_file(shop_data));
    write_to_file_binary(paths::out_folder/chr_param,write_param_file(chr));
    write_to_file_binary(paths::out_folder/other_param,write_param_file(other));   
    write_to_file_binary(paths::out_folder/classes_param,write_param_file(classes)); 
    if(devmode){
        std::filesystem::path dev_path{"C:\\Program Files (x86)\\Steam\\steamapps\\common\\Dark Souls II Scholar of the First Sin\\Game\\mods\\mod_testing\\Param"};
        write_to_file_binary(dev_path/shop_param,write_param_file(shop_data));
        write_to_file_binary(dev_path/chr_param,write_param_file(chr));
        write_to_file_binary(dev_path/other_param,write_param_file(other));   
        write_to_file_binary(dev_path/classes_param,write_param_file(classes)); 
    }
}
void write_cheatsheet(ItemRandoData& data,ItemRandoConfig& config){
    std::stringstream ss;
    ss<<"---CONFIGURATION FILE---\n";
    ss<<generate_config_file(config);
    ss<<"\n\n\n---ITEM CHEATSHEET---\n";

    for(const auto& trade:data.shops.straid_trades){
        ss<<data.items.names[trade.item_id]<<"\nTraded by Straid\nReplaced"<<data.shops.original_items[trade.lot_id]<<"\n\n";
    }
    for(const auto& trade:data.shops.ornifex_trades){
        ss<<data.items.names[trade.item_id]<<"\nTraded by Ornifex\nReplaced"<<data.shops.original_items[trade.lot_id]<<"\n\n";
    }
    for(const auto& trade:data.shops.common){
        if(trade.infinite){
            ss<<data.items.names[trade.item_id]<<" infinite\nSold by "<<data.shops.original_items[trade.lot_id]<<"\n\n";
        }else{
            ss<<data.items.names[trade.item_id]<<" x"<<(int)trade.quantity<<"\nSold by"<<data.shops.original_items[trade.lot_id]<<"\n\n";
        }
    }

    for(const auto& lot:data.lots){
        if(lot.lot_id>=50000000&&lot.lot_id<=50000303){
            auto item = lot.lot_id%1000;
            ss<<data.items.names[lot.item_id]<<" Dyna & Tillo ";
            if(item<100) ss<<" Small Smooth and Silky Stone drop\n";
            else if(item<200) ss<<" Smooth and Silky Stone drop\n";
            else if(item<300) ss<<" Petrified Something drop\n";
            else ss<<" Prism Stone drop\n";
        }else{
            auto& desc = data.lot_name_other[(u32)lot.lot_id];
            auto find = desc.find("Replaces",0);
            if(find==std::string::npos){
                ss<<data.items.names[lot.item_id]<<" x"<<(int)lot.amount<<"\n"<<desc<<'\n';
            }else{
                ss<<data.items.names[lot.item_id]<<" x"<<(int)lot.amount<<"\n"<<desc.substr(0,find)<<'\n'<<desc.substr(find)<<"\n\n";
            }
            // ss<<data.items.names[lot.item_id]<<" x"<<(int)lot.amount<<" in "<<data.lot_name_other[lot.lot_id]<<'\n';
        }
    }
    for(const auto& lot:data.chr_lots){
        ss<<data.items.names[lot.item_id]<<" x"<<(int)lot.amount<<"\n"<<data.lot_name[(u32)lot.lot_id]<<"\n\n";
    }

    // for(const auto& [id,name] : data.enemy_names){
    //     std::cout<<id<<" "<<name<<'\n';
    // }
    std::unordered_map<u64,u8> enemy_used;
    enemy_used.reserve(512);
    for(const auto& lot:data.enemy_drop_lots){
        u64 id = lot.lot_id/10000u;
        u64 key = id;
        key = key<<32;
        key+=lot.item_id;
        if(enemy_used.find(key)==enemy_used.end()){
            ss<<data.items.names[lot.item_id]<<" dropped by "<<data.enemy_names[(s32)id]<<"\n\n";
            enemy_used[key]=1;
        }   
    }

    if(!std::filesystem::exists(paths::cheatsheet_folder)){
        std::filesystem::create_directories(paths::cheatsheet_folder);
    }
    auto path = paths::cheatsheet_folder/("items"+time_string_now()+".txt");
    std::ofstream out_file(path);
    if(out_file) out_file<<ss.str();
    else std::cout<<"Cannot write cheatsheet: "<<path<<'\n';
    out_file.close();
}

bool add_item_shop(s32 item_id,bool devmode){
    if(item_id==0){
        std::cout<<"Invalid item ID\n";
        return false;
    }
    //In this case is we need to get the output param as it will need to change the already modified file
    std::filesystem::path shop_path = paths::out_folder/"ShopLineupParam.param";
    if(devmode){
        shop_path = paths::dev_path/"ShopLineupParam.param";
    }
    ParamFile<ShopItem> shop_data = read_param_file<ShopItem>(get_file_contents_binary(shop_path));
    if(shop_data.header.start_of_data==0){
        std::cout<<"Can't find shop param file\n";
        return false;
    }
    ShopItem item;
    item.item_id=item_id;
    item.unk=10;
    item.enable_flag=-1;
    item.disable_flag=-1;
    item.material_id=0;
    item.duplicate_id=0;
    item.unk2=0;
    item.price_rate=0.f;
    item.quantity=255;
    //777000 Sweet Shalquoir id
    auto new_row = insert_next_free_entry(77700404,item,shop_data);
    if(new_row!=SIZE_MAX){
        write_to_file_binary(shop_path,write_param_file(shop_data));
        return true;
    }
    return false;
}

bool randomize_items(IRData& irdata,bool devmode){
    Stopwatch clock;
    auto& config = irdata.config;
    if(!config.valid){
        std::cout<<"Item loading went wrong, item randomizer skipped\n";
        return false;
    }
    //Makes a copy of data so there is no need to reload eveything after a new randomization during same session
    //This makes a copy of more things than needed but its fast enough so I dont care
    auto data = *irdata.data;
    if(config.randomize_key_items){
        if(!place_graph_key_items(data,config)){
            std::cout<<"Failed to place key items\n";
            return false;
        }
    }else{
        remove_key_items(data,config);
    }
    if(!place_rest_keys(data,config)){
        std::cout<<"Failed to place key items\n";
        return false;
    }
    place_shop_items(data,config);
    place_dyna_tillo_items(data,config);
    place_items(data,config);
    place_enemy_drops(data,config);
    if(config.infuse_weapons) randomize_weapon_infusion(data,config);
    if(config.randomize_classes) randomize_classes(data,config);
    if(config.randomize_gifts) randomize_starting_gifts(data,config);
    write_item_params(data,config,devmode);
    if(config.write_cheatsheet){
        write_cheatsheet(data,config);
    }
    auto t = clock.passed();
    std::cout<<"Randomized items in "<<t/1000<<"ms\n";
    return true;
}
bool restore_default_params(){
    const std::string chr_param = "ItemLotParam2_Chr.param";
    const std::string shop_param = "ShopLineupParam.param";
    const std::string other_param = "ItemLotParam2_Other.param";
    const std::string classes_param = "PlayerStatusParam.param";
    //Cppref wasnt clear if this thing returns false or throws when it fails so idk
    try{
        auto ow = std::filesystem::copy_options::overwrite_existing;
        std::filesystem::copy_file(paths::params/chr_param,paths::out_folder/chr_param,ow);
        std::filesystem::copy_file(paths::params/shop_param,paths::out_folder/shop_param,ow);
        std::filesystem::copy_file(paths::params/other_param,paths::out_folder/other_param,ow);
        std::filesystem::copy_file(paths::params/classes_param,paths::out_folder/classes_param,ow);
    }catch (std::filesystem::filesystem_error& e){
        std::cout << "Could not restore defaults: " << e.what() << '\n';
        return false;
    }
    return true; 
}

//Tests and file generators
void lots_test(){
    std::vector<uint32_t> safe,miss;
    safe.reserve(2000);
    miss.reserve(2000);
    std::ifstream file_unmissable("./build/OtherLots.txt");
    safe.reserve(2000);
    std::string line;
    bool missable = false,unmissable =false;
    while(std::getline(file_unmissable,line)){
        if(line.empty()||line.substr(0,2)=="//")continue;
        if(line.front()=='#'){
            missable=unmissable=false;
            if(line=="#UNMISSABLE") unmissable=true;
            else if(line=="#MISSABLE") missable=true;
        }else{
            auto tokens = parse::split(line,';');
            uint32_t value=0;
            if(parse::read_var(tokens.front(),value)){
                if(unmissable){
                    safe.push_back(value);
                }else if(missable){
                    miss.push_back(value);
                }
            }
        }
    }
    for(const auto& a:safe){
        for(const auto& b:miss){
            if(a==b){
                std::cout<<"Repeated lot: "<<a<<'\n';
            }
        }
    }
    std::vector<std::vector<uint32_t>> equivalent; 
    std::ifstream file("./build/EquivalentLots.txt");
    while(std::getline(file,line)){
        std::string_view view = line;
        auto tokens = parse::split(view,',');
        equivalent.push_back({});
        for(const auto& lot:tokens){
            uint32_t value=0;
            parse::read_var(lot,value);
            equivalent.back().push_back(value);
        }
    }
    auto is_missable = [&](uint32_t lot){
        for(const auto& a:miss){
            if(a==lot){
                return true;
            }
        }
        for(const auto& a:safe){
            if(a==lot){
                return false;
            }
        }
        std::cout<<"Unknown event: "<<lot<<'\n';
        return false;
    };
    for(const auto& e:equivalent){
        bool first = true;
        bool result = false;
        for(const auto& lot:e){
            if(first){
                result = is_missable(lot);
                first = false;
            }else{
                if(result!=is_missable(lot)){
                    std::cout<<"Missmatch missable and unmissable: ";
                    for(const auto& ll:e){
                        std::cout<<ll<<",";
                    }
                    std::cout<<'\n';
                }   
            }
        }
    }

}
bool unmissable_location_test(){
    struct Lot{
        uint32_t id;
        std::string location;
        size_t index{SIZE_MAX};
        bool known{false};
    };
    std::ifstream file_unmissable("./build/OtherLots.txt");
    std::vector<Lot> safe;
    safe.reserve(2000);
    std::string line;
    bool missable = false,unmissable =false;
    while(std::getline(file_unmissable,line)){
        if(line.empty()||line.substr(0,2)=="//")continue;
        if(line.front()=='#'){
            missable=unmissable=false;
            if(line=="#UNMISSABLE") unmissable=true;
            else if(line=="#MISSABLE") missable=true;
        }else{
            auto tokens = parse::split(line,';');
            uint32_t value=0;
            if(unmissable&&parse::read_var(tokens.front(),value)){
                safe.push_back({value,"",SIZE_MAX,false});
            }
        }
    }
    std::vector<Lot> loaded;
    loaded.reserve(2000);
    std::ifstream file("./build/FloorItems.txt");
    if(!file){
        std::cout<<"Error opening file\n";
        return false;
    }
    std::string current_location;
    while(std::getline(file,line)){
        std::string_view view = line;
        if(view.substr(0,2)=="//") continue;
        if(view.front()=='#'){
            current_location = view.substr(1);
            continue;
        }
        auto tokens = parse::split(view,',');
        for(const auto& t:tokens){
            uint32_t value=0;
            if(parse::read_var(t,value)){
                loaded.push_back({value,current_location});
            }
        }
        
    }
    file.close();
    file.open("./build/EventLots.txt");
    if(!file){
        std::cout<<"Error opening file\n";
        return false;
    }
    while(std::getline(file,line)){
        std::string_view view = line;
        if(view.substr(0,2)=="//") continue;
        auto tokens = parse::split(view,',');
        if(tokens.size()<2){
            std::cout<<"Missing data in line: "<<line<<'\n';
            continue;
        }
        uint32_t value=0;
        if(parse::read_var(tokens.front(),value)){
            loaded.push_back({value,std::string(tokens[1])});
        }
    }

    for(size_t i =0;i<loaded.size();i++){
        for(size_t j=i+1;j<loaded.size();j++){
            if(loaded[i].id==loaded[j].id){
                std::cout<<"duplicated: "<<loaded[i].id<<'\n';
                return false;
            }
        }
    }
    solver::Graph graph;
    solver::parse_from_file("build/Drangleic.txt",graph);
    for(auto& a:safe){
        for(auto& l:loaded){
            if(a.id==l.id){
                a.known=true;
                a.location=l.location;
                break;
            }
        }
        for(size_t i = 0;i<graph.rooms.size();i++){
            if(graph.rooms[i].name==a.location){
                a.index=i;
                break;
            }
        }
        if(!a.known){
            std::cout<<"No location for id: "<<a.id<<'\n';
            return false;
        }
        if(a.index==SIZE_MAX){
            std::cout<<"Cannot match room: "<<a.id<<" "<<a.location<<'\n';
            return false;
        }
    }
    return true;

}
void find_item_lots_enemies(){
    std::vector<std::tuple<u64,std::string,std::string>> map_names{
        {10020000u,"m10_02_00_00","Things Betwixt"},
        {10040000u,"m10_04_00_00","Majula"},
        {10100000u,"m10_10_00_00","Forest of Fallen Giants"},
        {10140000u,"m10_14_00_00","Brightstone Cove Tseldora"},
        {10150000u,"m10_15_00_00","Aldia's Keep"},
        {10160000u,"m10_16_00_00","The Lost Bastille & Belfy Luna"},
        {10170000u,"m10_17_00_00","Harvest Valley & Earthen Peak"},
        {10180000u,"m10_18_00_00","No-man's Wharf"},
        {10190000u,"m10_19_00_00","Iron Keep & Belfry Sol"},
        {10230000u,"m10_23_00_00","Huntsman's Copse & Undead Purgatory"},
        {10250000u,"m10_25_00_00","The Gutter & Black Gulch"},
        {10270000u,"m10_27_00_00","Dragon Aerie & Shrine"},
        {10290000u,"m10_29_00_00","Majula <=> Shaded Woods"},
        {10300000u,"m10_30_00_00","Heide's Tower <=> No-man's Wharf"},
        {10310000u,"m10_31_00_00","Heide's Tower of Flame"},
        {10320000u,"m10_32_00_00","Shaded Woods & Shrine of Winter"},
        {10330000u,"m10_33_00_00","Doors of Pharros"},
        {10340000u,"m10_34_00_00","Grave of Saints"},
        {20100000u,"m20_10_00_00","Giant memories"},
        {20110000u,"m20_11_00_00","Shrine of Amana"},
        {20210000u,"m20_21_00_00","Drangleic Castle & Throne"},
        {20240000u,"m20_24_00_00","Undead Crypt"},
        {20260000u,"m20_26_00_00","Dragon Memories"},
        {40030000u,"m40_03_00_00","Dark Chasm of Old"},
        {50350000u,"m50_35_00_00","Shulva, Sanctum City"},
        {50360000u,"m50_36_00_00","Brume Tower"},
        {50370000u,"m50_37_00_00","Frozen Eleum Loyce"},
        {50380000u,"m50_38_00_00","Memory of the King"}
    };
    std::filesystem::path folder_path = "build/Params/generator";
    std::filesystem::path names_path = "build/Params/generator_names";
    std::filesystem::path lots_path = "build/Params/ItemLotParam2_Chr.param";
    
    std::vector<ParamFile<Generator>> v;
    for(const auto& entry:std::filesystem::directory_iterator(folder_path)){
        auto path = entry.path();
        if(path.extension()!=".param") continue;
        std::ifstream file(path,std::ifstream::binary);
        if(!file){
            std::cout<<"Failed to load generators, can't open file:"<<path<<"\n";
            return;
        }
        std::stringstream ss;
        ss<<file.rdbuf();
        v.push_back(read_param_file<Generator>(ss.str()));
    }
    std::vector<std::unordered_map<u64,std::string>> names;
    for(const auto& entry:std::filesystem::directory_iterator(names_path)){
        auto path = entry.path();
        std::ifstream file(path);
        if(!file){
            std::cout<<"Failed to load generators names, can't open file:"<<path<<"\n";
            return;
        }
        names.push_back({});
        std::string line;
        while(std::getline(file,line)){
            std::string_view view=line;
            u64 id =0;
            parse::read_var(view.substr(0,view.find(" [")),id);
            std::string name{view.substr(view.find("] ")+2)};
            names.back()[id] = name;
            //std::cout<<id<<" "<<name<<'\n';
        }
    }
    auto chr_lots=read_param_file<ItemLot>(get_file_contents_binary(lots_path));

    std::vector<std::string> rows;
    for(size_t i = 0;i<v.size();i++){
        const auto& gen=v[i];
        std::cout<<"Map: "<<std::get<2>(map_names[i])<<'\n';
        // std::cout<<names[i].size()<<'\n';
        // for(const auto& n:names[i]){
        //     std::cout<<n.first<<" "<<n.second<<'\n';
        // }
        for(const auto& lot:chr_lots.row_info){
            if(lot.row==0)continue;
            rows.clear();
            for(size_t j=0;j<gen.data.size();j++){
                const auto& entry=gen.data[j];
                if(entry.item_lot_id[0]==lot.row||entry.item_lot_id[1]==lot.row){
                    auto name = names[i][gen.row_info[j].row];
                    if(!vector_contains(rows,name))rows.push_back(name);
                    // rows.push_back(gen.row_info[j].row);
                }
            }
            if(!rows.empty()){
                std::cout<<"Lot "<<lot.row<<" in: ";
                for(const auto& x:rows) std::cout<<x<<',';
                std::cout<<"\n";
            }
        }
    }

    

}
void get_weapon_data(){
    std::filesystem::path name_path ="build/stuff/weapon_infusions.txt";
    std::ifstream file;
    open_file(file,name_path);
    std::string line;
    std::unordered_map<s32,std::string> weapon_infusions;
    while(std::getline(file,line)){
        auto tokens = parse::split(line,',');
        auto id = 0;
        parse::read_var(tokens[0],id);
        weapon_infusions[id]=tokens[1];
    }
    file.close();

    std::unordered_map<s32,std::pair<s32,s32>> weapons_infusion_reinf;
    std::filesystem::path path = "build/Params/WeaponReinforceParam.param";
    auto weapon_data = read_param_file<WeaponReinforce>(get_file_contents_binary(path));
    for(size_t i =0;i<weapon_data.data.size();i++){
        auto& data = weapon_data.data[i];
        auto row = weapon_data.row_info[i].row;
        //auto& infusion_type = weapon_infusions[data.spec_param];
        auto max_reinforcement = data.max_lvl;
        if(data.reinforce_cost==0) max_reinforcement=0;
        weapons_infusion_reinf[(s32)row]=std::make_pair(data.spec_param,max_reinforcement);
        // out<<row<<","<<infusion_type<<","<<max_reinforcement<<'\n';
    }

    path = "build/Params/WeaponParam.param";
    auto weapon_stats = read_param_file<WeaponStat>(get_file_contents_binary(path));
    std::stringstream out;
    for(size_t i =0;i<weapon_stats.data.size();i++){
        auto row = weapon_stats.row_info[i].row;
        // if(row<1000000||row>6000000) continue;
        auto& data = weapon_stats.data[i];
        auto& inf_reinf =weapons_infusion_reinf[data.weapon_reinf_id];
        auto& inf_str = weapon_infusions[inf_reinf.first];
        out<<row<<','<<data.str<<','<<data.dex<<','<<data.inte<<','<<data.fth<<','<<data.weight<<','<< inf_str<<','<<inf_reinf.second<<'\n';
    }
    std::ofstream out_file("build/stuff/WeaponData.txt");
    out_file<<out.str();
    out_file.close();
}
void get_armor_data(){
    std::stringstream out;
    std::filesystem::path path = "build/Params/ArmorParam.param";
    auto armor_data = read_param_file<ArmorStat>(get_file_contents_binary(path));
    for(size_t i =0;i<armor_data.data.size();i++){
        auto& data = armor_data.data[i];
        auto row = armor_data.row_info[i].row;
        out<<row<<","<<data.str_req<<","<<data.dex_req<<','<<data.int_req<<','<<data.fth_req<<','<<data.weight<<'\n';
    }
    std::ofstream out_file("build/stuff/ArmorData.txt");
    out_file<<out.str();
    out_file.close();
}
void get_spell_data(){
    std::stringstream out;
    std::filesystem::path path = "build/Params/SpellParam.param";
    auto spell_data = read_param_file<SpellStat>(get_file_contents_binary(path));
    for(size_t i =0;i<spell_data.data.size();i++){
        auto& data = spell_data.data[i];
        auto row = spell_data.row_info[i].row;
        out<<row<<','<<data.spell_class<<','<<data.int_req<<','<<data.fth_req<<','<<(int)data.slots_used<<'\n';
    }
    std::ofstream out_file("build/stuff/SpellData.txt");
    out_file<<out.str();
    out_file.close();
}
void get_ring_data(){
    std::stringstream out;
    std::filesystem::path path = "build/Params/RingParam.param";
    auto ring_data = read_param_file<RingStat>(get_file_contents_binary(path));
    for(size_t i =0;i<ring_data.data.size();i++){
        auto& data = ring_data.data[i];
        auto row = ring_data.row_info[i].row;
        out<<row<<','<<data.weight<<'\n';
    }
    std::ofstream out_file("build/stuff/RingData.txt");
    out_file<<out.str();
    out_file.close();
}
void get_itemlots_description(){
    std::filesystem::path path = "build/lots_description.txt";
    std::ifstream file;
    if(!open_file(file,path))return;
    std::string line;
    std::unordered_map<s32,std::string> lots_descriptions;
    while(std::getline(file,line)){
        auto tokens = parse::split(line,';');
        if(tokens.size()!=2){
            std::cout<<"Bad line: "<<line<<'\n';
        }
        s32 id = 0;
        parse::read_var(tokens[0],id);
        lots_descriptions[id]=tokens[1];
    }
    file.close();

    GameItems items;
    load_items(items);
    std::filesystem::path other_path = "build/Params/ItemLotParam2_Other.param";
    ParamFile<ItemLot> other_lots = read_param_file<ItemLot>(get_file_contents_binary(other_path));
    std::unordered_map<s32,std::string> original_items;
    for(size_t i = 0;i<other_lots.data.size();i++){
        auto row = other_lots.row_info[i].row;
        auto& data = other_lots.data[i];
        line.clear();
        for(size_t j =0;j<10;j++){
            if(data.amount[j]==0||data.item_id[j]==10||data.chance[j]<0.001f) continue;
            if(!line.empty()) line+=", ";
            if((Infusion)data.infusion[j]!=Infusion::None){
                line+=infusion_to_string((Infusion)data.infusion[j]);
            }
            line+=items.names[data.item_id[j]];
            if(data.reinforcement[j]>0u){
                line+="+"+std::to_string(data.reinforcement[j]);
            }
            if(data.amount[j]>1){
                line+=" x"+std::to_string(data.amount[j]);
            }
        }
        original_items[(s32)row]=line;
    }


    std::stringstream out;
    std::filesystem::path other_lots_path = "build/OtherLots.txt";
    if(!open_file(file,other_lots_path))return;
    while(std::getline(file,line)){
        if(line.empty()||line.substr(0,2)=="//")continue;
        if(line.front()=='#'){
            out<<line<<'\n';
        }else{
            auto tokens = parse::split(line,';');
            s32 value=0;
            if(parse::read_var(tokens.front(),value)){
                if(lots_descriptions.find(value)!=lots_descriptions.end()){
                    out<<value<<";"<<lots_descriptions[value]<<". Replaces "<<original_items[value]<<'\n';
                }else{
                    auto e = line.find(']',0);
                    out<<line.substr(0,e+1)<<" Replaces "<<original_items[value]<<'\n';
                    // out<<line<<'\n';
                }
            }
        }
    }
    std::ofstream out_file("build/stuff/description_test.txt");
    out_file<<out.str();
    out_file.close();
}



//Housekeeping
void free_rando_data(IRData& data){
    if(data.data)delete data.data;
}

// int main(){
    // lots_test();
    // unmissable_location_test();
    // ItemRandoConfig config;
    // config.infuse_weapons=true;
    // config.unlock_common_shop=true;
    // config.unlock_ornifex_trades=true;
    // config.unlock_straid_trades=true;
    // config.randomize_classes=true;
    // ItemRandoData data;
    // load_randomizer_data(data,config);
    // randomize_items(data,config);
    // std::cout<<"out\n";
    // return EXIT_SUCCESS;
// }
}