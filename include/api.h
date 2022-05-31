#pragma once

#include <httplib.h>

#include <atomic>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

namespace mpv_glsl {
using namespace std;

class Api {
 public:
  Api();
  ~Api();
  optional<string> get_play_command();

 private:
  void server_run();

  httplib::Server server;
  thread server_thread;
  mutex command_mutex;
  optional<string> command;
};

}  // namespace mpv_glsl
