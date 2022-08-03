#include "config.h"

#include <fstream>

#include "json.hpp"

namespace sendprotest {
using json = nlohmann::json;

void to_json(json& j, const EffectConfig& cfg) {
  j = json{{"finger_radius", cfg.finger_radius},
           {"effect_fade_in", cfg.effect_fade_in},
           {"effect_amount", cfg.effect_amount}};
}

void from_json(const json& j, EffectConfig& cfg) {
  j.at("finger_radius").get_to(cfg.finger_radius);
  j.at("effect_fade_in").get_to(cfg.effect_fade_in);
  j.at("effect_amount").get_to(cfg.effect_amount);
}

void to_json(json& j, const PlayerConfig& cfg) {
  j = json{{"playback_duration", cfg.playback_duration},
           {"category", cfg.category}};
}

void from_json(const json& j, PlayerConfig& cfg) {
  j.at("playback_duration").get_to(cfg.playback_duration);
  j.at("category").get_to(cfg.category);
}

Config::Config(const string& json_path) : json_path(json_path) {
  std::ifstream f(json_path);
  json j = json::parse(f);
  effect_data = j["EffectConfig"].get<map<string, EffectConfig>>();
  player_config = j["PlayerConfig"].get<PlayerConfig>();
  selected_effect = effect_data.begin()->first;
}

map<string, EffectConfig>& Config::get_effects() { return effect_data; }
PlayerConfig& Config::get_player_config() { return player_config; }

void Config::save() {
  std::ofstream f(json_path);
  json j;
  j["EffectConfig"] = effect_data;
  j["PlayerConfig"] = player_config;
  f << std::setw(4) << j << std::endl;
}

EffectConfig& Config::get_selected_effect() { return effect_data[selected_effect]; }
const string& Config::get_selected_name() { return selected_effect; }
void Config::set_selected_name(const string& name) { selected_effect = name; }

}  // namespace sendprotest
