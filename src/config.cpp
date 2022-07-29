#include "config.h"

#include <fstream>

#include "json.hpp"

namespace mpv_glsl {
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

Config::Config(const string& json_path) {
  std::ifstream f(json_path);
  json j = json::parse(f);
  data = j.get<unordered_map<string, EffectConfig>>();
  selected_effect = data.begin()->first;
}

unordered_map<string, EffectConfig>& Config::get() { return data; }

void Config::save() {
  // TODO
}

EffectConfig& Config::get_selected_effect() { return data[selected_effect]; }
const string& Config::get_selected_name() { return selected_effect; }
void Config::set_selected_name(const string& name) { selected_effect = name; }

}  // namespace mpv_glsl
