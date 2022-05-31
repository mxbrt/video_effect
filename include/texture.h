#pragma once
namespace mpv_glsl {
class Texture {
 public:
  Texture(int width, int height);
  ~Texture();
  unsigned int id;
};
}  // namespace mpv_glsl
