import wave
import os


def tnij_plik(input_file, output_folder, chunk_length_s=3):
    """Tnie plik WAV na równe kawałki o zadanej długości (w sekundach).

    Zwraca listę ścieżek do utworzonych chunków.
    """
    os.makedirs(output_folder, exist_ok=True)
    base_name = os.path.splitext(os.path.basename(input_file))[0]
    utworzone_pliki = []

    with wave.open(input_file, 'rb') as infile:
        nchannels = infile.getnchannels()
        sampwidth = infile.getsampwidth()
        framerate = infile.getframerate()
        nframes = infile.getnframes()

        frames_per_chunk = framerate * chunk_length_s
        total_chunks = nframes // frames_per_chunk

        print(f"Tnę na {total_chunks} kawałków")

        for i in range(total_chunks):
            chunk_data = infile.readframes(frames_per_chunk)
            filename = os.path.join(output_folder, f"{base_name}_chunk_{i:04d}.wav")

            with wave.open(filename, 'wb') as outfile:
                outfile.setnchannels(nchannels)
                outfile.setsampwidth(sampwidth)
                outfile.setframerate(framerate)
                outfile.writeframes(chunk_data)

            utworzone_pliki.append(filename)

    print("Gotowe!")
    return utworzone_pliki


if __name__ == "__main__":
    INPUT_FILE = "pop1.wav"
    OUTPUT_FOLDER = "dataset/muzyka_pop"
    CHUNK_LENGTH_S = 3
    tnij_plik(INPUT_FILE, OUTPUT_FOLDER, CHUNK_LENGTH_S)