#include <iostream>
#include <rtl-sdr.h>

using namespace std;

const int frequency = 98500000;
const int sample_rate = 2048000;
const int gain = 0;

int main() {
    rtlsdr_dev *device = nullptr;
    int device_index = 0;
    int status = rtlsdr_open(&device, device_index);

    if (status < 0) {
        cout << "Bład podczas otwierania urzadzenia RTL-SDR" << endl;
        return -1;
    }
    cout << "Urzadzenie otwarto pomyslnie" << endl;

    // Konfiguracja sprzetu
    rtlsdr_set_sample_rate(device, sample_rate);
    cout << "Probkowanie ustawione na: " << sample_rate << " Hz" << endl;

    rtlsdr_set_center_freq(device, frequency);
    cout << "Czestotliwosc ustawiona na: " << frequency << " Hz" << endl;

    rtlsdr_reset_buffer(device);
    cout << "Radio gotowe do odbioru" << endl;


    rtlsdr_close(device);
    cout << "Urzadzenie zamkniete" << endl;
    return 0;
}