#include "texture.hpp"
#include <cstdio>
#include <sys/stat.h>

static bool available = false;
static int cols = 3, rows = 1;

bool load_tiles_texture(const std::string &path) {
  struct stat st;
  if (stat(path.c_str(), &st) == 0) {
    fprintf(stderr, "[texture] found asset: %s (size=%ld) - but image loader not enabled\n", path.c_str(), (long)st.st_size);
    available = false; // keep false until loader implemented
    return true;
  }
  fprintf(stderr, "[texture] asset not found: %s\n", path.c_str());
  available = false;
  return false;
}

uint32_t get_tiles_texture_id() { return 0; }
int get_tiles_cols() { return cols; }
int get_tiles_rows() { return rows; }
bool tiles_texture_available() { return available; }
