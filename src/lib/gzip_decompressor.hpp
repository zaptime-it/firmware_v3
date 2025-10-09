#pragma once

#include "rom/miniz.h"
#include <Arduino.h>

class GzipDecompressor {
public:
    static bool decompressData(const uint8_t* input, size_t inputSize, 
                             uint8_t* output, size_t* outputSize) {
        if (!input || !output || !outputSize || inputSize < 18) { // Minimum gzip size
            return false;
        }

        tinfl_decompressor* decomp = new tinfl_decompressor;
        if (!decomp) {
            return false;
        }

        tinfl_init(decomp);
        
        size_t inPos = 10; // Skip gzip header
        size_t outPos = 0;
        
        while (inPos < inputSize - 8) { // -8 for footer
            size_t inBytes = inputSize - inPos - 8;
            size_t outBytes = *outputSize - outPos;
            
            tinfl_status status = tinfl_decompress(decomp,
                &input[inPos], &inBytes,
                output, &output[outPos], &outBytes,
                TINFL_FLAG_HAS_MORE_INPUT | TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);

            inPos += inBytes;
            outPos += outBytes;

            if (status == TINFL_STATUS_DONE) {
                *outputSize = outPos;
                delete decomp;
                return true;
            } else if (status < 0) {
                delete decomp;
                return false;
            }
        }
        
        delete decomp;
        return false;
    }
};