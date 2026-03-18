#include <iostream>
#include <rtl-sdr.h>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#include <thread>
#include <chrono>
#include <portaudio.h>
#include "WavWriter.h"
#include "filter.h"
#include <sstream>
#include <mutex>

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
int ostatni_kat = -1, ostatni_podkat = -1;
int reklama_counter = 0; // licznik kolejnych detekcji reklam
mutex state_mutex;

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

void http_server_thread() {
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "Blad bindowania serwera HTTP" << endl;
        return;
    }

    listen(server_socket, 5);
    cout << "Serwer HTTP uruchomiony na porcie 8080" << endl;

    while (keep_running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_socket, &readfds);

        timeval timeout = {1, 0};
        int activity = select(0, &readfds, NULL, NULL, &timeout);

        if (activity <= 0) continue;

        SOCKET client = accept(server_socket, NULL, NULL);
        if (client == INVALID_SOCKET) continue;

        char buffer[4096] = {0};
        int bytes = recv(client, buffer, sizeof(buffer), 0);

        if (bytes > 0) {
            string request(buffer);
            string response;
            string headers = "HTTP/1.1 200 OK\r\nAccess-Control-Allow-Origin: *\r\nContent-Type: application/json\r\n\r\n";

            if (request.find("GET /api/status") != string::npos) {
                lock_guard<mutex> lock(state_mutex);
                stringstream json;
                json << "{\"frequency\":" << frequency
                     << ",\"mode\":" << tryb_pracy
                     << ",\"target_genre\":" << cel_gatunek
                     << ",\"last_cat\":" << ostatni_kat
                     << ",\"last_subcat\":" << ostatni_podkat
                     << ",\"station_idx\":" << aktualna_stacja_idx << "}";
                response = headers + json.str();
            }
            else if (request.find("POST /api/frequency") != string::npos) {
                size_t body_pos = request.find("\r\n\r\n");
                if (body_pos != string::npos) {
                    string body = request.substr(body_pos + 4);
                    size_t freq_pos = body.find("\"freq\":");
                    if (freq_pos != string::npos) {
                        int new_freq = stoi(body.substr(freq_pos + 7));
                        lock_guard<mutex> lock(state_mutex);
                        frequency = new_freq;
                        response = headers + "{\"status\":\"ok\"}";
                    }
                }
            }
            else if (request.find("POST /api/mode") != string::npos) {
                size_t body_pos = request.find("\r\n\r\n");
                if (body_pos != string::npos) {
                    string body = request.substr(body_pos + 4);
                    size_t mode_pos = body.find("\"mode\":");
                    if (mode_pos != string::npos) {
                        int new_mode = stoi(body.substr(mode_pos + 7));
                        lock_guard<mutex> lock(state_mutex);
                        tryb_pracy = new_mode;

                        size_t genre_pos = body.find("\"genre\":");
                        if (genre_pos != string::npos) {
                            cel_gatunek = stoi(body.substr(genre_pos + 8));
                        }
                        response = headers + "{\"status\":\"ok\"}";
                    }
                }
            }
            else {
                response = headers + "{\"error\":\"unknown endpoint\"}";
            }

            send(client, response.c_str(), response.length(), 0);
        }

        closesocket(client);
    }

    closesocket(server_socket);
}

void input_thread(rtlsdr_dev_t *device) {
    string wejscie_tekst;
    cout << "\n--- MENU RADIA ---" << endl;
    cout << "Podaj czestotliwosc (np. 104.4) aby zmienic stacje" << endl;
    cout << "m1 - Wlacz tryb Anty-Reklama" << endl;
    cout << "m2 - Wlacz tryb Szukanie Gatunku" << endl;
    cout << "m0 - Wylacz automatyczne tryby" << endl;
    cout << "0  - Zakoncz program\n" << endl;

    while (true) {
        cin >> wejscie_tekst;

        if (wejscie_tekst == "0") {
            keep_running = false;
            cout << "Zamykanie programu..." << endl;
            break;
        }
        else if (wejscie_tekst == "m1") {
            lock_guard<mutex> lock(state_mutex);
            tryb_pracy = 1;
            cout << "TRYB ANTY-REKLAMA WLACZONY" << endl;
        }
        else if (wejscie_tekst == "m2") {
            tryb_pracy = 2;
            cout << "Podaj numer szukanego gatunku:" << endl;
            cout << "(0-Audycja, 1-Reklama, 2-Wiadomosci, 3-Lata80-00, 4-Pop, 5-PopRock, 6-Reggae, 7-RockAlternatywny): ";
            cin >> cel_gatunek;
            cout << "TRYB SZUKANIA GATUNKU WLACZONY (cel: " << cel_gatunek << ")" << endl;
        }
        else if (wejscie_tekst == "m0") {
            lock_guard<mutex> lock(state_mutex);
            tryb_pracy = 0;
            cout << "AUTOMATYKA WYLACZONA (Tryb reczny)" << endl;
        }
        else {
            try {
                // Próba odczytania tego jako częstotliwości
                double input_mhz = stod(wejscie_tekst);
                int new_freq = (int)(input_mhz * 1000000);
                lock_guard<mutex> lock(state_mutex);
                rtlsdr_set_center_freq(device, new_freq);
                frequency = new_freq;
                cout << "Zmieniono stacje na: " << input_mhz << " MHz" << endl;
                tryb_pracy = 0;
            } catch (...) {
                cout << "Nieznana komenda" << endl;
            }
        }
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

    thread http_thread(http_server_thread);
    http_thread.detach();

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

            {
                lock_guard<mutex> lock(state_mutex);
                ostatni_kat = kat;
                ostatni_podkat = podkat;
            }

            if (tryb_pracy == 1) {
                if (podkat == 1) {
                    reklama_counter++;
                    cout << "REKLAMA WYKRYTA (" << reklama_counter << "/2)..." << endl;
                    if (reklama_counter >= 2) {
                        cout << "Dwie reklamy z rzedu! Przelaczam na nastepna stacje..." << endl;
                        aktualna_stacja_idx = (aktualna_stacja_idx + 1) % stacje.size();
                        rtlsdr_set_center_freq(device, stacje[aktualna_stacja_idx]);
                        lock_guard<mutex> lock(state_mutex);
                        frequency = stacje[aktualna_stacja_idx];
                        reklama_counter = 0;
                    }
                } else {
                    reklama_counter = 0; // reset licznika gdy nie ma reklamy
                }
            }

            if (tryb_pracy == 2 && podkat != cel_gatunek) {
                cout << "To nie jest szukany gatunek. Szukam dalej..." << endl;
                aktualna_stacja_idx = (aktualna_stacja_idx + 1) % stacje.size();
                rtlsdr_set_center_freq(device, stacje[aktualna_stacja_idx]);
                lock_guard<mutex> lock(state_mutex);
                frequency = stacje[aktualna_stacja_idx];
            } else if (tryb_pracy == 2 && podkat == cel_gatunek) {
                cout << "ZNALAZLEM! Zostajemy na tej stacji." << endl;
                lock_guard<mutex> lock(state_mutex);
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
    closesocket(feedback_socket);
    WSACleanup();
    cout << "Urzadzenie zamkniete" << endl;
    return 0;
}