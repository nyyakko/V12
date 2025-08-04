#pragma once

#include <liberror/Result.hpp>

#include <vector>
#include <filesystem>

liberror::Result<std::vector<uint8_t>> assemble(std::filesystem::path source);
