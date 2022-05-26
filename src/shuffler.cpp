#include "shuffler.h"

#include <algorithm>
#include <filesystem>
#include <random>

#include "util.h"

namespace fs = std::filesystem;

Shuffler::Shuffler(const std::string& directory_path)
    : cur_idx(0), directory_path(directory_path) {
    for (const auto& f : fs::directory_iterator(directory_path)) {
        files.push_back(f.path());
    }
    if (files.size() == 0) {
        die("Expected at least one file in %s\n", directory_path.c_str());
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng = std::default_random_engine{seed};
    shuffle();
}

void Shuffler::shuffle() {
    std::shuffle(std::begin(files), std::end(files), rng);
}

std::string Shuffler::get() {
    auto result = files[cur_idx];
    cur_idx++;
    if (cur_idx >= files.size()) {
        cur_idx = 0;
        shuffle();
    }
    return result;
}

