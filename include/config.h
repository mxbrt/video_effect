#pragma once
#include <string>
#include <unordered_map>

namespace mpv_glsl {
using namespace std;

struct EffectConfig {
  float finger_radius;
  float effect_fade_in;
  float effect_amount;
};

class Config {
 public:
  Config(const string& json_path);

  unordered_map<string, EffectConfig>& get();
  void save();

  EffectConfig& get_selected_effect();
  const string& get_selected_name();
  void set_selected_name(const string& name);

 private:
  unordered_map<string, EffectConfig> data;
  string selected_effect;
};
}  // namespace mpv_glsl
