#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include <rom/miniz.h>

// Font metadata structure
struct FontData {
    const uint8_t* compressedData;
    const GFXglyph* glyphs;
    const size_t compressedSize;
    const size_t originalSize;
    const uint16_t first;
    const uint16_t last;
    const uint8_t yAdvance;
};


class FontLoader {
public:
    static GFXfont* loadCompressedFont(
        const uint8_t* compressedData,
        const GFXglyph* glyphs,
        const size_t compressedSize,
        const size_t originalSize,
        const uint16_t first,
        const uint16_t last,
        const uint8_t yAdvance) 
    {
        uint8_t* decompressedData = (uint8_t*)malloc(originalSize);
        if (!decompressedData) {
            Serial.println(F("Failed to allocate memory for font decompression"));
            return nullptr;
        }

        size_t decompressedSize = originalSize;
        if (GzipDecompressor::decompressData(compressedData, 
                                           compressedSize,
                                           decompressedData, 
                                           &decompressedSize))
        {
            GFXfont* font = (GFXfont*)malloc(sizeof(GFXfont));
            if (!font) {
                free(decompressedData);
                Serial.println(F("Failed to allocate memory for font structure"));
                return nullptr;
            }

            font->bitmap = decompressedData;
            font->glyph = (GFXglyph*)glyphs;
            font->first = first;
            font->last = last;
            font->yAdvance = yAdvance;

            return font;
        }

        Serial.println(F("Font decompression failed"));
        free(decompressedData);
        return nullptr;
    }

    static void unloadFont(GFXfont* font) {
        if (font) {
            if (font->bitmap) {
                free((void*)font->bitmap);
            }
            free(font);
        }
    }
};
