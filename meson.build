project('mpv_glsl', 'c')

sources = [
  'src/fbo.c',
  'src/glad.c',
  'src/main.c',
  'src/player.c',
  'src/shader.c',
  'src/texture.c',
  'src/util.c',
  'src/vbo.c',
]
dependencies = [ dependency('sdl2'), dependency('mpv') ]

executable('player',
  sources,
  include_directories: include_directories('include'),
  dependencies : dependencies)
