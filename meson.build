project('sendprotest', 'c', 'cpp',
        default_options : ['c_std=c11', 'cpp_std=c++20'])

add_global_arguments('-DIMGUI_IMPL_OPENGL_ES3', language : 'cpp')

sources = [
  'src/api.cpp',
  'src/config.cpp',
  'src/fbo.cpp',
  'src/gui.cpp',
  'src/main.cpp',
  'src/player.cpp',
  'src/shader.cpp',
  'src/shuffler.cpp',
  'src/texture.cpp',
  'src/util.cpp',
  'src/vbo.cpp',
]

includes = [
  'include'
]

thirdparty_sources = [
  'vendor/glad/src/glad.c',
  'vendor/imgui/src/imgui.cpp',
  'vendor/imgui/src/imgui_draw.cpp',
  'vendor/imgui/src/imgui_tables.cpp',
  'vendor/imgui/src/imgui_widgets.cpp',
  'vendor/imgui/src/imgui_impl_sdl.cpp',
  'vendor/imgui/src/imgui_impl_opengl3.cpp',
]

thirdparty_includes = [
  'vendor/glad/include',
  'vendor/imgui/include',
  'vendor/cpp-httplib',
  'vendor/json/include',
]

dependencies = [ dependency('sdl2'), dependency('mpv'), dependency('glesv2') ]



executable('sendprotest_player',
  sources + thirdparty_sources,
  include_directories: include_directories(includes + thirdparty_includes),
  dependencies : dependencies,
  install: true)

sendprotest_dir = '/opt/share/sendprotest'
install_subdir('shaders', install_dir : sendprotest_dir)
install_data('src/config.json', install_dir : '/etc/sendprotest')
