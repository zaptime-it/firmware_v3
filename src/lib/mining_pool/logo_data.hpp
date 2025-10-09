#pragma once

#include <cstdint>
#include <stddef.h>

struct LogoData {
    const uint8_t* data;
    size_t width;
    size_t height;  
    size_t size;
};
