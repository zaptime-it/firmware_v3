#pragma once

#include <cstdint>
#include <memory>
#include <stddef.h>

// Owning logo buffer. Using a shared_ptr means LogoData can be safely copied
// and returned by value without leaking the heap-allocated bitmap the file
// loader hands back. Before this, `data` was a raw pointer that no one ever
// freed.
struct LogoData {
  std::shared_ptr<uint8_t[]> data;
  size_t width;
  size_t height;
  size_t size;
};
