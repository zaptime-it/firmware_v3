// src/noderunners/noderunners_pool.cpp
#include "satoshi_radio_pool.hpp"

std::string SatoshiRadioPool::getApiUrl() const
{
    return "https://pool.satoshiradio.nl/api/v1/users/" + poolUser;
}
