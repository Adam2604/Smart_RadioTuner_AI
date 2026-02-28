#include <gtest/gtest.h>
#include "../WavWriter.h"
#include <vector>

using namespace std;

TEST(WavWriterTest, PoprawnieZliczaRozmiarDanych) {
    WavWriter wav("test_zapis.wav");
    vector<float> test_audio = {0.5f, -0.5f, 0.0f}; // 3 próbki
    wav.write(test_audio);

    // Każda z naszych 3 próbek zamienia się w 2-bajtowy PCM, więc spodziewamy się 6 bajtów.
    EXPECT_EQ(wav.data_size, 6); 
}