#include <iostream>
#include <rtl-sdr.h>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>
#include <portaudio.h>

using namespace std;

const int frequency = 94600000;
const int sample_rate = 2048000;
const int gain = 0;
const int buffer_size = 16 * 16384;
const int decimation = 24; // bierze co 24 pare
const int audio_rate = (sample_rate / 2) / decimation;

double previous_phase = 0.0;

void demodulate_fm(const vector<uint8_t> &input_iq, vector<float> &output_audio) {
    output_audio.clear();
    const double PI = 3.14159265358979323846;

    for (size_t i = 0; i < input_iq.size(); i += 2) {
        double I = (double)input_iq[i] - 127.5;
        double Q = (double)input_iq[i + 1] - 127.5;
        double current_phase = atan2(Q, I);
        double phase_diff = current_phase - previous_phase;
        // Korekta granicy kóła, zapobiega ogromnym trzaskom, gdy kąt przeskakuje z -179 na +179
        while (phase_diff > PI) phase_diff -= 2 * PI;
        while (phase_diff < -PI) phase_diff += 2 * PI;
        previous_phase = current_phase;

        output_audio.push_back((float)phase_diff);
    }
}

int main() {
    rtlsdr_dev_t *device = nullptr;
    int device_index = 0;
    int status = rtlsdr_open(&device, device_index);

    if (status < 0) {
        cout << "Blad podczas otwierania urzadzenia RTL-SDR" << endl;
        return -1;
    }
    cout << "Urzadzenie otwarto pomyslnie" << endl;

    // Konfiguracja sprzetu
    rtlsdr_set_sample_rate(device, sample_rate);
    cout << "Probkowanie ustawione na: " << sample_rate << " Hz" << endl;

    rtlsdr_set_center_freq(device, frequency);
    cout << "Czestotliwosc ustawiona na: " << frequency << " Hz" << endl;

    rtlsdr_set_tuner_gain_mode(device, 0); // tryb auto
    rtlsdr_reset_buffer(device);
    cout << "Radio gotowe do odbioru" << endl;

    Pa_Initialize();
    PaStream *stream;
    Pa_OpenDefaultStream(&stream, 0, 1, paFloat32, audio_rate, paFramesPerBufferUnspecified, nullptr, nullptr);
    Pa_StartStream(stream);
    vector<uint8_t> buffer(buffer_size);
    vector<float> audio_buffer;
    int n_read = 0; // ile zostalo odczytane

    while (true) {
        rtlsdr_read_sync(device, buffer.data(), buffer.size(), &n_read);
        demodulate_fm(buffer, audio_buffer);
        Pa_WriteStream(stream, audio_buffer.data(), audio_buffer.size());
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    rtlsdr_close(device);
    cout << "Urzadzenie zamkniete" << endl;
    return 0;
}
