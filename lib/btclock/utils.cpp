#include "utils.hpp"

int modulo(int x, int N)
{
    return (x % N + N) % N;
}

double getSupplyAtBlock(std::uint32_t blockNr)
{
    if (blockNr >= 33 * 210000)
    {
        return 20999999.9769;
    }

    const int initialBlockReward = 50;  // Initial block reward
    const int halvingInterval = 210000; // Number of blocks before halving

    int halvingCount = blockNr / halvingInterval;
    double totalBitcoinInCirculation = 0;

    for (int i = 0; i < halvingCount; ++i)
    {
        totalBitcoinInCirculation += halvingInterval * initialBlockReward * std::pow(0.5, i);
    }

    totalBitcoinInCirculation += (blockNr % halvingInterval) * initialBlockReward * std::pow(0.5, halvingCount);

    return totalBitcoinInCirculation;
}

std::string formatNumberWithSuffix(std::uint64_t num, int numCharacters) 
{
    return formatNumberWithSuffix(num, numCharacters, false);
}

std::string formatNumberWithSuffix(std::uint64_t num, int numCharacters, bool mowMode)
{
    static char result[20]; // Adjust size as needed
    const long long quadrillion = 1000000000000000LL;
    const long long trillion = 1000000000000LL;
    const long long billion = 1000000000;
    const long long million = 1000000;
    const long long thousand = 1000;

    double numDouble = (double)num;
    int numDigits = (int)log10(num) + 1;
    char suffix;

    if (num >= quadrillion || numDigits > 15)
    {
        numDouble /= quadrillion;
        suffix = 'Q';
    }
    else if (num >= trillion || numDigits > 12)
    {
        numDouble /= trillion;
        suffix = 'T';
    }
    else if (num >= billion || numDigits > 9)
    {
        numDouble /= billion;
        suffix = 'B';
    }
    else if (num >= million || numDigits > 6 || (mowMode && num >= thousand))
    {
        numDouble /= million;
        suffix = 'M';
    }
    else if (!mowMode && (num >= thousand || numDigits > 3))
    {
        numDouble /= thousand;
        suffix = 'K';
    }
    else if (!mowMode)
    {
        snprintf(result, sizeof(result), "%llu", (unsigned long long)num);
        return result;
    }
    else // mowMode is true and num < 1000
    {
        numDouble /= million;
        suffix = 'M';
    }

    // Add suffix
    int len;

    // Mow Mode always uses string truncation to avoid rounding
    std::string mowAsString = std::to_string(numDouble);
    if (mowMode) {
        // Default to one decimal place
        len = snprintf(result, sizeof(result), "%s%c", mowAsString.substr(0, mowAsString.find(".") + 2).c_str(), suffix);
    }
    else
    {
        len = snprintf(result, sizeof(result), "%.0f%c", numDouble, suffix);
    }

    // If there's room, add more decimal places
    if (len < numCharacters)
    {
        int restLen = mowMode ? numCharacters - len : numCharacters - len - 1;

        if (mowMode) {
            snprintf(result, sizeof(result), "%s%c", mowAsString.substr(0, mowAsString.find(".") + 2 + restLen).c_str(), suffix);
        }
        else
        {
            snprintf(result, sizeof(result), "%.*f%c", restLen, numDouble, suffix);
        }
    }

    return result;
}

/**
 * Get sat amount from a bolt11 invoice 
 * 
 * Based on https://github.com/lnbits/nostr-zap-lamp/blob/main/nostrZapLamp/nostrZapLamp.ino
 */
int64_t getAmountInSatoshis(std::string bolt11) {
    int64_t number = -1;
    char multiplier = ' ';

    for (unsigned int i = 0; i < bolt11.length(); ++i) {
        if (isdigit(bolt11[i])) {
            number = 0;
            while (isdigit(bolt11[i])) {
                number = number * 10 + (bolt11[i] - '0');
                ++i;
            }
            for (unsigned int j = i; j < bolt11.length(); ++j) {
                if (isalpha(bolt11[j])) {
                    multiplier = bolt11[j];
                    break;
                }
            }
            break;
        }
    }

    if (number == -1 || multiplier == ' ') {
        return -1;
    }

    int64_t satoshis = number;

    switch (multiplier) {
        case 'm':
            satoshis *= 100000; // 0.001 * 100,000,000
            break;
        case 'u':
            satoshis *= 100; // 0.000001 * 100,000,000
            break;
        case 'n':
            satoshis /= 10; // 0.000000001 * 100,000,000
            break;
        case 'p':
            satoshis /= 10000; // 0.000000000001 * 100,000,000
            break;
        default:
            return -1;
    }

    return satoshis;
}

void parseHashrateString(const std::string& hashrate, std::string& label, std::string& output, unsigned int maxCharacters) {
    // Handle empty string or "0" cases
    if (hashrate.empty() || hashrate == "0") {
        label = "H/S";
        output = "0";
        return;
    }

    size_t suffixLength = 0;
    if (hashrate.length() > 21) {
        label = "ZH/S";
        suffixLength = 21;
    } else if (hashrate.length() > 18) {
        label = "EH/S";
        suffixLength = 18;
    } else if (hashrate.length() > 15) {
        label = "PH/S";
        suffixLength = 15;
    } else if (hashrate.length() > 12) {
        label = "TH/S";
        suffixLength = 12;
    } else if (hashrate.length() > 9) {
        label = "GH/S";
        suffixLength = 9;
    } else if (hashrate.length() > 6) {
        label = "MH/S";
        suffixLength = 6;
    } else if (hashrate.length() > 3) {
        label = "KH/S";
        suffixLength = 3;
    } else {
        label = "H/S";
        suffixLength = 0;
    }

        double value = std::stod(hashrate) / std::pow(10, suffixLength);

        // Calculate integer part length
        int integerPartLength = std::to_string(static_cast<int>(value)).length();

        // Calculate remaining space for decimals
        int remainingSpace = maxCharacters - integerPartLength;

        char buffer[32];
        if (remainingSpace <= 0)
        {
            // No space for decimals, just round to integer
            snprintf(buffer, sizeof(buffer), "%.0f", value);
        }
        else
        {
            // Space for decimal point and some decimals
            snprintf(buffer, sizeof(buffer), "%.*f", remainingSpace - 1, value);
        }

        // Remove trailing zeros and decimal point if necessary
        output = buffer;
        if (output.find('.') != std::string::npos)
        {
            output = output.substr(0, output.find_last_not_of('0') + 1);
            if (output.back() == '.')
            {
                output.pop_back();
            }
        }
  
}

int getHashrateMultiplier(char unit) {
    if (unit == '0')
        return 0;

    static const std::unordered_map<char, int> multipliers = {
        {'Z', 21}, {'E', 18}, {'P', 15}, {'T', 12},
        {'G', 9},  {'M', 6},  {'K', 3}
    };
    return multipliers.at(unit);
}

int getDifficultyMultiplier(char unit) {
    if (unit == '0')
        return 0;

    static const std::unordered_map<char, int> multipliers = {
        {'Q', 15}, {'T', 12}, {'B', 9}, {'M', 6}, {'K', 3}, {'G', 9},
        {'q', 15}, {'t', 12}, {'b', 9}, {'m', 6}, {'k', 3}, {'g', 9}
    };
    return multipliers.at(unit);
}
