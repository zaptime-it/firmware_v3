#include "mining_pool_stats_handler.hpp"
#include <iostream>

std::array<std::string, NUM_SCREENS> parseMiningPoolStatsHashRate(std::string miningPoolName, std::string text)
{
    std::array<std::string, NUM_SCREENS> ret;
    ret.fill(""); // Initialize all elements to empty strings
    std::string hashrate;
    std::string label;

    if (text.length() > 21) {
        // We are massively future-proof!!
        label = "ZH/S";
        hashrate = text.substr(0, text.length() - 21);
    } else if (text.length() > 18) {
        label = "EH/S";
        hashrate = text.substr(0, text.length() - 18);
    } else if (text.length() > 15) {
        label = "PH/S";
        hashrate = text.substr(0, text.length() - 15);
    } else if (text.length() > 12) {
        label = "TH/S";
        hashrate = text.substr(0, text.length() - 12);
    } else if (text.length() > 9) {
        label = "GH/S";
        hashrate = text.substr(0, text.length() - 9);
    } else if (text.length() > 6) {
        label = "MH/S";
        hashrate = text.substr(0, text.length() - 6);
    } else if (text.length() > 3) {
        label = "KH/S";
        hashrate = text.substr(0, text.length() - 3);
    } else {
        label = "H/S";
        hashrate = text;
    }

    std::size_t textLength = hashrate.length();

    // Calculate the position where the digits should start
    // Account for the position of the mining pool logo and the hashrate label
    std::size_t startIndex = NUM_SCREENS - 1 - textLength;

    std::cout << "miningPoolName: " << miningPoolName << std::endl;

    // Insert the mining pool logo icon just before the digits
    if (startIndex > 0)
    {
        if (miningPoolName == "ocean") {
            ret[startIndex - 1] = "mdi:braiins_logo";
        } else if (miningPoolName == "braiins") {
            ret[startIndex - 1] = "mdi:braiins_logo";
        }
    }

    // Place the digits
    for (std::size_t i = 0; i < textLength; ++i)
    {
        ret[startIndex + i] = hashrate.substr(i, 1);
    }

    ret[NUM_SCREENS - 1] = label;
    ret[0] = "POOL/STATS";

    return ret;
}