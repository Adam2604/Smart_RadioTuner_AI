#include "WavWriter.h"
#include <algorithm>

using namespace std;

WavWriter::WavWriter(const string &filename) {
    data_size = 0;
    file.open(filename, ios::binary);
    file.seekp(44);
}

WavWriter::~WavWriter() {
    file.seekp(0);
    file.write("RIFF", 4);
    int file_size = data_size + 36; file.write((char*)&file_size, 4);
    file.write("WAVEfmt ", 8);
    int fmt_size = 16; file.write((char*)&fmt_size, 4);
    short audio_fmt = 1, channels = 1;
    file.write((char*)&audio_fmt, 2); file.write((char*)&channels, 2);
    int sample_rate = 48000, byte_rate = 48000 * 2;
    file.write((char*)&sample_rate, 4); file.write((char*)&byte_rate, 4);
    short block_align = 2, bits_per_sample = 16;
    file.write((char*)&block_align, 2); file.write((char*)&bits_per_sample, 2);
    file.write("data", 4); file.write((char*)&data_size, 4);
    file.close();
}

void WavWriter::write(const vector<float> &audio) {
    for (float sample : audio) {
        int16_t pcm = (int16_t)(max(-1.0f, min(1.0f, sample)) * 32767.0f);
        file.write((char*)&pcm, 2);
        data_size += 2;
    }
}