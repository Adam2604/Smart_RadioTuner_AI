import wave
import os

INPUT_FILE = "pop1.wav"      
OUTPUT_FOLDER = "dataset/muzyka_pop"     
CHUNK_LENGTH_S = 3

os.makedirs(OUTPUT_FOLDER, exist_ok=True)

base_name = os.path.splitext(os.path.basename(INPUT_FILE))[0]

with wave.open(INPUT_FILE, 'rb') as infile:
    nchannels = infile.getnchannels()
    sampwidth = infile.getsampwidth()
    framerate = infile.getframerate()
    nframes = infile.getnframes()
    
    frames_per_chunk = framerate * CHUNK_LENGTH_S
    total_chunks = nframes // frames_per_chunk
    
    print(f"Tnę na {total_chunks} kawałków")
    
    for i in range(total_chunks):
        chunk_data = infile.readframes(frames_per_chunk)
        filename = os.path.join(OUTPUT_FOLDER, f"{base_name}_chunk_{i:04d}.wav")
        
        with wave.open(filename, 'wb') as outfile:
            outfile.setnchannels(nchannels)
            outfile.setsampwidth(sampwidth)
            outfile.setframerate(framerate)
            outfile.writeframes(chunk_data)

print("Gotowe!")