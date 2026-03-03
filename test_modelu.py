import numpy as np
import librosa
import tensorflow as tf

# Odwrotne słowniki (cyfra -> tekst)
KATEGORIA_ODWROTNA = {0: "Mowa", 1: "Muzyka"}
PODKATEGORIA_ODWROTNA = {
    0: "Audycja", 1: "Reklama", 2: "Wiadomości",
    3: "Lata 1980-2000", 4: "Pop", 5: "Pop-rock",
    6: "Reggae-pop", 7: "Rock-alternatywny"
}

print("Wczytywanie modelu...")
model = tf.keras.models.load_model("smart_tuner_model_v2.keras")

def sprawdz_dzwiek(sciezka_do_pliku):
    audio, sr = librosa.load(sciezka_do_pliku, sr=48000, duration=3.0)
    spektrogram = librosa.feature.melspectrogram(y=audio, sr=sr, n_mels=128)
    spektrogram_db = librosa.power_to_db(spektrogram, ref=np.max)
    
    if spektrogram_db.shape[1] < 128:
        pad_width = 128 - spektrogram_db.shape[1]
        spektrogram_db = np.pad(spektrogram_db, pad_width=((0, 0), (0, pad_width)))
    else:
        spektrogram_db = spektrogram_db[:, :128]
        
    X_test = np.array([spektrogram_db])[..., np.newaxis]
    
    wynik = model.predict(X_test, verbose=0)
    
    # wynik[0] to kategoria, wynik[1] to podkategoria
    szansa_kat = np.max(wynik[0][0]) * 100
    indeks_kat = np.argmax(wynik[0][0])
    
    szansa_podkat = np.max(wynik[1][0]) * 100
    indeks_podkat = np.argmax(wynik[1][0])
    
    print(f"Analiza pliku: {sciezka_do_pliku}")
    print(f"Kategoria główna: {KATEGORIA_ODWROTNA[indeks_kat]} (Pewność: {szansa_kat:.1f}%)")
    print(f"Szczegóły:        {PODKATEGORIA_ODWROTNA[indeks_podkat]} (Pewność: {szansa_podkat:.1f}%)\n")


sprawdz_dzwiek("jakis_nowy_plik.wav")