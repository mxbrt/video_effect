#include "api.h"

namespace mpv_glsl {
using namespace std;
using namespace httplib;
Api::Api() { server_thread = thread(&Api::server_run, this); }
Api::~Api() {
  server.stop();
  server_thread.join();
}

void Api::server_run() {
  server.Post("/play", [this](const Request& req, Response& res) {
    scoped_lock lock(command_mutex);
    command = req.body;
  });

  server.listen("localhost", 8000);
}

optional<string> Api::get_play_command() {
  scoped_lock lock(command_mutex);
  auto command_copy = command;
  command = std::nullopt;
  return command_copy;
}
}  // namespace mpv_glsl
