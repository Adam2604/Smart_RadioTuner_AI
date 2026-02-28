cmake_minimum_required(VERSION 3.20)
project(Smart_Tuner_AI)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_TOOLCHAIN_FILE "C:/dev/vcpkg/scripts/buildsystems/vcpkg.cmake")
find_path(RTLSDR_INCLUDE_DIR rtl-sdr.h)
find_library(RTLSDR_LIBRARY NAMES rtlsdr librtlsdr)
if (NOT RTLSDR_LIBRARY)
    message(FATAL_ERROR "Nie znaleziono RTL-SDR! Sprawdz vcpkg.")
else()
    message(STATUS "Znaleziono RTL-SDR: ${RTLSDR_LIBRARY}")
endif()

find_path(PORTAUDIO_INCLUDE_DIR portaudio.h)
find_library(PORTAUDIO_LIBRARY NAMES portaudio portaudio_x64)

if (NOT PORTAUDIO_LIBRARY)
    message(FATAL_ERROR "Nie znaleziono PortAudio! Sprawdz vcpkg.")
else()
    message(STATUS "Znaleziono PortAudio: ${PORTAUDIO_LIBRARY}")
endif()

find_package(FFTW3 CONFIG REQUIRED)
add_executable(Smart_Tuner_AI main.cpp
        WavWriter.cpp
        WavWriter.h)
target_include_directories(Smart_Tuner_AI PRIVATE
        ${RTLSDR_INCLUDE_DIR}
        ${PORTAUDIO_INCLUDE_DIR}
)
target_link_libraries(Smart_Tuner_AI PRIVATE
        ${RTLSDR_LIBRARY}
        ${PORTAUDIO_LIBRARY}
        FFTW3::fftw3
)
include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)
add_executable(RadioTests tests/WavWriterTest.cpp WavWriter.cpp
        tests/WavWriterTest.cpp)
target_link_libraries(RadioTests PRIVATE gtest_main)