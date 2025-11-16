#pragma once

#include <string>
#include <cstdint>

bool load_tiles_texture(const std::string &path);
uint32_t get_tiles_texture_id();
int get_tiles_cols();
int get_tiles_rows();

// returns whether texture loaded
bool tiles_texture_available();
