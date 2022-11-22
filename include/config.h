#pragma once
#include <string>
#include <map>

auto const COMBINED_CATEGORY = 9;

namespace sendprotest {
using namespace std;

struct PlayerConfig {
  int playback_duration;
  int category;
  string selected_effect;
  };

  struct EffectConfig {
    float finger_radius;
    float effect_fade_in;
    float effect_amount;
  };

  class Config {
   public:
    Config(const string& json_path);

    map<string, EffectConfig>& get_effects();
    PlayerConfig& get_player_config();
    void save();

    EffectConfig& get_selected_effect();
    void set_random_effect();
    const string& get_selected_name();
    void set_selected_name(const string& name);

   private:
    map<string, EffectConfig> effect_data;
    PlayerConfig player_config;
    const string json_path;
  };
}  // namespace sendprotest
