project('mpv_glsl', 'c')

sources = [ 'src/main.c', 'src/glad.c' ]
dependencies = [ dependency('sdl2'), dependency('mpv') ]

executable('player',
  sources,
  include_directories: include_directories('include'),
  dependencies : dependencies)

