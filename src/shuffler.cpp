#include "shuffler.h"

#include <algorithm>
#include <filesystem>
#include <random>

#include "util.h"

namespace fs = std::filesystem;

Shuffler::Shuffler(const std::vector<std::string>& directories)
    : cur_idx(0), directories(directories) {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng = std::default_random_engine{seed};
    read_directories();
    shuffle();
}

void Shuffler::read_directories() {
    files.clear();
    for (const auto& directory : directories) {
        for (const auto& f : fs::directory_iterator(directory)) {
            files.push_back(f.path());
        }
        if (files.size() == 0) {
            die("Expected at least one file in %s\n", directory.c_str());
        }
    }
}

void Shuffler::shuffle() {
    std::shuffle(std::begin(files), std::end(files), rng);
}

std::string Shuffler::get() {
    auto result = files[cur_idx];
    cur_idx++;
    if (cur_idx >= files.size()) {
        cur_idx = 0;
        read_directories();
        shuffle();
    }
    return result;
}

