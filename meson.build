project('mpv_glsl', 'cpp')

sources = [
  'src/fbo.cpp',
  'src/glad.c',
  'src/main.cpp',
  'src/player.c',
  'src/shader.cpp',
  'src/texture.cpp',
  'src/util.cpp',
  'src/vbo.cpp',
]
dependencies = [ dependency('sdl2'), dependency('mpv') ]

executable('player',
  sources,
  include_directories: include_directories('include'),
  dependencies : dependencies)
