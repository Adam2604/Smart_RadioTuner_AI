#include <iostream>
#include <rtl-sdr.h>
#include <vector>
#include <winsock2.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <chrono>
#include <portaudio.h>
#include "WavWriter.h"
#include "filter.h"

using namespace std;

int frequency = 94600000;
const int sample_rate = 1152000;
const int gain = 0;
const int buffer_size = 196608;
const int decimation = 24; // bierze co 24 pare
const int audio_rate = 48000;

double previous_phase = 0.0;
double previous_audio = 0.0;

bool keep_running = true;

vector<int> stacje = {91890000, 92290000, 94600000, 96400000, 99390000, 100900000, 101790000, 103700000, 105100000, 106790000};
int aktualna_stacja_idx = 0;
int tryb_pracy = 0; // 1: Anty-Reklama, 2: Szukanie Gatunku
int cel_gatunek = -1;

void demodulate_fm(const vector<uint8_t>& input_iq, vector<float>& output_audio) {
    output_audio.clear();

    static double phase_sum = 0.0;
    static int count = 0;
    double PI = 3.14159265358979323846;
    static vector<float> audio_history(FIR_COEFFS.size(), 0.0f);

    for (size_t i = 0; i < input_iq.size(); i += 2) {
        double I = (double)input_iq[i] - 127.5;
        double Q = (double)input_iq[i+1] - 127.5;

        double current_phase = atan2(Q, I);
        double phase_diff = current_phase - previous_phase;

        while (phase_diff > PI)  phase_diff -= 2 * PI;
        while (phase_diff < -PI) phase_diff += 2 * PI;

        previous_phase = current_phase;
        phase_sum += phase_diff;
        count++;

        if (count == decimation) {
            float raw_audio = (float)((phase_sum / count) * 3.0);

            // Filtr FIR
            static vector<float> audio_history(FIR_COEFFS.size(), 0.0f);
            float fir_out = apply_fir_filter(raw_audio, audio_history);

            // Deemfaza
            float final_audio = (fir_out * 0.2f) + (previous_audio * 0.8f);
            previous_audio = final_audio;

            output_audio.push_back(final_audio);
            phase_sum = 0.0;
            count = 0;
        }
    }
}

void input_thread(rtlsdr_dev_t *device) {
    double input_mhz;
    while (true) {
        cin >> input_mhz;
        if (input_mhz == 0.0) {
            keep_running = false;
            cout << "Zamykanie programu i zapisywanie pliku..." << endl;
            break;
        }
        int new_freq = (int)(input_mhz * 1000000);
        rtlsdr_set_center_freq(device, new_freq);
        cout << "Zmieniono stacje na: " << input_mhz << " MHz" << endl;
    }
}

int main() {
    // Uruchomienie gniazd UDP w Windowsie
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
    SOCKET udp_socket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(5005);
    server.sin_addr.s_addr = inet_addr("127.0.0.1");

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

    cout << "Wpisz czestotliwosc (lub 0 zeby wyjsc) w MHz i nacisnij Enter: " << endl;
    thread console_thread(input_thread, device);
    console_thread.detach();

    WavWriter wav("drugie_nagranie.wav");

    SOCKET feedback_socket = socket(AF_INET, SOCK_DGRAM, 0);
    sockaddr_in local_addr;
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(5006);
    local_addr.sin_addr.s_addr = INADDR_ANY;
    bind(feedback_socket, (sockaddr*)&local_addr, sizeof(local_addr));

    // Ustawienie trybu nieblokującego
    u_long mode = 1;
    ioctlsocket(feedback_socket, FIONBIO, &mode);

    char recv_buf[32];
    while (keep_running) {
        rtlsdr_read_sync(device, buffer.data(), buffer.size(), &n_read);
        demodulate_fm(buffer, audio_buffer);
        wav.write(audio_buffer);
        Pa_WriteStream(stream, audio_buffer.data(), audio_buffer.size());

        //Wysylanie dzwieku przez UDP
        sendto(udp_socket, (const char*) audio_buffer.data(), audio_buffer.size() * sizeof(float), 0, (sockaddr*) &server, sizeof(server));

        int bytes = recv(feedback_socket, recv_buf, sizeof(recv_buf), 0);
        if (bytes > 0) {
            recv_buf[bytes] = '\0';
            int kat, podkat;
            sscanf(recv_buf, "%d,%d", &kat, &podkat);

            if (tryb_pracy == 1 && podkat == 1) {
                cout << "!!! REKLAMA WYKRYTA !!! Przełączam na następną stację..." << endl;
                aktualna_stacja_idx = (aktualna_stacja_idx + 1) % stacje.size();
                rtlsdr_set_center_freq(device, stacje[aktualna_stacja_idx]);
            }

            if (tryb_pracy == 2 && podkat != cel_gatunek) {
                cout << "To nie jest szukany gatunek. Szukam dalej..." << endl;
                aktualna_stacja_idx = (aktualna_stacja_idx + 1) % stacje.size();
                rtlsdr_set_center_freq(device, stacje[aktualna_stacja_idx]);
            } else if (tryb_pracy == 2 && podkat == cel_gatunek) {
                cout << "ZNALAZŁEM! Zostajemy na tej stacji." << endl;
                tryb_pracy = 0; // Wracamy do trybu normalnego
            }
        }
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    rtlsdr_close(device);

    //Zamkniecie UDP
    closesocket(udp_socket);
    WSACleanup();
    cout << "Urzadzenie zamkniete" << endl;
    return 0;
}