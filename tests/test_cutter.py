"""Testy pytest dla modułu cutter.py — cięcie plików WAV na chunki."""

import wave
import struct
import os
import sys
import pytest

# Dodanie katalogu projektu do ścieżki importów
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))
from cutter import tnij_plik


# Pomocnicza funkcja do tworzenia testowych plików WAV
def stworz_wav(sciezka, dlugosc_s, framerate=48000, nchannels=1, sampwidth=2):
    """Tworzy syntetyczny plik WAV wypełniony zerami o zadanej długości."""
    nframes = int(framerate * dlugosc_s)
    with wave.open(sciezka, "wb") as f:
        f.setnchannels(nchannels)
        f.setsampwidth(sampwidth)
        f.setframerate(framerate)
        # Zapisujemy cichy sygnał (zera)
        dane = struct.pack(f"<{nframes * nchannels}h", *([0] * nframes * nchannels))
        f.writeframes(dane)


# Fixture: tymczasowy katalog roboczy
@pytest.fixture
def katalog_roboczy(tmp_path):
    """Przygotowuje katalog z plikiem wejściowym i folderem wyjściowym."""
    return {
        "input_dir": tmp_path,
        "output_dir": str(tmp_path / "output"),
    }

# TESTY

class TestLiczbaChunkow:
    """Testy sprawdzające poprawną liczbę wygenerowanych chunków."""

    def test_dokladnie_3_chunki_z_9_sekund(self, katalog_roboczy):
        """9 sekund audio ÷ 3-sekundowe chunki = dokładnie 3 pliki."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=9)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        assert len(wynik) == 3

    def test_reszta_jest_pomijana(self, katalog_roboczy):
        """10 sekund ÷ 3 = 3 chunki (1 sekunda reszty jest pomijana)."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=10)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        assert len(wynik) == 3

    def test_dokladnie_1_chunk(self, katalog_roboczy):
        """Dokładnie 3 sekundy = 1 chunk."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=3)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        assert len(wynik) == 1

    def test_wiecej_chunkow_5_sekundowych(self, katalog_roboczy):
        """30 sekund ÷ 5-sekundowe chunki = 6 plików."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=30)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=5)

        assert len(wynik) == 6


class TestKrotkiePlikI:
    """Testy dla plików krótszych niż chunk_length_s."""

    def test_za_krotki_plik_brak_chunkow(self, katalog_roboczy):
        """Plik krótszy niż 3 sekundy → 0 chunków."""
        wav_path = str(katalog_roboczy["input_dir"] / "krotki.wav")
        stworz_wav(wav_path, dlugosc_s=2)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        assert len(wynik) == 0

    def test_pusty_plik_brak_chunkow(self, katalog_roboczy):
        """Plik WAV z 0 ramek → 0 chunków, brak błędu."""
        wav_path = str(katalog_roboczy["input_dir"] / "pusty.wav")
        stworz_wav(wav_path, dlugosc_s=0)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        assert len(wynik) == 0


class TestParametryAudio:
    """Testy sprawdzające, czy chunki zachowują parametry oryginału."""

    def test_zachowanie_sample_rate(self, katalog_roboczy):
        """Chunki muszą mieć ten sam sample rate co plik wejściowy."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        framerate = 44100
        stworz_wav(wav_path, dlugosc_s=6, framerate=framerate)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        for chunk_path in wynik:
            with wave.open(chunk_path, "rb") as f:
                assert f.getframerate() == framerate

    def test_zachowanie_liczby_kanalow(self, katalog_roboczy):
        """Chunki stereo muszą pozostać stereo."""
        wav_path = str(katalog_roboczy["input_dir"] / "stereo.wav")
        stworz_wav(wav_path, dlugosc_s=6, nchannels=2)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        for chunk_path in wynik:
            with wave.open(chunk_path, "rb") as f:
                assert f.getnchannels() == 2

    def test_zachowanie_sample_width(self, katalog_roboczy):
        """Chunki muszą mieć tę samą szerokość próbki."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=6, sampwidth=2)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        for chunk_path in wynik:
            with wave.open(chunk_path, "rb") as f:
                assert f.getsampwidth() == 2

    def test_poprawna_dlugosc_chunkow(self, katalog_roboczy):
        """Każdy chunk powinien mieć dokładnie framerate × chunk_length_s ramek."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        framerate = 48000
        chunk_s = 3
        stworz_wav(wav_path, dlugosc_s=9, framerate=framerate)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=chunk_s)

        for chunk_path in wynik:
            with wave.open(chunk_path, "rb") as f:
                assert f.getnframes() == framerate * chunk_s


class TestNazwyPlikow:
    """Testy nazewnictwa i struktury plików wyjściowych."""

    def test_nazwy_chunkow_zawieraja_nazwe_bazowa(self, katalog_roboczy):
        """Chunki powinny zawierać oryginalną nazwę pliku w nazwie."""
        wav_path = str(katalog_roboczy["input_dir"] / "moja_piosenka.wav")
        stworz_wav(wav_path, dlugosc_s=6)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        for chunk_path in wynik:
            nazwa = os.path.basename(chunk_path)
            assert nazwa.startswith("moja_piosenka_chunk_")

    def test_numeracja_od_zera(self, katalog_roboczy):
        """Chunki powinny być numerowane od 0000."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=9)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        nazwy = [os.path.basename(p) for p in wynik]
        assert nazwy[0] == "test_chunk_0000.wav"
        assert nazwy[1] == "test_chunk_0001.wav"
        assert nazwy[2] == "test_chunk_0002.wav"

    def test_tworzy_folder_wyjsciowy(self, katalog_roboczy):
        """Folder wyjściowy powinien zostać utworzony automatycznie."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=3)
        nowy_folder = str(katalog_roboczy["input_dir"] / "nowy" / "podkatalog")

        tnij_plik(wav_path, nowy_folder, chunk_length_s=3)

        assert os.path.isdir(nowy_folder)

    def test_chunki_istnieja_na_dysku(self, katalog_roboczy):
        """Zwrócone ścieżki powinny wskazywać na istniejące pliki."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        stworz_wav(wav_path, dlugosc_s=6)

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=3)

        for chunk_path in wynik:
            assert os.path.isfile(chunk_path)


class TestIntegralnosc:
    """Testy integralności danych — czy dane nie są tracone."""

    def test_dane_chunkow_skladaja_sie_na_oryginal(self, katalog_roboczy):
        """Połączenie wszystkich chunków powinno dać te same dane co oryginał 
        (do granicy pełnych chunków)."""
        wav_path = str(katalog_roboczy["input_dir"] / "test.wav")
        framerate = 48000
        chunk_s = 3
        stworz_wav(wav_path, dlugosc_s=9, framerate=framerate)

        # Odczytaj oryginalne dane
        with wave.open(wav_path, "rb") as f:
            oryginalne_dane = f.readframes(f.getnframes())

        wynik = tnij_plik(wav_path, katalog_roboczy["output_dir"], chunk_length_s=chunk_s)

        # Złącz dane chunków
        zlaczone = b""
        for chunk_path in wynik:
            with wave.open(chunk_path, "rb") as f:
                zlaczone += f.readframes(f.getnframes())

        assert zlaczone == oryginalne_dane
