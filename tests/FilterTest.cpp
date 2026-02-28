#include <gtest/gtest.h>
#include "../Filter.h"
#include <vector>
#include <cmath>

using namespace std;

TEST(FilterTest, TlumiWysokieCzestotliwosci) {
    int sample_rate = 48000; // Częstotliwość po Twojej decymacji
    int num_samples = 1000;
    vector<float> history(FIR_COEFFS.size(), 0.0f);
    float max_amplitude = 0.0f;

    for (int i = 0; i < num_samples; ++i) {
        float time = (float)i / sample_rate;
        float niska = sin(2.0f * 3.14159f * 1000.0f * time);
        float wysoka = sin(2.0f * 3.14159f * 20000.0f * time);

        float input_sample = niska + wysoka; // Połączone fale mają amplitudę ~2.0
        float output_sample = apply_fir_filter(input_sample, history);

        // Zapisujemy amplitudę, ale pomijamy pierwsze próbki (zanim bufor filtra się zapełni)
        if (i > 20) {
            max_amplitude = max(max_amplitude, abs(output_sample));
        }
    }

    // Filtr powinien wyciąć szum (wysoką falę), więc amplituda musi spaść z powrotem do ok. 1.0
    EXPECT_LT(max_amplitude, 1.2f);
}