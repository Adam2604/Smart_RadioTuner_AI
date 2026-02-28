#pragma once
#include <vector>

using namespace std;

const vector<float> FIR_COEFFS = {
    -0.014f, -0.024f, 0.0f, 0.058f, 0.144f, 0.233f, 0.268f,
    0.233f, 0.144f, 0.058f, 0.0f, -0.024f, -0.014f
};

inline float apply_fir_filter(float raw_audio, vector<float>& history) {
    history.insert(history.begin(), raw_audio);
    history.pop_back();

    float fir_out = 0.0f;
    for (size_t j = 0; j < FIR_COEFFS.size(); j++) {
        fir_out += history[j] * FIR_COEFFS[j];
    }
    return fir_out;
}