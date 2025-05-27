#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include <iostream>
#include <vector>
#include <chrono>
#include <mpi.h>

using namespace std;
using namespace chrono;

void resizeImageSection(const unsigned char* input, unsigned char* output, int oldWidth, int oldHeight,
    int newWidth, int newHeight, int channels, int startY, int endY) {
    double widthRatio = static_cast<double>(oldWidth) / newWidth;
    double heightRatio = static_cast<double>(oldHeight) / newHeight;

    for (int y = startY; y < endY; ++y) {
        for (int x = 0; x < newWidth; ++x) {
            int srcX = static_cast<int>(x * widthRatio);
            int srcY = static_cast<int>(y * heightRatio);
            for (int c = 0; c < channels; ++c) {
                output[((y - startY) * newWidth + x) * channels + c] =
                    input[(srcY * oldWidth + srcX) * channels + c];
            }
        }
    }
}

void convertToBlackAndWhiteSection(const unsigned char* input, unsigned char* output,
    int width, int height, int channels) {
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

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int width = 0, height = 0, channels = 0;
    unsigned char* img = nullptr;
    int newWidth = 0, newHeight = 0;

    if (rank == 0) {
        string inputPath = "poza5.jpg";
        img = stbi_load(inputPath.c_str(), &width, &height, &channels, 0);
        if (!img) {
            cout << "Eroare! Imaginea nu poate fi incarcata!" << endl;
            MPI_Abort(MPI_COMM_WORLD, 1);
        }

        cout << "Introduceti latimea imaginii: ";
        cin >> newWidth;
        cout << "Introduceti inaltimea imaginii: ";
        cin >> newHeight;
    }

    // Distribuie dimensiunile
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&channels, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&newWidth, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&newHeight, 1, MPI_INT, 0, MPI_COMM_WORLD);

    // Trimite imaginea originala de la rank 0 la toate procesele
    int imgSize = width * height * channels;
    if (rank != 0) {
        img = new unsigned char[imgSize];
    }
    MPI_Bcast(img, imgSize, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // Impartirea liniilor
    int rowsPerProcess = newHeight / size;
    int startY = rank * rowsPerProcess;
    int endY = (rank == size - 1) ? newHeight : startY + rowsPerProcess;
    int localRows = endY - startY;

    // Alocari
    unsigned char* localResized = new unsigned char[localRows * newWidth * channels];
    unsigned char* localBW = new unsigned char[localRows * newWidth];

    // Procesare
    auto start = high_resolution_clock::now();
    resizeImageSection(img, localResized, width, height, newWidth, newHeight, channels, startY, endY);
    convertToBlackAndWhiteSection(localResized, localBW, newWidth, localRows, channels);
    auto stop = high_resolution_clock::now();

    cout << "Rank " << rank << " timp local: "
        << duration_cast<milliseconds>(stop - start).count() << " ms" << endl;

    // Colectare rezultate
    unsigned char* finalResized = nullptr;
    unsigned char* finalBW = nullptr;

    if (rank == 0) {
        finalResized = new unsigned char[newWidth * newHeight * channels];
        finalBW = new unsigned char[newWidth * newHeight];
    }

    MPI_Gather(localResized, localRows * newWidth * channels, MPI_UNSIGNED_CHAR,
        finalResized, localRows * newWidth * channels, MPI_UNSIGNED_CHAR,
        0, MPI_COMM_WORLD);

    MPI_Gather(localBW, localRows * newWidth, MPI_UNSIGNED_CHAR,
        finalBW, localRows * newWidth, MPI_UNSIGNED_CHAR,
        0, MPI_COMM_WORLD);

    if (rank == 0) {
        stbi_write_png("output_poza5_redimensionare.png", newWidth, newHeight, channels, finalResized, newWidth * channels);
        stbi_write_png("output_poza5_alb-negru.png", newWidth, newHeight, 1, finalBW, newWidth);
        stbi_image_free(img);
        delete[] finalResized;
        delete[] finalBW;
    }

    // Eliberare memorie
    delete[] localResized;
    delete[] localBW;
    if (rank != 0) {
        delete[] img;
    }

    MPI_Finalize();
    return 0;
}
