// src/noderunners/noderunners_pool.cpp
#include "gobrrr_pool.hpp"

std::string GoBrrrPool::getApiUrl() const {
    return "https://pool.gobrrr.me/api/client/" + poolUser;
}

LogoData GoBrrrPool::getLogo() const {
    return LogoData {
        .data = epd_icons_allArray[7],
        .width = 122,
        .height = 122
    };
}
