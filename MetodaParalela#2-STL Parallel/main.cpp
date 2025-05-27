#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <execution>  // Pentru std::execution::par

using namespace std;
using namespace chrono;

void resizeImage(const unsigned char* input, unsigned char* output, int oldWidth, int oldHeight,
    int newWidth, int newHeight, int channels) {
    double widthRatio = static_cast<double>(oldWidth) / newWidth;
    double heightRatio = static_cast<double>(oldHeight) / newHeight;

    vector<int> indices(newHeight);
    iota(indices.begin(), indices.end(), 0); // Creează vector [0, 1, ..., newHeight-1]

    for_each(execution::par, indices.begin(), indices.end(), [&](int y) {
        for (int x = 0; x < newWidth; ++x) {
            int srcX = static_cast<int>(x * widthRatio);
            int srcY = static_cast<int>(y * heightRatio);
            for (int c = 0; c < channels; ++c) {
                output[(y * newWidth + x) * channels + c] =
                    input[(srcY * oldWidth + srcX) * channels + c];
            }
        }
        });
}

void convertToBlackAndWhite(const unsigned char* input, unsigned char* output,
    int width, int height, int channels) {
    vector<int> indices(height);
    iota(indices.begin(), indices.end(), 0);

    for_each(execution::par, indices.begin(), indices.end(), [&](int y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * channels;
            int red = input[index];
            int green = input[index + 1];
            int blue = input[index + 2];
            unsigned char gray = static_cast<unsigned char>(0.299 * red + 0.587 * green + 0.114 * blue);
            output[y * width + x] = gray;
        }
        });
}

int main() {
    int width, height, channels;
    string inputPath = "poza7.jpg";

    unsigned char* img = stbi_load(inputPath.c_str(), &width, &height, &channels, 0);
    if (!img) {
        cout << "Eroare! Imaginea nu poate fi incarcata!" << endl;
        return 1;
    }

    int newWidth, newHeight;
    cout << "Introduceti latimea imaginii: ";
    cin >> newWidth;
    cout << "Introduceti inaltimea imaginii: ";
    cin >> newHeight;

    auto start = high_resolution_clock::now();

    unsigned char* resized = new unsigned char[newWidth * newHeight * channels];
    resizeImage(img, resized, width, height, newWidth, newHeight, channels);

    auto stop = high_resolution_clock::now();

    unsigned char* blackWhite = new unsigned char[newWidth * newHeight];
    convertToBlackAndWhite(resized, blackWhite, newWidth, newHeight, channels);


    cout << "Timp total procesare redimensionare (cu STL parallel): "
        << duration_cast<milliseconds>(stop - start).count() << " ms" << endl;

    stbi_write_png("output_poza7_redimensionare.png", newWidth, newHeight, channels, resized, newWidth * channels);
    stbi_write_png("output_poza7_alb-negru.png", newWidth, newHeight, 1, blackWhite, newWidth);

    stbi_image_free(img);
    delete[] resized;
    delete[] blackWhite;

    return 0;
}
