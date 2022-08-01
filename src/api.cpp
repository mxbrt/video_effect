#include "api.h"

#include <cstdio>
#include <optional>

#include "util.h"

namespace mpv_glsl {
using namespace std;
using namespace httplib;
Api::Api(const string& media_path, const string& website_path, int category)
    : media_path(media_path),
      website_path(website_path),
      command_pending(false),
      category(category) {
  server_thread = thread(&Api::server_run, this);
}

Api::~Api() {
  server.stop();
  server_thread.join();
}

void Api::server_run() {
  if (!server.set_mount_point("/", website_path)) {
    die("Failed to start file server\n");
  }

  if (!server.set_mount_point("/media", media_path)) {
    die("Failed to start file server\n");
  }

  server.Post("/play", [this](const Request& req, Response& res) {
    scoped_lock lock(command_mutex);
    command_pending = true;
    command = req.body;
    printf("Got play cmd %s\n", command.c_str());
  });

  server.Get("/play", [this](const Request& req, Response& res) {
    scoped_lock lock(command_mutex);
    res.body = command;
  });

  server.Get("/category", [this](const Request& req, Response& res) {
    res.body = to_string(category);
  });

  server.listen("localhost", 8000);
}

void Api::set_category(int id) {
  category = id;
}

void Api::set_play_cmd(const string& cmd) {
  scoped_lock lock(command_mutex);
  if (!command_pending) {
    command = cmd;
  }
}

optional<string> Api::get_play_cmd() {
  scoped_lock lock(command_mutex);
  if (command_pending) {
    command_pending = false;
    return command;
  } else {
    return std::nullopt;
  }
}
}  // namespace mpv_glsl
