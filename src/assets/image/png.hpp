#pragma once

#include <vector>
#include <string>
#include <filesystem>

// This file actually supports any image file decodable by STB_IMAGE, I should probably change the name!

struct Image;

// Just do magic number check
bool is_png(std::vector<uint8_t> buffer);


// ID is to be used in error output when the data comes from a buffer,
// to help use identify what files etc. are causing issues.
std::unique_ptr<Image> load_png(std::vector<uint8_t> buffer, std::string id = "");
std::unique_ptr<Image> load_png(std::filesystem::path path);
