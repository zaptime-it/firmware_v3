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

    // Insert the pickaxe icon just before the digits
    if (startIndex > 0)
    {
        ret[startIndex - 1] = "mdi:pickaxe";
    }

    // Place the digits
    for (std::size_t i = 0; i < textLength; ++i)
    {
        ret[startIndex + i] = hashrate.substr(i, 1);
    }

    ret[NUM_SCREENS - 1] = label;

    // Set the mining pool logo
    if (miningPoolName == "ocean") {
        ret[0] = "mdi:ocean_logo";
    } else if (miningPoolName == "braiins") {
        ret[0] = "mdi:braiins_logo";
    }

    return ret;
}


std::array<std::string, NUM_SCREENS> parseMiningPoolStatsDailyEarnings(std::string miningPoolName, int sats)
{
    std::array<std::string, NUM_SCREENS> ret;
    ret.fill(""); // Initialize all elements to empty strings
    std::string satsDisplay = std::to_string(sats);
    std::string label;

    if (miningPoolName == "braiins") {
        // fpps guarantees payout; report current daily earnings
        label = "sats/earned";
    } else {
        // TIDES can only estimate earnings on the next block Ocean finds
        label = "sats/block";
    }

    if (sats >= 100000000) {
        // A whale mining 1+ BTC per day! No decimal points; whales scoff at such things.
        label = "BTC" + label.substr(4);
        satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 8);
    } else if (sats >= 10000000) {
        // 10.0M to 99.9M you get one decimal point
        satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 6) + "." + satsDisplay[2] + "M";
    } else if (sats >= 1000000) {
        // 1.00M to 9.99M you get two decimal points
        satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 6) + "." + satsDisplay.substr(2, 2) + "M";
    } else if (sats >= 100000) {
        // 100K to 999K you get no extra precision
        satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 3) + "K";
    } else if (sats >= 10000) {
        // 10.0K to 99.9K you get one decimal point
        satsDisplay = satsDisplay.substr(0, satsDisplay.length() - 3) + "." + satsDisplay[2] + "K";
    } else {
        // Pleb miner! 4 digit or fewer sats will fit as-is. no-op.
    }

    std::size_t textLength = satsDisplay.length();

    // Calculate the position where the digits should start
    // Account for the position of the mining pool logo
    std::size_t startIndex = NUM_SCREENS - 1 - textLength;

    // Insert the pickaxe icon just before the digits if there's room
    if (startIndex > 0)
    {
        ret[startIndex - 1] = "mdi:pickaxe";
    }

    // Place the digits
    for (std::size_t i = 0; i < textLength; ++i)
    {
        ret[startIndex + i] = satsDisplay.substr(i, 1);
    }

    ret[NUM_SCREENS - 1] = label;

    // Set the mining pool logo
    if (miningPoolName == "ocean") {
        ret[0] = "mdi:ocean_logo";
    } else if (miningPoolName == "braiins") {
        ret[0] = "mdi:braiins_logo";
    }

    return ret;
}