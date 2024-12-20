// src/noderunners/noderunners_pool.cpp
#include "gobrrr_pool.hpp"

std::string GoBrrrPool::getApiUrl() const {
    return "https://pool.gobrrr.me/api/client/" + poolUser;
}