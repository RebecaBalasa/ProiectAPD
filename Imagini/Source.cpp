#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <vector>
#include <chrono>
#include "stb_image.h"
#include "stb_image_write.h"

using namespace std;
using namespace chrono;

// Functie pentru redimensionarea imaginii folosind interpolarea celui mai apropiat vecin
void resizeImage(const unsigned char* input, unsigned char* output, int oldWidth, int oldHeight, int newWidth, int newHeight, int channels) {
    double widthRatio = static_cast<double>(oldWidth) / newWidth;
    double heightRatio = static_cast<double>(oldHeight) / newHeight;

    for (int y = 0; y < newHeight; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int srcX = static_cast<int>(x * widthRatio);
            int srcY = static_cast<int>(y * heightRatio);

            for (int c = 0; c < channels; ++c) {
                output[(y * newWidth + x) * channels + c] = input[(srcY * oldWidth + srcX) * channels + c];
            }
        }
    }
}

// Functie pentru conversia imaginii in alb-negru
void convertToBlackAndWhite(const unsigned char* input, unsigned char* output, int width, int height, int channels) {
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * channels;
            int red = input[index];
            int green = input[index + 1];
            int blue = input[index + 2];

            unsigned char gray = static_cast<unsigned char>(0.299 * red + 0.587 * green + 0.114 * blue);

            output[y * width + x] = gray;
        }
    }
}

int main() {
    string inputPath = "poza7.jpg";
    string outputPath = "output.jpg";

    int width, height, channels;

    // Incarca imaginea
    unsigned char* img = stbi_load(inputPath.c_str(), &width, &height, &channels, 0);
    if (!img) {
        cout << "Eroare! Imaginea nu poate fi incarcata!" << endl;
        return -1;
    }

    int newWidth, newHeight;
    cout << "Introduceti latimea imaginii: ";
    cin >> newWidth;
    cout << "Introduceti inaltimea imaginii: ";
    cin >> newHeight;

    // Aloca memorie pentru imaginea redimensionata
    unsigned char* resizedImg = new unsigned char[newWidth * newHeight * channels];

    // Masoara timpul de redimensionare
    auto startResize = high_resolution_clock::now();
    resizeImage(img, resizedImg, width, height, newWidth, newHeight, channels);
    auto stopResize = high_resolution_clock::now();
    cout << "Timpul pentru redimensionare: " << duration_cast<milliseconds>(stopResize - startResize).count() << " ms" << endl;

    // Salveaza imaginea redimensionata
    string resizedFilename = outputPath + "output_poza7_redimensionare.jpg";
    stbi_write_png(resizedFilename.c_str(), newWidth, newHeight, channels, resizedImg, newWidth * channels);

    // Aloca memorie pentru imaginea alb-negru
    unsigned char* bwImg = new unsigned char[newWidth * newHeight];

    // Masoara timpul de conversie
    auto startBW = high_resolution_clock::now();
    convertToBlackAndWhite(resizedImg, bwImg, newWidth, newHeight, channels);
    auto stopBW = high_resolution_clock::now();
    cout << "Timpul pentru convertirea imaginii in alb-negru: " << duration_cast<milliseconds>(stopBW - startBW).count() << " ms" << endl;

    // Salveaza imaginea alb-negru
    string bwFilename = outputPath + "output_poza7_alb-negru.jpg";
    stbi_write_png(bwFilename.c_str(), newWidth, newHeight, 1, bwImg, newWidth);

    // Elibereaza memoria alocata
    stbi_image_free(img);
    delete[] resizedImg;
    delete[] bwImg;

    return 0;
}
