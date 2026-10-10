#pragma once

// Shared between slang and C++
#ifdef __cplusplus
#include <cstdint>
    #define STRUCT_ALIGN(x) alignas(x)
#else
    // Slang
    #define STRUCT_ALIGN(x)
#endif

struct STRUCT_ALIGN(16) AstcParameters {
    uint8_t ep0[4];
    uint8_t ep1[4];
    uint8_t weights[16];
    bool use_alpha;
};

struct STRUCT_ALIGN(16) AstcDebug {
    uint32_t sum_error_squared;
    AstcParameters params;
};

struct STRUCT_ALIGN(16) AstcAnalysis {
    uint32_t count;
    uint64_t sum_error_squared;
    uint64_t sum_quantization_error_squared;
    uint64_t sum_quantization_alt_error_squared;
    uint64_t sum_color_spread_squared;
    uint64_t sum_weights;
    uint64_t sum_weights_squared;
    uint64_t quantized_weight_errors_3;
    uint64_t quantized_weight_errors_7;
    uint64_t quantized_weight_errors_15;
    uint64_t quantized_color_errors_47;
    uint64_t quantized_color_errors_191;
    uint64_t quantized_weight_errors_refined_3;
    uint64_t quantized_weight_errors_refined_7;
    uint64_t quantized_weight_errors_refined_15;
    uint64_t quantized_pixel_error_191_15_exact;
    uint64_t quantized_pixel_error_47_15_exact;
    uint64_t quantized_pixel_error_255_7_exact;
    uint64_t quantized_pixel_error_255_3_exact;
    uint64_t quantized_pixel_error_191_15_approx;
    uint64_t quantized_pixel_error_47_15_approx;
    uint64_t quantized_pixel_error_255_7_approx;
    uint64_t quantized_pixel_error_255_3_approx;
};
