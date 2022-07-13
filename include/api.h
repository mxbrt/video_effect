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
  Api(const string& media_path, const string& website_path);
  ~Api();
  Api(const Api&) = delete;
  Api& operator=(const Api&) = delete;

  optional<string> get_play_cmd();
  void set_play_cmd(const string& cmd);

 private:
  void server_run();

  const string media_path;
  const string website_path;

  httplib::Server server;
  thread server_thread;
  mutex command_mutex;
  string command;
  bool command_pending;
};

}  // namespace mpv_glsl
