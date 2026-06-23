#ifndef MY_PARAMEDITOR
#define MY_PARAMEDITOR

#include <string>
#include <vector>
#include <iostream>
#include <cstdint>
#include <cstring>

using s8  = int8_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;
using s32 = int32_t;
using f32 = float;


struct Location{
    f32 position[3];
    f32 rotation[3];
    u32 unk18;
    u32 inherit_instance_param;
};
struct Register{
    s32 enemy_id;
    s32 logic_id;
    s32 default_logic_id;
    u16 spawn_state;
    u8  draw_group;
    u8  display_group;
};
struct Generator{
    u8  spawn_limit;
    u8  spawn_limit_clear;
    u8  unk02;
    u8  aggro_group;
    u32 death_event_id;
    u32 generator_regist_param;
    u16 unk0C;
    u16 unk10;
    u32 apperance_event_id;
    u32 previous_hostile_event_id[2];
    u32 unk1c[4];
    u32 location_event_id[5];
    u32 unk40;
    u32 unk44;
    u16 unk48;
    u16 bonfire_id;
    f32 leash_dist;
    u32 location_event_id2[2];
    u32 ai_think_id;
    u32 activation_event_id[2];
    u32 hostile_event_id[2];
    u16 unk6C;
    u16 unk71;
    u32 location_event_id3[2];
    u32 unk78;
    u32 unk7C;
    f32 unk80;
    u32 unk84;
    u32 item_lot_id[2];
    u32 grave_event_id;
    f32 unk94;
    u32 unk98;
    u32 unk9C;
};
struct PlayerStatus{
    u8  body_type;
    u8  hollowing_type;
    u16 body_build;
    u16 soul_level;
    u16 vigor;
    s32 souls;
    u16 endurance;
    u16 attunement;
    u16 vitality;
    u16 strength;
    u16 dexterity;
    u16 intelligence;
    u16 faith;
    u16 adaptability;
    u32 unk0;
    u16 facial_preset;
    u16 unk1;
    s32 item_id[10];//-1 for no item
    u16 item_amount[10];
    s32 spell_id[10];//-1 for no spell
    s32 right_weapon[3];//3400000 for no weapon
    s32 left_weapon[3];
    s32 head_armor;//21001100 for no armor
    s32 chest_armor;//21001101 for no armor
    s32 hands_armor;//21001102 for no armor
    s32 legs_armor;//21001103 for no armor
    s32 ring_id[4];//-1 for no ring
    u32 right_weapon_reinforcement[3];
    u32 left_weapon_reinforcement[3];
    s32 unk2;
    s32 player_status_id[3];
    s32 unk4;
    s32 arrow_id[2];//-1 for no arrow
    s32 bolt_id[2];//-1 for no bolt
    u16 arrow_amount[2];
    u16 bolt_amount[2];
    s32 gestures[8];
    u8  covenant;
    u8  covenant_lvl;
    u16 unk3;
};
struct SpellStat{
    s32 spell_class;//0 sorcery,1 miracle,2 pyro,3 hex(staff),4 hex(chime)
    u8  spell_function[2];
    u16 unk0;
    u16 int_req;
    u16 fth_req;
    s32 right_bullet[2];
    s32 hand_stuff[40];
    u8  unk2[12];
    s32 staff_sfx;
    u8  unk3[28];
    f32 startup_speed;
    f32 cast_speed;
    f32 unk4[2];
    u8  slots_used;
    u8  cast_tier[10];
    u8  unk5;
    s32 spell_soul_id;
    s32 left_bullet[2];
};
struct RingStat{
    f32 weight;
    f32 durability;
    s32 reapair_cost;
    s32 item_discovery;
};
struct ArmorStat{
    s32 id;
    u8  unk0;
    u8  slot_category;//1 head,2 body,3 arms,4 legs
    u16 unk1;
    u32 model_id;
    u8  is_gendered;
    u8  unk2;
    u16 unk3;
    s32 equip_interfere_id;
    s32 unk4;
    s32 armor_reinforce_id;
    f32 physical_defense;
    f32 slash_defense;
    f32 pierce_defense;
    f32 blunt_defense;
    u16 str_req;
    u16 dex_req;
    u16 int_req;
    u16 fth_req;
    f32 weight;
    f32 durability;
    u32 repair_cost;
    f32 poise;
    u8  blend_weight[2];
    u16 add_item_disc;
    s32 dmg_effect;
    s32 unk5;
    s32 dmg_effect_sfx;
};
struct WeaponStat{
    s32 id;
    s32 weapon_model_id;
    s32 weapon_reinf_id;
    s32 weapon_action;
    s32 weapon_type;
    s32 unk0;
    u16 str;
    u16 dex;
    u16 inte;
    u16 fth;
    f32 weight;
    f32 recovery_weight;
    f32 max_dur;
    s32 repair_cost;
    s32 unk1;
    f32 stamina_com;
    f32 stamina_dmg;
    s32 stamina_cost;
    f32 unk2[4];
    s32 dmg_id[9];
    f32 parry_dur;
    s32 unk3[2];
    f32 dmg_percent;
    f32 stm_dmg_percent;
    f32 equip_dmg_percent;
    f32 guard_break;
    f32 status_effect;
    f32 posture_dmg;
    f32 hitbox_rad;
    f32 hitbox_len;
    f32 hitback_rad;
    f32 hitback_len;
    u16 dmg_type_menu;
    u16 poise_type_menu;
    u16 counter_type_menu;
    u16 casting_speed_menu;
    f32 poise_pvp;
    f32 poise_pve;
    f32 hyperarmor_mult;
};
struct WeaponReinforce{
    f32 dmg[9];
    f32 max_dmg[9];
    s32 max_lvl;
    s32 stats_effect;
    f32 resistance[9];
    f32 stability[11];
    f32 dmg_percent[9];
    f32 infusion_mod[9];
    s32 spec_param;
    s32 cost_param;
    s32 reinforce_cost;
};
struct ShopItem{
    s32 item_id;
    s32 unk;
    s32 enable_flag;
    s32 disable_flag;
    s32 material_id;//What item do you need to craft boss stuff
    s32 duplicate_id;//Removes every copy of this from shops when purchased
    s32 unk2;
    f32 price_rate;
    s32 quantity;//255 is infinite

};
struct ItemLot{
    u8  drop_order;
    u8  drop_num;
    u16 unk;
    u8  amount[10];
    u8  reinforcement[10];
    u8  infusion[10];
    u8  infinite[10];
    s32 item_id[10];
    f32 chance[10];
};
struct EnemyParam{
    s32 id;
    s32 behavior_id;
    s32 common_id;
    s32 move_id;
    u8  unk[21];
    u8  spawn_limit;
    u8  unk23[2];
    s32 hp;
    u8  unk25[61];
    u8  dmg_table;
    u8  unk1[50];
    s32 defense;
    s32 magic_def;
    s32 light_def;
    s32 fire_def;
    s32 dark_def;
    s32 poison_def;
    u8  unk7[28];
    s32 souls_held;
    s32 unk2;
    f32 dmg_mult;
    u8  unk3[88];
    s32 item_lot;
    s32 item_lot_overkill;
    f32 unk4;
    s32 ngp_hp[7];
    u8  unk5[12];  
};

struct RoundOutDmg{
    u16 dmg;
    u16 stamina;
    u16 posture;
};
struct RoundInDmg{
    u16 stamina;
    u16 posture;
};
struct ChrRoundDamageParam{
    RoundOutDmg out[8];
    RoundInDmg  in [8]; 
};

struct ParamHeader{
    u32 end_of_file;
    u8 unk00[6];
    u16 n_rows;
    u8 filename[36];
    u64 start_of_data{0};
    u64 unk01;
};
struct ParamRowInfo{
    u64 row;
    u64 initial_byte;
    u64 end_of_file;
};

template<typename T>
struct ParamFile{
    ParamHeader header;
    std::vector<ParamRowInfo> row_info;
    std::vector<T> data;
};

template<typename T>
ParamFile<T> read_param_file(const std::string& buffer){
    if(buffer.empty()) return {};
    ParamFile<T> param_file;
    std::memcpy(&param_file.header,buffer.data(),sizeof(ParamHeader));
    auto n_rows = param_file.header.n_rows;
    param_file.row_info.resize(n_rows);
    std::memcpy(param_file.row_info.data(),buffer.data()+sizeof(ParamHeader),sizeof(ParamRowInfo)*n_rows);
    param_file.data.resize(n_rows);
    std::memcpy(param_file.data.data(),buffer.data()+param_file.header.start_of_data,sizeof(T)*n_rows);
    return param_file;
}

template<typename T>
std::string write_param_file(ParamFile<T>& param_file){
    if(param_file.data.size()!=param_file.row_info.size()){
        std::cout<<"Size of param file rows and data does not match\n";
        return "";
    }
    auto start_of_data = sizeof(ParamHeader)+sizeof(ParamRowInfo)*param_file.row_info.size();
    auto end_of_file = start_of_data+sizeof(T)*param_file.data.size();
    auto& header  = param_file.header;
    header.n_rows = static_cast<u16>(param_file.row_info.size());
    header.end_of_file   = static_cast<u32>(end_of_file);
    header.start_of_data = start_of_data;
    std::string buffer;
    buffer.resize(end_of_file,0);
    std::memcpy(buffer.data(),&param_file.header,sizeof(ParamHeader));
    for(size_t i = 0;i<param_file.row_info.size();i++){
        auto& row_entry = param_file.row_info[i];
        row_entry.end_of_file=end_of_file;
        row_entry.initial_byte=start_of_data+sizeof(T)*i;
    }

    std::memcpy(buffer.data()+sizeof(ParamHeader),param_file.row_info.data(),sizeof(ParamRowInfo)*param_file.row_info.size());
    std::memcpy(buffer.data()+start_of_data,param_file.data.data(),sizeof(T)*param_file.data.size());
    buffer+='\0';
    buffer+='\0';
    buffer+='\0';
    buffer+='\0';
    // std::cout<<"Successfully wrote param file\n";
    return buffer;
}

template<typename T>
void add_entry(u64 row,T entry,ParamFile<T>& params){
    params.row_info.push_back(ParamRowInfo{row});
    params.data.push_back(entry);
}

template<typename T>
size_t insert_next_free_entry(u64 row,T entry,ParamFile<T>& params){
    size_t offset = 0;
    for(size_t i = 0;i<params.row_info.size();i++){
        if(params.row_info[i].row>=row){
            if(params.row_info[i].row!=row+offset){
                params.data.insert(params.data.begin()+i,entry);
                params.row_info.insert(params.row_info.begin()+i,ParamRowInfo{row+offset});
                return row+offset;
            }
            offset+=1;
        }
    }
    return SIZE_MAX;
}

template<typename T>
void delete_entry(size_t index,ParamFile<T>& params){
    if(index>=params.data.size()){
        std::cout<<"Deleting outside range: "<<index<<" "<<params.header.filename<<'\n';
        return;
    }
    params.row_info.erase(params.row_info.begin()+index);
    params.data.erase(params.data.begin()+index);
}

template<typename T>
T* get_entry_ptr(ParamFile<T>& params,size_t row){
    for(size_t i =0;i<params.row_info.size();i++){
        if(params.row_info[i].row==row) return &params.data[i];
    }
    std::cout<<"Cannot find entry for row "<<row<<" in "<<params.header.filename<<'\n';
    return nullptr;
}

#endif