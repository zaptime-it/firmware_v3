#include "bitaxe_handler.hpp"

std::array<std::string, NUM_SCREENS> parseBitaxeHashRate(uint64_t hashrate)
{
    std::array<std::string, NUM_SCREENS> ret;
    ret.fill(""); // Initialize all elements to empty strings

    // Convert hashrate to GH/s and round to nearest integer
    double hashRateGH = static_cast<double>(hashrate) / std::pow(10, getHashrateMultiplier('G'));
    std::string hashRateStr = std::to_string(static_cast<uint64_t>(std::round(hashRateGH)));

    // Place the icons
    ret[0] = "mdi:bitaxe";
    ret[NUM_SCREENS - 1] = "GH/S";

    // Calculate the position where the digits should start
    std::size_t textLength = hashRateStr.length();
    std::size_t startIndex = NUM_SCREENS - 1 - textLength;

    // Insert the "mdi:pickaxe" icon just before the digits
    if (startIndex > 0)
    {
        ret[startIndex - 1] = "mdi:pickaxe";
    }

    // Place each digit
    for (std::size_t i = 0; i < textLength; ++i)
    {
        ret[startIndex + i] = std::string(1, hashRateStr[i]);
    }

    return ret;
}

std::array<std::string, NUM_SCREENS> parseBitaxeBestDiff(uint64_t difficulty)
{
    std::array<std::string, NUM_SCREENS> ret;
    ret.fill("");

    // Add icons at the start
    ret[0] = "mdi:bitaxe";
    ret[1] = "mdi:rocket";

    if (difficulty == 0) {
        ret[NUM_SCREENS - 1] = "0";
        return ret;
    }

    // Find the appropriate suffix and format the number
    const std::pair<char, int> suffixes[] = {
        {'Q', 15}, {'T', 12}, {'G', 9}, {'M', 6}, {'K', 3}
    };

    std::string text;
    for (const auto& suffix : suffixes) {
        if (difficulty >= std::pow(10, suffix.second)) {
            double value = difficulty / std::pow(10, suffix.second);
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "%.1f", value);
            text = buffer;
            // Remove trailing zeros and decimal point if not needed
            if (text.find('.') != std::string::npos) {
                text = text.substr(0, text.find_last_not_of('0') + 1);
                if (text.back() == '.') {
                    text.pop_back();
                }
            }
            text += suffix.first;
            break;
        }
    }

    if (text.empty()) {
        text = std::to_string(difficulty);
    }

    // Calculate start position to right-align the text
    std::size_t startIndex = NUM_SCREENS - text.length();
    
    // Place the formatted difficulty string
    for (std::size_t i = 0; i < text.length() && (startIndex + i) < NUM_SCREENS; ++i)
    {
        ret[startIndex + i] = std::string(1, text[i]);
    }

    return ret;
}

