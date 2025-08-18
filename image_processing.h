#pragma once

#include <string>
#include <vector>
#include <cstdint>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Simple BMP file loader
struct BMPHeader {
    uint16_t type;
    uint32_t size;
    uint16_t reserved1, reserved2;
    uint32_t offset;
    uint32_t dib_size;
    int32_t width, height;
    uint16_t planes, bits;
    uint32_t compression, imagesize;
    int32_t xresolution, yresolution;
    uint32_t ncolors, importantcolors;
};

// RGB to CMYK conversion functions for color support
struct CMYKPixel {
    unsigned char c, m, y, k;
    CMYKPixel(unsigned char c = 0, unsigned char m = 0, unsigned char y = 0, unsigned char k = 255) 
        : c(c), m(m), y(y), k(k) {}
};

struct ImageData {
    int width, height;
    std::vector<unsigned char> data;  // Grayscale data (for backward compatibility)
    
    // Color channel data (only populated in color mode)
    std::vector<unsigned char> colorData;  // Raw RGB data: R,G,B,R,G,B,...
    std::vector<unsigned char> cyanData;   // Cyan channel
    std::vector<unsigned char> magentaData; // Magenta channel
    std::vector<unsigned char> yellowData;  // Yellow channel
    std::vector<unsigned char> blackData;   // Black (K) channel
    bool isColorMode;
    
    ImageData(int w = 0, int h = 0, bool colorMode = false);
    
    // Grayscale access (backward compatibility)
    unsigned char& at(int x, int y);
    const unsigned char& at(int x, int y) const;
    
    // Color channel access
    unsigned char& cyanAt(int x, int y);
    unsigned char& magentaAt(int x, int y);
    unsigned char& yellowAt(int x, int y);
    unsigned char& blackAt(int x, int y);
    
    // Perform CMYK color separation from RGB data
    void performColorSeparation();
    
    // Resize image to optimize processing - short side becomes 400px max
    void resizeForProcessing();
};

// Function declarations
bool loadBMP(const std::string& filename, std::vector<unsigned char>& imageData, int& width, int& height);
bool convertToBMP(const std::string& inputFile, const std::string& tempBMP);
CMYKPixel rgbToCmyk(unsigned char r, unsigned char g, unsigned char b);
bool loadBMPColor(const std::string& filename, ImageData& img);

// Native PNG loading functions
bool loadPNG(const std::string& filename, std::vector<unsigned char>& imageData, int& width, int& height);
bool loadPNGColor(const std::string& filename, ImageData& img);
bool isPNGFile(const std::string& filename);

// PNG helper functions
bool zlibDecompress(const std::vector<unsigned char>& compressed, std::vector<unsigned char>& decompressed);
void applyPNGFilter(unsigned char filter, unsigned char* row, unsigned char* prevRow, int rowBytes, int bytesPerPixel);
uint32_t readBigEndianUint32(std::ifstream& file);

// JPEG loading functions
bool loadJPEG(const std::string& filename, std::vector<unsigned char>& imageData, int& width, int& height);
bool loadJPEGColor(const std::string& filename, ImageData& img);
bool isJPEGFile(const std::string& filename);
bool parseJPEGHeader(const std::vector<unsigned char>& buffer, int& width, int& height);
bool decodeJPEG(const std::vector<unsigned char>& buffer, std::vector<unsigned char>& imageData, int width, int height);
