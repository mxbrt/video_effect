project('mpv_glsl', 'c', 'cpp',
        default_options : ['c_std=c11', 'cpp_std=c++20'])

sources = [
  'src/fbo.cpp',
  'src/main.cpp',
  'src/player.c',
  'src/shader.cpp',
  'src/texture.cpp',
  'src/util.cpp',
  'src/vbo.cpp',
]

includes = [
  'include'
]

thirdparty_sources = [
  'vendor/glad/src/glad.c',
thirdparty_includes = [
  'vendor/glad/include',
]

dependencies = [ dependency('sdl2'), dependency('mpv') ]

executable('player',
  sources + thirdparty_sources,
  include_directories: include_directories(includes + thirdparty_includes),
  dependencies : dependencies)
