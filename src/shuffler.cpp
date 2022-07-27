#include "shuffler.h"

#include <algorithm>
#include <filesystem>
#include <random>

#include "util.h"

namespace mpv_glsl {
using namespace std;

Shuffler::Shuffler(const vector<string>& directories)
    : cur_idx(0), directories(directories) {
    unsigned seed = chrono::system_clock::now().time_since_epoch().count();
    rng = default_random_engine{seed};
    read_directories();
}

void Shuffler::read_directories() {
    files.clear();
    for (const auto& directory : directories) {
        for (const auto& f : filesystem::directory_iterator(directory)) {
            files.push_back(f.path());
        }
    }
    if (files.size() == 0) {
        die("Expected at least one media file\n");
    }
}

void Shuffler::shuffle() { std::shuffle(begin(files), end(files), rng); }

vector<string> Shuffler::get() {
    shuffle();
    return files;
}
}  // namespace mpv_glsl
