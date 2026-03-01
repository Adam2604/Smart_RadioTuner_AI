import tensorflow as tf
from tensorflow.keras.layers import Input, Conv2D, MaxPooling2D, Flatten, Dense
from tensorflow.keras.models import Model
import librosa
import os
import numpy as np
from sklearn.model_selection import train_test_split
import matplotlib.pyplot as plt

KATEGORIA_GLOWNA = {"mowa": 0, "muzyka": 1}
PODKATEGORIA = {
    "audycja": 0, "reklama": 1, "wiadomosci": 2,
    "lata1980-2000": 3, "pop": 4, "pop-rock": 5,
    "reggae-pop": 6, "rock-alternatywny": 7
}

def przygotuj_dane(sciezka="dataset"):
    X, y_kat, y_podkat = [],[],[]

    for folder in os.listdir(sciezka):
        sciezka_folderu = os.path.join(sciezka, folder)
        if not os.path.isdir(sciezka_folderu): continue

        #Rozdzielenie nazwy folderu
        czesci = folder.split("_")
        if len(czesci) != 2: continue
        kat_str, podkat_str = czesci[0], czesci[1]
        for plik in os.listdir(sciezka_folderu):
            if not plik.endswith(".wav"): continue
            sciezka_pliku = os.path.join(sciezka_folderu, plik)
            #Wczytanie audio
            audio, sr = librosa.load(sciezka_pliku, sr=48000, duration = 3.0)

            #Tworzenie spektogramu
            spektogram = librosa.feature.melspectrogram(y=audio, sr=sr, n_mels=128)
            spektrogram_db = librosa.power_to_db(spektogram, ref=np.max)

            #Dopełnienie lub przycięcie do wymiaru 128x128
            if spektrogram_db.shape[1] < 128:
                pad_width = 128 - spektrogram_db.shape[1]
                spektrogram_db = np.pad(spektrogram_db, pad_width = ((0,0), (0,pad_width)))
            else:
                spektrogram_db = spektrogram_db[:, :128]
            
            X.append(spektrogram_db)
            y_kat.append(KATEGORIA_GLOWNA[kat_str])
            y_podkat.append(PODKATEGORIA[podkat_str])
    X = np.array(X)[..., np.newaxis]
    return X, np.array(y_kat), np.array(y_podkat)

print("Przetwarzanie plików na spektogramy...")
X, y_kat, y_podkat = przygotuj_dane()
print("Zakończono przetwarzanie plików.")
print(f"Przygotowano {X.shape[0]} próbek gotowych do treningu")

wejscie = Input(shape=(128, 128, 1))

# wyciąganie cech z dźwięku
x = Conv2D(32, (3, 3), activation='relu')(wejscie)
x = MaxPooling2D((2, 2))(x)
x = Conv2D(64, (3, 3), activation='relu')(x)
x = MaxPooling2D((2, 2))(x)
x = Flatten()(x)
x = Dense(128, activation='relu')(x)

# Głowa 1: Mowa czy muzyka? (2 opcje)
wyjscie_glowne = Dense(len(KATEGORIA_GLOWNA), activation='softmax', name='kategoria')(x)

# Głowa 2: Konkretny gatunek (8 opcji)
wyjscie_podkategoria = Dense(len(PODKATEGORIA), activation='softmax', name='podkategoria')(x)

# Złożenie w jeden model
model = Model(inputs=wejscie, outputs=[wyjscie_glowne, wyjscie_podkategoria])

model.compile(
    optimizer='adam',
    loss={'kategoria': 'sparse_categorical_crossentropy', 'podkategoria': 'sparse_categorical_crossentropy'},
    metrics={'kategoria': 'accuracy', 'podkategoria': 'accuracy'}
)

model.summary()

# Podział danych: 80% do nauki, 20% do testowania
X_train, X_test, y_kat_train, y_kat_test, y_podkat_train, y_podkat_test = train_test_split(
    X, y_kat, y_podkat, test_size=0.2, random_state=42
)

print("Rozpoczynam trening modelu...")
historia = model.fit(
    X_train, 
    {'kategoria': y_kat_train, 'podkategoria': y_podkat_train},
    validation_data=(X_test, {'kategoria': y_kat_test, 'podkategoria': y_podkat_test}),
    epochs=15,        # Ile razy sieć przejrzy cały zbiór danych
    batch_size=32     # Po ile obrazków analizuje naraz przed aktualizacją wiedzy
)

model.save("smart_tuner_model_v1.keras")
print("Trening zakończony! Model zapisano jako 'smart_tuner_model.keras'.")

# Wykres skuteczności dla Kategorii (Mowa vs Muzyka)
plt.figure(figsize=(12, 5))
plt.subplot(1, 2, 1)
plt.plot(historia.history['kategoria_accuracy'], label='Trening')
plt.plot(historia.history['val_kategoria_accuracy'], label='Test (Egzamin)')
plt.title('Skuteczność: Mowa vs Muzyka')
plt.xlabel('Epoka')
plt.ylabel('Skuteczność')
plt.legend()

# Wykres skuteczności dla Podkategorii (Gatunki)
plt.subplot(1, 2, 2)
plt.plot(historia.history['podkategoria_accuracy'], label='Trening')
plt.plot(historia.history['val_podkategoria_accuracy'], label='Test (Egzamin)')
plt.title('Skuteczność: Gatunki (Pop, Rock itp.)')
plt.xlabel('Epoka')
plt.ylabel('Skuteczność')
plt.legend()

plt.tight_layout()
plt.savefig("wykres_nauki.png")
print("Wykres wygenerowany i zapisany jako 'wykres_nauki.png'")