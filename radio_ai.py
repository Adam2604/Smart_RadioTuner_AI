import socket
import numpy as np
import librosa
import tensorflow as tf
import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

KATEGORIA_ODWROTNA = {0: "Mowa", 1: "Muzyka"}
PODKATEGORIA_ODWROTNA = {
    0: "Audycja", 1: "Reklama", 2: "Wiadomości",
    3: "Lata 1980-2000", 4: "Pop", 5: "Pop-rock",
    6: "Reggae-pop", 7: "Rock-alternatywny"
}

print("Wczytywanie modelu...")
model = tf.keras.models.load_model("smart_tuner_model_v1.keras")

SR = 48000
POTRZEBNE_PROBKI = SR * 3  # 3 sekundy

def nasluch_udp():
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.bind(("127.0.0.1", 5005))
    feedback_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    feedback_addr = ("127.0.0.1", 5006)
    print("Nasłuchuję na porcie UDP 5005...")

    bufor_audio = []
    while True:
        dane, _ =sock.recvfrom(65536)
        probki = np.frombuffer(dane, dtype=np.float32)
        bufor_audio.extend(probki)

        #Gdy będą minimum 3 sekundy dźwięku
        if len(bufor_audio) >= POTRZEBNE_PROBKI:
            # Odcinamy równe 3 sekundy do analizy
            audio_do_analizy = np.array(bufor_audio[:POTRZEBNE_PROBKI])
            # Resztę zostawiamy w buforze na paczkę kolejnych 3 sekund
            bufor_audio = bufor_audio[POTRZEBNE_PROBKI:]

            spektrogram = librosa.feature.melspectrogram(y=audio_do_analizy, sr=SR, n_mels=128)
            spektrogram_db = librosa.power_to_db(spektrogram, ref=np.max)
            if spektrogram_db.shape[1] < 128:
                pad_width = 128 - spektrogram_db.shape[1]
                spektrogram_db = np.pad(spektrogram_db, pad_width=((0, 0), (0, pad_width)))
            else:
                spektrogram_db = spektrogram_db[:, :128]

            X_test = np.array([spektrogram_db])[..., np.newaxis]
            
            wynik = model.predict(X_test, verbose=0)
            indeks_kat = np.argmax(wynik[0][0])
            szansa_kat = np.max(wynik[0][0]) * 100
            indeks_podkat = np.argmax(wynik[1][0])
            szansa_podkat = np.max(wynik[1][0]) * 100

            feedback_msg = f"{indeks_kat},{indeks_podkat}"
            feedback_socket.sendto(feedback_msg.encode(), feedback_addr)

            print(f"[{szansa_kat:5.1f}%] {KATEGORIA_ODWROTNA[indeks_kat]:<10} | "
                  f"[{szansa_podkat:5.1f}%] {PODKATEGORIA_ODWROTNA[indeks_podkat]:<15}")

if __name__ == "__main__":
    nasluch_udp()