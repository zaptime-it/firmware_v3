#include "nostrdisplay_handler.hpp"

std::array<std::string, NUM_SCREENS> parseZapNotify(std::uint16_t amount, bool withSatsSymbol)
{
    // Initialize defensively so it works for any NUM_SCREENS value (previously
    // a fixed 7-element initializer assumed NUM_SCREENS == 7).
    std::array<std::string, NUM_SCREENS> textEpdContent;
    textEpdContent.fill("");
    textEpdContent[0] = "ZAP";
    if (NUM_SCREENS > 1)
    {
        textEpdContent[1] = "mdi-lnbolt";
    }

    std::string text = std::to_string(amount);
    std::size_t textLength = text.length();

    // Clamp textLength so startIndex never underflows (size_t) even for
    // pathologically small NUM_SCREENS values.
    if (textLength >= NUM_SCREENS)
    {
        // Only the rightmost NUM_SCREENS chars will fit.
        text = text.substr(text.length() - NUM_SCREENS);
        textLength = text.length();
    }

    std::size_t startIndex = NUM_SCREENS - textLength;

    // Insert the sats symbol just before the digits
    if (startIndex > 0 && withSatsSymbol)
    {
        textEpdContent[startIndex - 1] = "STS";
    }

    // Place the digits
    for (std::size_t i = 0; i < textLength; i++)
    {
        textEpdContent[startIndex + i] = text.substr(i, 1);
    }

    return textEpdContent;
}