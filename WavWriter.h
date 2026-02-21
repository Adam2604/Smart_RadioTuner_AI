#pragma once
#include <vector>
#include <string>
#include <fstream>

using namespace std;

class WavWriter {
    ofstream file;
    int data_size;
public:
    WavWriter(const string &filename);
    ~WavWriter();
    void write(const vector<float> &audio);
};


