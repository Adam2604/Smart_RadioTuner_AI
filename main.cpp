#include <iostream>
#include <rtl-sdr.h>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

using namespace std;

const int frequency = 98500000;
const int sample_rate = 2048000;
const int gain = 0;
const int buffer_size = 16 * 16384;

int main() {
    rtlsdr_dev *device = nullptr;
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

    vector<uint8_t> buffer(buffer_size);
    int n_read = 0; // ile zostalo odczytane

    while (true) {
        int result = rtlsdr_read_sync(device, buffer.data(), buffer_size, &n_read);
        if (result < 0) {
            cout << "Blad odczytu danych" << endl;
            break;
        }
        if (n_read < buffer_size) {
            cout << "Uwaga: Odebrano niepelna ramke danych" << endl;
        }
        // Obliczenie sily sygnalu
        double sum_squares = 0.0;
        for (int i = 0; i < n_read; i++) {
            //127.5 to "cisza", wiec to trzeba traktowac jako zero
            double sample = (double)buffer[i] - 127.5;
            sum_squares += sample * sample;
        }
        double rms = sqrt(sum_squares / n_read);
        double db = 20 * log10(rms);

        cout << "\rSila sygnalu: [";
        int bar_width = (int)db;
        if (bar_width < 0) bar_width = 0;
        if (bar_width > 50) bar_width = 50;

        for (int i = 0; i < bar_width; i++) {
            if (i < bar_width) cout << "#";
            else cout << " ";
        }
        cout << "] " << int(db) <<" dB (RMS: " << int(rms) << ") " << flush;
    }

    rtlsdr_close(device);
    cout << "Urzadzenie zamkniete" << endl;
    return 0;
}