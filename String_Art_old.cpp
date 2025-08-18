#include "image_processing.h"
#include "string_art_generator.h"
#include "svg_generator.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <algorithm>

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

// Updated BMP loader - supports both grayscale and color modes
bool loadBMP(const std::string& filename, std::vector<unsigned char>& imageData, int& width, int& height) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    BMPHeader header;
    file.read(reinterpret_cast<char*>(&header.type), 2);
    file.read(reinterpret_cast<char*>(&header.size), 4);
    file.read(reinterpret_cast<char*>(&header.reserved1), 2);
    file.read(reinterpret_cast<char*>(&header.reserved2), 2);
    file.read(reinterpret_cast<char*>(&header.offset), 4);
    file.read(reinterpret_cast<char*>(&header.dib_size), 4);
    file.read(reinterpret_cast<char*>(&header.width), 4);
    file.read(reinterpret_cast<char*>(&header.height), 4);
    file.read(reinterpret_cast<char*>(&header.planes), 2);
    file.read(reinterpret_cast<char*>(&header.bits), 2);
    file.read(reinterpret_cast<char*>(&header.compression), 4);
    file.read(reinterpret_cast<char*>(&header.imagesize), 4);
    file.read(reinterpret_cast<char*>(&header.xresolution), 4);
    file.read(reinterpret_cast<char*>(&header.yresolution), 4);
    file.read(reinterpret_cast<char*>(&header.ncolors), 4);
    file.read(reinterpret_cast<char*>(&header.importantcolors), 4);
    
    if (header.type != 0x4D42 || header.bits != 24) return false;
    
    width = header.width;
    height = abs(header.height);
    
    file.seekg(header.offset);
    
    int rowSize = ((width * 3 + 3) / 4) * 4;
    std::vector<unsigned char> row(rowSize);
    imageData.resize(width * height);
    
    for (int y = 0; y < height; y++) {
        file.read(reinterpret_cast<char*>(row.data()), rowSize);
        for (int x = 0; x < width; x++) {
            unsigned char b = row[x * 3];
            unsigned char g = row[x * 3 + 1];
            unsigned char r = row[x * 3 + 2];
            unsigned char gray = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
            
            int flipped_y = height - 1 - y;
            imageData[flipped_y * width + x] = gray;
        }
    }
    
    return true;
}


// PNG/JPEG loader using Python PIL
bool convertToBMP(const std::string& inputFile, const std::string& tempBMP) {
    std::string cmd = "python -c \"";
    cmd += "from PIL import Image; ";
    cmd += "img = Image.open('" + inputFile + "'); ";
    cmd += "img = img.convert('RGB'); ";
    cmd += "img.save('" + tempBMP + "')\"";
    
    int result = system(cmd.c_str());
    if (result == 0) {
        return std::filesystem::exists(tempBMP);
    }
    
    // Fallback: try ImageMagick convert
    cmd = "magick \"" + inputFile + "\" \"" + tempBMP + "\"";
    result = system(cmd.c_str());
    if (result == 0) {
        return std::filesystem::exists(tempBMP);
    }
    
    return false;
}

// RGB to CMYK conversion functions for color support
struct CMYKPixel {
    unsigned char c, m, y, k;
    CMYKPixel(unsigned char c = 0, unsigned char m = 0, unsigned char y = 0, unsigned char k = 255) 
        : c(c), m(m), y(y), k(k) {}
};

CMYKPixel rgbToCmyk(unsigned char r, unsigned char g, unsigned char b) {
    // Convert RGB to CMYK
    double rf = r / 255.0;
    double gf = g / 255.0;
    double bf = b / 255.0;
    
    double k = 1.0 - std::max({rf, gf, bf});
    
    if (k >= 1.0) {
        return CMYKPixel(0, 0, 0, 255);
    }
    
    double c = (1.0 - rf - k) / (1.0 - k);
    double m = (1.0 - gf - k) / (1.0 - k);
    double y = (1.0 - bf - k) / (1.0 - k);
    
    return CMYKPixel(
        (unsigned char)(c * 255),
        (unsigned char)(m * 255),
        (unsigned char)(y * 255),
        (unsigned char)(k * 255)
    );
}

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
    
    ImageData(int w = 0, int h = 0, bool colorMode = false) 
        : width(w), height(h), data(w * h, 0), isColorMode(colorMode) {
        if (colorMode) {
            colorData.resize(w * h * 3, 0);  // RGB triplets
            cyanData.resize(w * h, 0);
            magentaData.resize(w * h, 0);
            yellowData.resize(w * h, 0);
            blackData.resize(w * h, 0);
        }
    }
    
    // Grayscale access (backward compatibility)
    unsigned char& at(int x, int y) {
        return data[y * width + x];
    }
    
    const unsigned char& at(int x, int y) const {
        return data[y * width + x];
    }
    
    // Color channel access
    unsigned char& cyanAt(int x, int y) {
        return cyanData[y * width + x];
    }
    
    unsigned char& magentaAt(int x, int y) {
        return magentaData[y * width + x];
    }
    
    unsigned char& yellowAt(int x, int y) {
        return yellowData[y * width + x];
    }
    
    unsigned char& blackAt(int x, int y) {
        return blackData[y * width + x];
    }
    
    // Perform CMYK color separation from RGB data
    void performColorSeparation() {
        if (!isColorMode || colorData.empty()) return;
        
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < width; x++) {
                int pixelIdx = y * width + x;
                int colorIdx = pixelIdx * 3;
                
                unsigned char r = colorData[colorIdx];
                unsigned char g = colorData[colorIdx + 1];
                unsigned char b = colorData[colorIdx + 2];
                
                // Convert to grayscale for backward compatibility
                unsigned char gray = (unsigned char)(0.299 * r + 0.587 * g + 0.114 * b);
                data[pixelIdx] = gray;
                
                // Convert to CMYK
                CMYKPixel cmyk = rgbToCmyk(r, g, b);
                cyanData[pixelIdx] = 255 - cmyk.c;    // Invert for darkness values
                magentaData[pixelIdx] = 255 - cmyk.m;
                yellowData[pixelIdx] = 255 - cmyk.y;
                blackData[pixelIdx] = 255 - cmyk.k;
            }
        }
    }
    
    // Resize image to optimize processing - short side becomes 400px max
    void resizeForProcessing() {
        const int targetShortSide = 400;
        
        // Check if resizing is needed
        int shortSide = std::min(width, height);
        if (shortSide <= targetShortSide) {
            return; // Image is already small enough
        }
        
        // Calculate new dimensions
        double scaleFactor = (double)targetShortSide / shortSide;
        int newWidth = (int)(width * scaleFactor);
        int newHeight = (int)(height * scaleFactor);
        
        std::cout << "Resizing image from " << width << "x" << height 
                  << " to " << newWidth << "x" << newHeight 
                  << " (scale factor: " << std::fixed << std::setprecision(3) << scaleFactor << ")" << std::endl;
        
        if (isColorMode) {
            // Resize color data using bilinear interpolation
            std::vector<unsigned char> newColorData(newWidth * newHeight * 3);
            
            for (int y = 0; y < newHeight; y++) {
                for (int x = 0; x < newWidth; x++) {
                    // Map new coordinates to original image
                    double srcX = x / scaleFactor;
                    double srcY = y / scaleFactor;
                    
                    // Bilinear interpolation
                    int x1 = (int)srcX;
                    int y1 = (int)srcY;
                    int x2 = std::min(x1 + 1, width - 1);
                    int y2 = std::min(y1 + 1, height - 1);
                    
                    double fx = srcX - x1;
                    double fy = srcY - y1;
                    
                    for (int c = 0; c < 3; c++) { // RGB channels
                        double p1 = colorData[(y1 * width + x1) * 3 + c] * (1 - fx) + colorData[(y1 * width + x2) * 3 + c] * fx;
                        double p2 = colorData[(y2 * width + x1) * 3 + c] * (1 - fx) + colorData[(y2 * width + x2) * 3 + c] * fx;
                        double result = p1 * (1 - fy) + p2 * fy;
                        
                        newColorData[(y * newWidth + x) * 3 + c] = (unsigned char)std::clamp(result, 0.0, 255.0);
                    }
                }
            }
            
            // Update dimensions and data
            width = newWidth;
            height = newHeight;
            colorData = newColorData;
            
            // Resize other data vectors
            data.resize(width * height);
            cyanData.resize(width * height);
            magentaData.resize(width * height);
            yellowData.resize(width * height);
            blackData.resize(width * height);
            
            // Recompute color separation with new size
            performColorSeparation();
        } else {
            // Resize grayscale data using bilinear interpolation
            std::vector<unsigned char> newData(newWidth * newHeight);
            
            for (int y = 0; y < newHeight; y++) {
                for (int x = 0; x < newWidth; x++) {
                    // Map new coordinates to original image
                    double srcX = x / scaleFactor;
                    double srcY = y / scaleFactor;
                    
                    // Bilinear interpolation
                    int x1 = (int)srcX;
                    int y1 = (int)srcY;
                    int x2 = std::min(x1 + 1, width - 1);
                    int y2 = std::min(y1 + 1, height - 1);
                    
                    double fx = srcX - x1;
                    double fy = srcY - y1;
                    
                    double p1 = data[y1 * width + x1] * (1 - fx) + data[y1 * width + x2] * fx;
                    double p2 = data[y2 * width + x1] * (1 - fx) + data[y2 * width + x2] * fx;
                    double result = p1 * (1 - fy) + p2 * fy;
                    
                    newData[y * newWidth + x] = (unsigned char)std::clamp(result, 0.0, 255.0);
                }
            }
            
            // Update dimensions and data
            width = newWidth;
            height = newHeight;
            data = newData;
        }
    }
};

// Color-aware BMP loader that preserves RGB information
bool loadBMPColor(const std::string& filename, ImageData& img) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;
    
    BMPHeader header;
    file.read(reinterpret_cast<char*>(&header.type), 2);
    file.read(reinterpret_cast<char*>(&header.size), 4);
    file.read(reinterpret_cast<char*>(&header.reserved1), 2);
    file.read(reinterpret_cast<char*>(&header.reserved2), 2);
    file.read(reinterpret_cast<char*>(&header.offset), 4);
    file.read(reinterpret_cast<char*>(&header.dib_size), 4);
    file.read(reinterpret_cast<char*>(&header.width), 4);
    file.read(reinterpret_cast<char*>(&header.height), 4);
    file.read(reinterpret_cast<char*>(&header.planes), 2);
    file.read(reinterpret_cast<char*>(&header.bits), 2);
    file.read(reinterpret_cast<char*>(&header.compression), 4);
    file.read(reinterpret_cast<char*>(&header.imagesize), 4);
    file.read(reinterpret_cast<char*>(&header.xresolution), 4);
    file.read(reinterpret_cast<char*>(&header.yresolution), 4);
    file.read(reinterpret_cast<char*>(&header.ncolors), 4);
    file.read(reinterpret_cast<char*>(&header.importantcolors), 4);
    
    if (header.type != 0x4D42 || header.bits != 24) return false;
    
    int width = header.width;
    int height = abs(header.height);
    
    img = ImageData(width, height, true);  // Color mode enabled
    
    file.seekg(header.offset);
    
    int rowSize = ((width * 3 + 3) / 4) * 4;
    std::vector<unsigned char> row(rowSize);
    
    for (int y = 0; y < height; y++) {
        file.read(reinterpret_cast<char*>(row.data()), rowSize);
        for (int x = 0; x < width; x++) {
            unsigned char b = row[x * 3];
            unsigned char g = row[x * 3 + 1];
            unsigned char r = row[x * 3 + 2];
            
            int flipped_y = height - 1 - y;
            int colorIdx = (flipped_y * width + x) * 3;
            
            // Store RGB data
            img.colorData[colorIdx] = r;
            img.colorData[colorIdx + 1] = g;
            img.colorData[colorIdx + 2] = b;
        }
    }
    
    // Perform color separation
    img.performColorSeparation();
    
    return true;
}

// Color order helper functions - defined before StringArtGenerator class
struct ColorOrderInfo {
    char letter;
    std::string name;
    std::string displayName;
    std::string svgColor;
    
    ColorOrderInfo(char l, const std::string& n, const std::string& d, const std::string& c)
        : letter(l), name(n), displayName(d), svgColor(c) {}
};

std::vector<ColorOrderInfo> getColorOrderSequence(const std::string& colorOrder) {
    std::vector<ColorOrderInfo> sequence;
    
    for (char c : colorOrder) {
        switch(c) {
            case 'C':
                sequence.emplace_back('C', "cyan", "CYAN", "cyan");
                break;
            case 'M':
                sequence.emplace_back('M', "magenta", "MAGENTA", "magenta");
                break;
            case 'Y':
                sequence.emplace_back('Y', "yellow", "YELLOW", "gold");
                break;
            case 'K':
                sequence.emplace_back('K', "black", "BLACK", "black");
                break;
        }
    }
    
    return sequence;
}

class StringArtGenerator {
private:
    double m_contrastFactor;
    
public:
    StringArtGenerator(double contrastFactor = 0.5) : m_contrastFactor(contrastFactor) {}
    
    // Grayscale image loading (backward compatibility)
    bool loadImage(const std::string& filename, ImageData& img) {
        return loadImage(filename, img, false);
    }
    
    // Enhanced image loading with color mode support
    bool loadImage(const std::string& filename, ImageData& img, bool colorMode) {
        std::string ext = filename.substr(filename.find_last_of(".") + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        
        bool loadSuccess = false;
        
        if (colorMode) {
            // Color mode: preserve RGB information
            if (ext == "bmp") {
                loadSuccess = loadBMPColor(filename, img);
            } else if (ext == "png" || ext == "jpg" || ext == "jpeg") {
                std::string tempBMP = "temp_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()) + ".bmp";
                
                if (convertToBMP(filename, tempBMP)) {
                    loadSuccess = loadBMPColor(tempBMP, img);
                    std::filesystem::remove(tempBMP);
                }
            }
        } else {
            // Grayscale mode (existing functionality)
            if (ext == "bmp") {
                std::vector<unsigned char> imageData;
                int width, height;
                
                if (loadBMP(filename, imageData, width, height)) {
                    img = ImageData(width, height);
                    img.data = imageData;
                    loadSuccess = true;
                }
            } else if (ext == "png" || ext == "jpg" || ext == "jpeg") {
                std::string tempBMP = "temp_" + std::to_string(std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count()) + ".bmp";
                
                if (convertToBMP(filename, tempBMP)) {
                    std::vector<unsigned char> imageData;
                    int width, height;
                    
                    if (loadBMP(tempBMP, imageData, width, height)) {
                        img = ImageData(width, height);
                        img.data = imageData;
                        loadSuccess = true;
                    }
                    std::filesystem::remove(tempBMP);
                }
            }
        }
        
        // Resize image for optimal processing if loading was successful
        if (loadSuccess) {
            img.resizeForProcessing();
        }
        
        return loadSuccess;
    }
    
    std::vector<int> generateStringArt(const ImageData& img, int numNails, bool isCircular, int maxStrings = 0) {
        std::cout << "Analyzing image (" << img.width << "x" << img.height << ") with contrast factor " << m_contrastFactor << std::endl;
        
        if (!isCircular) {
            return generateRectangularStringArt(img, numNails, maxStrings);
        }
        
        // Generate nail positions around circle
        std::vector<std::pair<double, double>> nails;
        int centerX = img.width / 2;
        int centerY = img.height / 2;
        double radius = std::min(img.width, img.height) / 2.0 - 10;
        
        for (int i = 0; i < numNails; i++) {
            double angle = 2.0 * M_PI * i / numNails;
            double x = centerX + radius * cos(angle);
            double y = centerY + radius * sin(angle);
            nails.push_back({x, y});
        }
        
        std::cout << "Placed " << numNails << " nails around circle (radius: " << radius << ")" << std::endl;
        
        // Greedy algorithm
        std::vector<int> sequence;
        std::vector<std::vector<double>> coverage(img.height, std::vector<double>(img.width, 0.0));
        
        int currentNail = 0;
        sequence.push_back(currentNail);
        
        int targetStrings = maxStrings;
        if (targetStrings > 0) {
            std::cout << "Target strings: " << targetStrings << std::endl;
        } else {
            std::cout << "Target strings: unlimited (will stop when no improvement)" << std::endl;
        }
        
        // Internal safety limit to prevent infinite loops
        int internalLimit = (targetStrings > 0) ? targetStrings : 10000;
        
        double lastBestScore = 1.0;
        int stagnantCount = 0;
        double lastScore = -1.0;
        double secondLastScore = -1.0;
        int alternatingCount = 0;
        
        for (int stringIdx = 0; stringIdx < internalLimit - 1; stringIdx++) {
            int bestNextNail = -1;
            double bestScore = -1.0;
            
            // Try all other nails
            for (int nextNail = 0; nextNail < numNails; nextNail++) {
                if (nextNail == currentNail) continue;
                
                // Avoid recent nails
                bool skipRecent = false;
                int lookbackLimit = std::min(7, (int)sequence.size());
                for (int lookback = 1; lookback <= lookbackLimit; lookback++) {
                    if (sequence.size() >= lookback && sequence[sequence.size() - lookback] == nextNail) {
                        skipRecent = true;
                        break;
                    }
                }
                if (skipRecent) continue;
                
                double score = calculateLineScore(img, coverage, nails[currentNail], nails[nextNail]);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestNextNail = nextNail;
                }
            }
            
            // Only break if no valid nail found OR score becomes negligible
            if (bestNextNail == -1 || bestScore < 0.01) {
                if (bestScore < 0.01) {
                    std::cout << "Stopping: Score too low (" << bestScore << "), no more meaningful connections" << std::endl;
                }
                break;
            }
            
            // Detect alternating pattern (oscillation between two scores)
            if (stringIdx > 100) {
                // Check if current score equals score from 2 iterations ago AND differs from last score
                if (abs(bestScore - secondLastScore) < 0.000001 && abs(bestScore - lastScore) > 0.000001) {
                    alternatingCount++;
                    if (alternatingCount >= 20) {
                        std::cout << "Stopping: Detected alternating pattern between scores " << bestScore << " and " << lastScore << std::endl;
                        break;
                    }
                } else {
                    alternatingCount = 0;
                }
            }
            
            // Check for stagnation
            if (bestScore >= lastBestScore - 0.0005) {
                stagnantCount++;
            } else {
                stagnantCount = 0;
            }
            
            
            // Force exploration if stagnant
            if (stagnantCount > 30) {
                int offset = 1 + (stringIdx % 11) + (stringIdx / 100);
                bestNextNail = (currentNail + offset) % numNails;
                
                while (bestNextNail == currentNail) {
                    bestNextNail = (bestNextNail + 1) % numNails;
                }
                
                stagnantCount = 0;
            }
            
            // Mark coverage
            double coverageStrength = 1.0;
            if (targetStrings > 0) {
                coverageStrength = 1.0 - (double)stringIdx / (targetStrings * 2.0);
            } else {
                // For unlimited strings, use constant moderate coverage to avoid artificial limits
                coverageStrength = 0.6;
            }
            markLineCoverage(coverage, nails[currentNail], nails[bestNextNail], coverageStrength);
            
            sequence.push_back(bestNextNail);
            currentNail = bestNextNail;
            secondLastScore = lastScore;
            lastScore = bestScore;
            lastBestScore = bestScore;
            
            if ((stringIdx + 1) % 100 == 0) {
                std::cout << "Generated " << (stringIdx + 1) << " strings, last score: " << bestScore << std::endl;
            }
        }
        
        std::cout << "Generated " << sequence.size() << " total strings" << std::endl;
        return sequence;
    }
    
    // EXPERIMENTAL coverage strategies - DO NOT modify original generateStringArt
    std::vector<int> generateStringArtExperimental(const ImageData& img, int numNails, bool isCircular, int maxStrings = 0, int coverageStrategy = 1) {
        std::cout << "Analyzing image (" << img.width << "x" << img.height << ") with contrast factor " << m_contrastFactor << std::endl;
        
        if (!isCircular) {
            return generateRectangularStringArt(img, numNails, maxStrings);
        }
        
        // Generate nail positions around circle
        std::vector<std::pair<double, double>> nails;
        int centerX = img.width / 2;
        int centerY = img.height / 2;
        double radius = std::min(img.width, img.height) / 2.0 - 10;
        
        for (int i = 0; i < numNails; i++) {
            double angle = 2.0 * M_PI * i / numNails;
            double x = centerX + radius * cos(angle);
            double y = centerY + radius * sin(angle);
            nails.push_back({x, y});
        }
        
        std::cout << "Placed " << numNails << " nails around circle (radius: " << radius << ")" << std::endl;
        
        // Greedy algorithm
        std::vector<int> sequence;
        std::vector<std::vector<double>> coverage(img.height, std::vector<double>(img.width, 0.0));
        
        int currentNail = 0;
        sequence.push_back(currentNail);
        
        int targetStrings = maxStrings;
        if (targetStrings > 0) {
            std::cout << "Target strings: " << targetStrings << std::endl;
        } else {
            std::cout << "Target strings: unlimited (will stop when no improvement)" << std::endl;
        }
        
        // Internal safety limit to prevent infinite loops
        int internalLimit = (targetStrings > 0) ? targetStrings : 10000;
        
        // Display coverage strategy info
        std::string strategyName[] = {"Default", "Adaptive Coverage", "Dynamic Threshold", "Exploration Boost"};
        std::cout << "Coverage strategy: " << strategyName[coverageStrategy] << " (" << coverageStrategy << ")" << std::endl;
        
        double lastBestScore = 1.0;
        int stagnantCount = 0;
        double lastScore = -1.0;
        double secondLastScore = -1.0;
        int alternatingCount = 0;
        
        for (int stringIdx = 0; stringIdx < internalLimit - 1; stringIdx++) {
            int bestNextNail = -1;
            double bestScore = -1.0;
            
            // Try all other nails
            for (int nextNail = 0; nextNail < numNails; nextNail++) {
                if (nextNail == currentNail) continue;
                
                // Avoid recent nails
                bool skipRecent = false;
                int lookbackLimit = std::min(7, (int)sequence.size());
                for (int lookback = 1; lookback <= lookbackLimit; lookback++) {
                    if (sequence.size() >= lookback && sequence[sequence.size() - lookback] == nextNail) {
                        skipRecent = true;
                        break;
                    }
                }
                if (skipRecent) continue;
                
                double score = calculateLineScore(img, coverage, nails[currentNail], nails[nextNail]);
                
                // Strategy 3: Exploration boost - bonus for longer distances
                if (coverageStrategy == 3) {
                    double dx = nails[nextNail].first - nails[currentNail].first;
                    double dy = nails[nextNail].second - nails[currentNail].second;
                    double distance = sqrt(dx*dx + dy*dy);
                    double maxDistance = 2.0 * radius; // Approximate max distance
                    double distanceBonus = 0.1 * (distance / maxDistance); // Small bonus for distance
                    score += distanceBonus;
                }
                
                if (score > bestScore) {
                    bestScore = score;
                    bestNextNail = nextNail;
                }
            }
            
            // Strategy 2: Dynamic threshold adjustment
            double scoreThreshold = 0.01;
            if (coverageStrategy == 2 && targetStrings > 0) {
                double progress = (double)stringIdx / targetStrings;
                scoreThreshold = 0.01 + 0.02 * progress; // Increase threshold as we progress
            }
            
            // Only break if no valid nail found OR score becomes negligible
            if (bestNextNail == -1 || bestScore < scoreThreshold) {
                if (bestScore < scoreThreshold) {
                    std::cout << "Stopping: Score too low (" << bestScore << "), no more meaningful connections" << std::endl;
                }
                break;
            }
            
            // Detect alternating pattern (oscillation between two scores)
            if (stringIdx > 100) {
                // Check if current score equals score from 2 iterations ago AND differs from last score
                if (abs(bestScore - secondLastScore) < 0.000001 && abs(bestScore - lastScore) > 0.000001) {
                    alternatingCount++;
                    if (alternatingCount >= 20) {
                        std::cout << "Stopping: Detected alternating pattern between scores " << bestScore << " and " << lastScore << std::endl;
                        break;
                    }
                } else {
                    alternatingCount = 0;
                }
            }
            
            // Check for stagnation
            if (bestScore >= lastBestScore - 0.0005) {
                stagnantCount++;
            } else {
                stagnantCount = 0;
            }
            
            // Force exploration if stagnant
            if (stagnantCount > 30) {
                int offset = 1 + (stringIdx % 11) + (stringIdx / 100);
                bestNextNail = (currentNail + offset) % numNails;
                
                while (bestNextNail == currentNail) {
                    bestNextNail = (bestNextNail + 1) % numNails;
                }
                
                stagnantCount = 0;
            }
            
            // Mark coverage - different strategies
            double coverageStrength = 1.0;
            
            if (coverageStrategy == 1) {
                // Strategy 1: Adaptive coverage - decreases more gradually to spread strings
                if (targetStrings > 0) {
                    double progress = (double)stringIdx / targetStrings;
                    coverageStrength = 1.0 - 0.3 * progress; // Less aggressive coverage decay
                } else {
                    coverageStrength = 0.8; // Higher base coverage for spreading
                }
            } else if (coverageStrategy == 2) {
                // Strategy 2: Dynamic threshold - adjust score threshold based on progress
                if (targetStrings > 0) {
                    coverageStrength = 0.9; // Consistent moderate coverage
                } else {
                    coverageStrength = 0.9;
                }
            } else if (coverageStrategy == 3) {
                // Strategy 3: Exploration boost - encourage longer jumps
                if (targetStrings > 0) {
                    double progress = (double)stringIdx / targetStrings;
                    coverageStrength = 0.5 + 0.4 * progress; // Increases coverage over time
                } else {
                    coverageStrength = 0.7;
                }
            }
            
            markLineCoverage(coverage, nails[currentNail], nails[bestNextNail], coverageStrength);
            
            sequence.push_back(bestNextNail);
            currentNail = bestNextNail;
            secondLastScore = lastScore;
            lastScore = bestScore;
            lastBestScore = bestScore;
            
            if ((stringIdx + 1) % 100 == 0) {
                std::cout << "Generated " << (stringIdx + 1) << " strings, last score: " << bestScore << std::endl;
            }
        }
        
        std::cout << "Generated " << sequence.size() << " total strings" << std::endl;
        return sequence;
    }
    
    std::vector<int> generateRectangularStringArt(const ImageData& img, int numNails, int maxStrings) {
        std::cout << "Generating rectangular layout with " << numNails << " nails" << std::endl;
        
        std::vector<std::pair<double, double>> nails;
        int margin = 15;
        int nailsPerSide = numNails / 4;
        
        // Top side
        for (int i = 0; i < nailsPerSide && nails.size() < numNails; i++) {
            double x = margin + (double)i * (img.width - 2 * margin) / std::max(1, nailsPerSide - 1);
            nails.push_back({x, margin});
        }
        
        // Right side  
        for (int i = 1; i < nailsPerSide && nails.size() < numNails; i++) {
            double y = margin + (double)i * (img.height - 2 * margin) / std::max(1, nailsPerSide - 1);
            nails.push_back({img.width - margin, y});
        }
        
        // Bottom side
        for (int i = nailsPerSide - 2; i >= 0 && nails.size() < numNails; i--) {
            double x = margin + (double)i * (img.width - 2 * margin) / std::max(1, nailsPerSide - 1);
            nails.push_back({x, img.height - margin});
        }
        
        // Left side
        for (int i = nailsPerSide - 2; i > 0 && nails.size() < numNails; i--) {
            double y = margin + (double)i * (img.height - 2 * margin) / std::max(1, nailsPerSide - 1);
            nails.push_back({margin, y});
        }
        
        // Use similar algorithm as circular
        std::vector<int> sequence;
        std::vector<std::vector<double>> coverage(img.height, std::vector<double>(img.width, 0.0));
        
        int currentNail = 0;
        sequence.push_back(currentNail);
        
        int targetStrings = maxStrings;
        if (targetStrings > 0) {
            std::cout << "Target strings: " << targetStrings << std::endl;
        } else {
            std::cout << "Target strings: unlimited (will stop when no improvement)" << std::endl;
        }
        
        // Internal safety limit to prevent infinite loops
        int internalLimit = (targetStrings > 0) ? targetStrings : 10000;
        
        double lastScore = -1.0;
        int sameScoreCount = 0;
        const int maxSameScoreCount = 1500; // Stop after 1500 identical scores for rectangular
        
        for (int stringIdx = 0; stringIdx < internalLimit - 1; stringIdx++) {
            int bestNextNail = -1;
            double bestScore = -1.0;
            
            for (int nextNail = 0; nextNail < nails.size(); nextNail++) {
                if (nextNail == currentNail) continue;
                
                bool skipRecent = false;
                for (int lookback = 1; lookback <= 5 && lookback <= sequence.size(); lookback++) {
                    if (sequence[sequence.size() - lookback] == nextNail) {
                        skipRecent = true;
                        break;
                    }
                }
                if (skipRecent) continue;
                
                double score = calculateLineScore(img, coverage, nails[currentNail], nails[nextNail]);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestNextNail = nextNail;
                }
            }
            
            if (bestNextNail == -1) break;
            
            // Check for score stagnation (same exact score repeatedly)
            // Only activate after minimum strings to avoid premature stopping
            if (stringIdx >= 2000) { // Only check stagnation after 2000 strings for rectangular
                if (abs(bestScore - lastScore) < 0.000001) { // Essentially identical scores
                    sameScoreCount++;
                    if (sameScoreCount >= maxSameScoreCount) {
                        std::cout << "Stopping: Score has not changed for " << maxSameScoreCount << " iterations (score: " << bestScore << ")" << std::endl;
                        break;
                    }
                } else {
                    sameScoreCount = 0;
                }
            }
            lastScore = bestScore;
            
            markLineCoverage(coverage, nails[currentNail], nails[bestNextNail], 1.0);
            sequence.push_back(bestNextNail);
            currentNail = bestNextNail;
            
            if ((stringIdx + 1) % 50 == 0) {
                std::cout << "Generated " << (stringIdx + 1) << " strings, last score: " << bestScore << std::endl;
            }
        }
        
        return sequence;
    }
    
    // Color string art generation - generates separate sequences for each CMYK channel
    struct ColorStringSequences {
        std::vector<int> cyanSequence;
        std::vector<int> magentaSequence;
        std::vector<int> yellowSequence;
        std::vector<int> blackSequence;
        int totalStrings;
        
        ColorStringSequences() : totalStrings(0) {}
    };
    
    // Static helper function to get color sequence by letter
    static const std::vector<int>& getSequenceForColor(char colorLetter, const ColorStringSequences& sequences) {
        switch(colorLetter) {
            case 'C': return sequences.cyanSequence;
            case 'M': return sequences.magentaSequence;
            case 'Y': return sequences.yellowSequence;
            case 'K': return sequences.blackSequence;
            default: return sequences.cyanSequence; // Fallback
        }
    }
    
    ColorStringSequences generateColorStringArt(const ImageData& img, int numNails, bool isCircular, int stringsPerColor) {
        ColorStringSequences result;
        
        if (!img.isColorMode) {
            std::cout << "Error: Image not loaded in color mode" << std::endl;
            return result;
        }
        
        std::cout << "Generating color string art with " << stringsPerColor << " strings per color channel" << std::endl;
        
        // Create temporary ImageData objects for each channel
        ImageData cyanImg(img.width, img.height, false);
        ImageData magentaImg(img.width, img.height, false);
        ImageData yellowImg(img.width, img.height, false);
        ImageData blackImg(img.width, img.height, false);
        
        // Copy channel data to grayscale data for processing
        cyanImg.data = img.cyanData;
        magentaImg.data = img.magentaData;
        yellowImg.data = img.yellowData;
        blackImg.data = img.blackData;
        
        std::cout << "Processing CYAN channel..." << std::endl;
        result.cyanSequence = generateStringArt(cyanImg, numNails, isCircular, stringsPerColor);
        
        std::cout << "Processing MAGENTA channel..." << std::endl;
        result.magentaSequence = generateStringArt(magentaImg, numNails, isCircular, stringsPerColor);
        
        std::cout << "Processing YELLOW channel..." << std::endl;
        result.yellowSequence = generateStringArt(yellowImg, numNails, isCircular, stringsPerColor);
        
        std::cout << "Processing BLACK channel..." << std::endl;
        result.blackSequence = generateStringArt(blackImg, numNails, isCircular, stringsPerColor);
        
        result.totalStrings = result.cyanSequence.size() + result.magentaSequence.size() + 
                             result.yellowSequence.size() + result.blackSequence.size();
        
        std::cout << "Color generation complete:" << std::endl;
        std::cout << "  Cyan: " << result.cyanSequence.size() << " strings" << std::endl;
        std::cout << "  Magenta: " << result.magentaSequence.size() << " strings" << std::endl;
        std::cout << "  Yellow: " << result.yellowSequence.size() << " strings" << std::endl;
        std::cout << "  Black: " << result.blackSequence.size() << " strings" << std::endl;
        std::cout << "  Total: " << result.totalStrings << " strings" << std::endl;
        
        return result;
    }
    
    // Color SVG generation - renders each CMYK channel with appropriate colors
    void generateColorSVG(const std::string& filename, const ColorStringSequences& colorSequences, 
                         int numNails, bool isCircular, int imgWidth, int imgHeight, const std::string& threadThickness = "hairline", const std::string& colorOrder = "CMYK") {
        std::ofstream svgFile(filename);
        if (!svgFile.is_open()) {
            std::cout << "Warning: Could not create SVG file: " << filename << std::endl;
            return;
        }
        
        int svgWidth = imgWidth + 40;
        int svgHeight = imgHeight + 40;
        int offsetX = 20, offsetY = 20;
        
        std::vector<std::pair<int, int>> nails;
        
        if (isCircular) {
            int centerX = imgWidth / 2;
            int centerY = imgHeight / 2;
            int radius = std::min(imgWidth, imgHeight) / 2 - 10;
            
            for (int i = 0; i < numNails; i++) {
                double angle = 2.0 * M_PI * i / numNails;
                int x = centerX + (int)(radius * cos(angle));
                int y = centerY + (int)(radius * sin(angle));
                nails.push_back({x + offsetX, y + offsetY});
            }
        } else {
            // Rectangular nail positions
            int margin = 15;
            int nailsPerSide = numNails / 4;
            
            for (int i = 0; i < nailsPerSide && nails.size() < numNails; i++) {
                int x = margin + i * (imgWidth - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({x + offsetX, margin + offsetY});
            }
            
            for (int i = 1; i < nailsPerSide && nails.size() < numNails; i++) {
                int y = margin + i * (imgHeight - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({imgWidth - margin + offsetX, y + offsetY});
            }
            
            for (int i = nailsPerSide - 2; i >= 0 && nails.size() < numNails; i--) {
                int x = margin + i * (imgWidth - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({x + offsetX, imgHeight - margin + offsetY});
            }
            
            for (int i = nailsPerSide - 2; i > 0 && nails.size() < numNails; i--) {
                int y = margin + i * (imgHeight - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({margin + offsetX, y + offsetY});
            }
        }
        
        // Determine thread width based on thickness parameter
        std::string strokeWidth = "0.1";  // hairline default
        if (threadThickness == "0.1mm") strokeWidth = "0.3";
        else if (threadThickness == "0.2mm") strokeWidth = "0.6";
        else if (threadThickness == "0.3mm") strokeWidth = "0.9";
        else if (threadThickness == "0.5mm") strokeWidth = "1.5";
        
        // Write SVG header
        svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        svgFile << "width=\"" << svgWidth << "\" height=\"" << svgHeight << "\">\n";
        svgFile << "  <title>Color String Art - " << colorSequences.totalStrings << " total connections</title>\n";
        svgFile << "  <desc>Generated color string art with " << numNails << " nails in " 
                << (isCircular ? "circular" : "rectangular") << " layout (CMYK mode)</desc>\n\n";
        
        // Background group
        svgFile << "  <g id=\"Background\">\n";
        svgFile << "    <rect width=\"" << svgWidth << "\" height=\"" << svgHeight 
                << "\" fill=\"white\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";
        svgFile << "  </g>\n\n";
        
        // Generate color threads in user-specified order
        std::vector<ColorOrderInfo> orderSequence = getColorOrderSequence(colorOrder);
        
        for (const ColorOrderInfo& colorInfo : orderSequence) {
            const std::vector<int>& sequence = StringArtGenerator::getSequenceForColor(colorInfo.letter, colorSequences);
            
            if (!sequence.empty()) {
                svgFile << "  <!-- " << colorInfo.displayName << " threads -->\n";
                svgFile << "  <g id=\"" << colorInfo.displayName << "\" stroke=\"" << colorInfo.svgColor << "\" stroke-width=\"" << strokeWidth << "\" stroke-opacity=\"0.8\">\n";
                
                for (size_t i = 0; i < sequence.size() - 1; i++) {
                    int nail1 = sequence[i];
                    int nail2 = sequence[i + 1];
                    
                    if (nail1 < nails.size() && nail2 < nails.size()) {
                        svgFile << "    <line id=\"" << nail1 << "-" << nail2 << "\" x1=\"" << nails[nail1].first 
                                << "\" y1=\"" << nails[nail1].second
                                << "\" x2=\"" << nails[nail2].first
                                << "\" y2=\"" << nails[nail2].second << "\"/>\n";
                    }
                }
                
                svgFile << "  </g>\n\n";
            }
        }
        
        // Draw nails group
        svgFile << "  <g id=\"Nails\" fill=\"#666\" stroke=\"#333\" stroke-width=\"0.5\">\n";
        for (size_t i = 0; i < nails.size(); i++) {
            svgFile << "    <circle id=\"nail-" << i << "\" cx=\"" << nails[i].first 
                    << "\" cy=\"" << nails[i].second
                    << "\" r=\"1.5\">\n";
            svgFile << "      <title>Nail " << i << "</title>\n";
            svgFile << "    </circle>\n";
        }
        svgFile << "  </g>\n\n";
        svgFile << "</svg>\n";
        
        svgFile.close();
        std::cout << "[+] Color SVG visualization saved to: " << filename << std::endl;
    }
    
    void generateSVG(const std::string& filename, const std::vector<int>& nailSequence, 
                     int numNails, bool isCircular, int imgWidth, int imgHeight, const std::string& threadThickness = "hairline") {
        std::ofstream svgFile(filename);
        if (!svgFile.is_open()) {
            std::cout << "Warning: Could not create SVG file: " << filename << std::endl;
            return;
        }
        
        int svgWidth = imgWidth + 40;
        int svgHeight = imgHeight + 40;
        int offsetX = 20, offsetY = 20;
        
        std::vector<std::pair<int, int>> nails;
        
        if (isCircular) {
            int centerX = imgWidth / 2;
            int centerY = imgHeight / 2;
            int radius = std::min(imgWidth, imgHeight) / 2 - 10;
            
            for (int i = 0; i < numNails; i++) {
                double angle = 2.0 * M_PI * i / numNails;
                int x = centerX + (int)(radius * cos(angle));
                int y = centerY + (int)(radius * sin(angle));
                nails.push_back({x + offsetX, y + offsetY});
            }
        } else {
            // Rectangular nail positions
            int margin = 15;
            int nailsPerSide = numNails / 4;
            
            for (int i = 0; i < nailsPerSide && nails.size() < numNails; i++) {
                int x = margin + i * (imgWidth - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({x + offsetX, margin + offsetY});
            }
            
            for (int i = 1; i < nailsPerSide && nails.size() < numNails; i++) {
                int y = margin + i * (imgHeight - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({imgWidth - margin + offsetX, y + offsetY});
            }
            
            for (int i = nailsPerSide - 2; i >= 0 && nails.size() < numNails; i--) {
                int x = margin + i * (imgWidth - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({x + offsetX, imgHeight - margin + offsetY});
            }
            
            for (int i = nailsPerSide - 2; i > 0 && nails.size() < numNails; i--) {
                int y = margin + i * (imgHeight - 2 * margin) / std::max(1, nailsPerSide - 1);
                nails.push_back({margin + offsetX, y + offsetY});
            }
        }
        
        // Determine thread width based on thickness parameter
        std::string strokeWidth = "0.1";  // hairline default
        if (threadThickness == "0.1mm") strokeWidth = "0.3";
        else if (threadThickness == "0.2mm") strokeWidth = "0.6";
        else if (threadThickness == "0.3mm") strokeWidth = "0.9";
        else if (threadThickness == "0.5mm") strokeWidth = "1.5";
        
        // Write SVG header
        svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        svgFile << "width=\"" << svgWidth << "\" height=\"" << svgHeight << "\">\n";
        svgFile << "  <title>String Art - " << nailSequence.size() << " connections</title>\n";
        svgFile << "  <desc>Generated string art with " << numNails << " nails in " 
                << (isCircular ? "circular" : "rectangular") << " layout</desc>\n\n";
        
        // Background group
        svgFile << "  <g id=\"Background\">\n";
        svgFile << "    <rect width=\"" << svgWidth << "\" height=\"" << svgHeight 
                << "\" fill=\"white\" stroke=\"#ccc\" stroke-width=\"1\"/>\n";
        svgFile << "  </g>\n\n";
        
        // OPAQUE BLACK THREADS - threads are NOT transparent!
        svgFile << "  <g id=\"Black\" stroke=\"black\" stroke-width=\"" << strokeWidth << "\" stroke-opacity=\"1.0\">\n";
        
        for (size_t i = 0; i < nailSequence.size() - 1; i++) {
            int nail1 = nailSequence[i];
            int nail2 = nailSequence[i + 1];
            
            if (nail1 < nails.size() && nail2 < nails.size()) {
                svgFile << "    <line id=\"" << nail1 << "-" << nail2 << "\" x1=\"" << nails[nail1].first 
                        << "\" y1=\"" << nails[nail1].second
                        << "\" x2=\"" << nails[nail2].first
                        << "\" y2=\"" << nails[nail2].second << "\"/>\n";
            }
        }
        
        svgFile << "  </g>\n\n";
        
        // Draw nails group
        svgFile << "  <g id=\"Nails\" fill=\"#666\" stroke=\"#333\" stroke-width=\"0.5\">\n";
        for (size_t i = 0; i < nails.size(); i++) {
            svgFile << "    <circle id=\"nail-" << i << "\" cx=\"" << nails[i].first 
                    << "\" cy=\"" << nails[i].second
                    << "\" r=\"1.5\">\n";
            svgFile << "      <title>Nail " << i << "</title>\n";
            svgFile << "    </circle>\n";
        }
        svgFile << "  </g>\n\n";
        svgFile << "</svg>\n";
        
        svgFile.close();
        std::cout << "[+] SVG visualization saved to: " << filename << std::endl;
    }

private:
    double calculateLineScore(const ImageData& img, const std::vector<std::vector<double>>& coverage, 
                             const std::pair<double, double>& nail1, const std::pair<double, double>& nail2) {
        double x1 = nail1.first, y1 = nail1.second;
        double x2 = nail2.first, y2 = nail2.second;
        
        double dx = x2 - x1;
        double dy = y2 - y1;
        double length = sqrt(dx*dx + dy*dy);
        
        if (length < 1.0) return 0.0;
        
        int numSamples = (int)(length * 1.2);
        if (numSamples < 2) numSamples = 2;
        
        double totalScore = 0.0;
        int validSamples = 0;
        
        for (int i = 0; i < numSamples; i++) {
            double t = (double)i / (numSamples - 1);
            int x = (int)(x1 + t * dx);
            int y = (int)(y1 + t * dy);
            
            if (x >= 0 && x < img.width && y >= 0 && y < img.height) {
                double darkness = (255.0 - img.at(x, y)) / 255.0;
                
                // Use adjustable contrast enhancement
                darkness = darkness * (1.0 + darkness * m_contrastFactor);
                
                double coverageFactor = std::max(0.1, 1.0 - coverage[y][x] / 6.0);
                
                totalScore += darkness * coverageFactor;
                validSamples++;
            }
        }
        
        return validSamples > 0 ? totalScore / validSamples : 0.0;
    }
    
    void markLineCoverage(std::vector<std::vector<double>>& coverage, 
                         const std::pair<double, double>& nail1, const std::pair<double, double>& nail2,
                         double strength) {
        double x1 = nail1.first, y1 = nail1.second;
        double x2 = nail2.first, y2 = nail2.second;
        
        double dx = x2 - x1;
        double dy = y2 - y1;
        double length = sqrt(dx*dx + dy*dy);
        
        if (length < 1.0) return;
        
        int numSamples = (int)(length * 1.5);
        if (numSamples < 2) numSamples = 2;
        
        for (int i = 0; i < numSamples; i++) {
            double t = (double)i / (numSamples - 1);
            int x = (int)(x1 + t * dx);
            int y = (int)(y1 + t * dy);
            
            if (x >= 0 && x < coverage[0].size() && y >= 0 && y < coverage.size()) {
                coverage[y][x] += strength * 0.8;
            }
        }
    }
};

std::string generateTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto tm = *std::localtime(&time_t);
    
    std::stringstream ss;
    ss << "_" << std::setfill('0')
       << std::setw(4) << (tm.tm_year + 1900)
       << std::setw(2) << (tm.tm_mon + 1)
       << std::setw(2) << tm.tm_mday
       << std::setw(2) << tm.tm_hour
       << std::setw(2) << tm.tm_min
       << std::setw(2) << tm.tm_sec;
    
    return ss.str();
}

void printUsage(const char* programName) {
    std::cout << "String Art Generator - Convert images to nail-and-string art instructions" << std::endl;
    std::cout << "========================================================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Usage: " << programName << " <image_file> [options]" << std::endl;
    std::cout << std::endl;
    std::cout << "Arguments:" << std::endl;
    std::cout << "  image_file               Input image file (PNG, JPG/JPEG, BMP)" << std::endl;
    std::cout << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -n, --nails <num>        Number of nails (50-1000, default: 400)" << std::endl;
    std::cout << "  -s, --strings <num>      Maximum number of strings (0=unlimited, default: 0)" << std::endl;
    std::cout << "  -o, --output <file>      Output filename base (parameters added automatically)" << std::endl;
    std::cout << "  -c, --circular           Use circular layout (default)" << std::endl;
    std::cout << "  -r, --rectangular        Use rectangular layout" << std::endl;
    std::cout << "  --contrast <factor>      Contrast adjustment (0.0-2.0, default: 0.5)" << std::endl;
    std::cout << "  --thread <thickness>     Thread thickness (hairline,0.1mm,0.2mm,0.3mm,0.5mm, default: hairline)" << std::endl;
    std::cout << "  --coverage-strategy <n>  Coverage strategy (0=default, 1=adaptive, 2=dynamic, 3=exploration, default: 0)" << std::endl;
    std::cout << "  --color [order]          Generate color string art with CMYK separation (default order: CMYK)" << std::endl;
    std::cout << "                           Optional order: CMYK, MYKC, YKCM, etc. (default: grayscale mode)" << std::endl;
    std::cout << "  --strings-per-color <n>  Strings per color channel in color mode (default: 2500, max: 2500)" << std::endl;
    std::cout << "  -h, --help               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Output Files:" << std::endl;
    std::cout << "  The program generates two descriptive files based on parameters:" << std::endl;
    std::cout << "  * Text file (.txt) - Step-by-step nail connection instructions" << std::endl;
    std::cout << "  * SVG file (.svg)  - Visual diagram with threads" << std::endl;
    std::cout << "  Grayscale: image.png-n400-s2000-c-0.8-t0.2-cs1.txt" << std::endl;
    std::cout << "  Color:     image.png-n400-c-0.8-h-spc2500-CMYK.txt" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " image.png -n 400 -s 2000                # Grayscale with 2000 strings" << std::endl;
    std::cout << "  " << programName << " photo.png --color MYKC --strings-per-color 1500  # Color with custom order, 1500 per color" << std::endl;
    std::cout << "  " << programName << " portrait.png --color                            # Color with default CMYK order, 2500 per color" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Always use PNG files for testing! For PNG/JPEG support:" << std::endl;
    std::cout << "      pip install Pillow" << std::endl;
}

int main(int argc, char* argv[]) {
    std::string timestamp = generateTimestamp();
    
    std::string inputFile = "";
    std::string outputFile = "";
    int numNails = 400;
    int maxStrings = 0;
    bool isCircular = true;
    double contrastFactor = 0.5;
    std::string threadThickness = "hairline";
    int coverageStrategy = 0; // 0=current/default, 1=adaptive_coverage, 2=dynamic_threshold, 3=exploration_boost
    
    // Color mode options
    bool colorMode = false;
    int stringsPerColor = 2500;  // Default strings per color channel
    std::string colorOrder = "CMYK";  // Default color order
    
    // Parse command line arguments
    // First argument (if not an option) is the input file
    if (argc > 1 && argv[1][0] != '-') {
        inputFile = argv[1];
    }
    
    for (int i = (inputFile.empty() ? 1 : 2); i < argc; i++) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            printUsage(argv[0]);
            return 0;
        }
        else if (arg == "-n" || arg == "--nails") {
            if (i + 1 < argc) {
                numNails = std::atoi(argv[++i]);
            } else {
                std::cout << "Error: --nails requires a number" << std::endl;
                return 1;
            }
        }
        else if (arg == "-s" || arg == "--strings") {
            if (i + 1 < argc) {
                maxStrings = std::atoi(argv[++i]);
            } else {
                std::cout << "Error: --strings requires a number" << std::endl;
                return 1;
            }
        }
        else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) {
                outputFile = argv[++i];
            } else {
                std::cout << "Error: --output requires a filename" << std::endl;
                return 1;
            }
        }
        else if (arg == "-c" || arg == "--circular") {
            isCircular = true;
        }
        else if (arg == "-r" || arg == "--rectangular") {
            isCircular = false;
        }
        else if (arg == "--contrast") {
            if (i + 1 < argc) {
                contrastFactor = std::atof(argv[++i]);
            } else {
                std::cout << "Error: --contrast requires a number" << std::endl;
                return 1;
            }
        }
        else if (arg == "--thread") {
            if (i + 1 < argc) {
                threadThickness = argv[++i];
            } else {
                std::cout << "Error: --thread requires a thickness value" << std::endl;
                return 1;
            }
        }
        else if (arg == "--coverage-strategy") {
            if (i + 1 < argc) {
                coverageStrategy = std::atoi(argv[++i]);
                if (coverageStrategy < 0 || coverageStrategy > 3) {
                    std::cout << "Error: --coverage-strategy must be 0-3" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Error: --coverage-strategy requires a number (0-3)" << std::endl;
                return 1;
            }
        }
        else if (arg == "--color") {
            colorMode = true;
            // Check if next argument is a color order (not starting with -)
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                colorOrder = argv[++i];
                // Convert to uppercase for consistency
                std::transform(colorOrder.begin(), colorOrder.end(), colorOrder.begin(), ::toupper);
                // Validate color order
                if (colorOrder.length() != 4 || 
                    colorOrder.find('C') == std::string::npos ||
                    colorOrder.find('M') == std::string::npos ||
                    colorOrder.find('Y') == std::string::npos ||
                    colorOrder.find('K') == std::string::npos) {
                    std::cout << "Error: Color order must contain exactly C, M, Y, K (e.g. CMYK, MYKC, YKCM)" << std::endl;
                    return 1;
                }
            }
        }
        else if (arg == "--strings-per-color") {
            if (i + 1 < argc) {
                stringsPerColor = std::atoi(argv[++i]);
                if (stringsPerColor < 1 || stringsPerColor > 2500) {
                    std::cout << "Error: --strings-per-color must be between 1 and 2500" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Error: --strings-per-color requires a number" << std::endl;
                return 1;
            }
        }
        else {
            std::cout << "Error: Unknown option: " << arg << std::endl;
            printUsage(argv[0]);
            return 1;
        }
    }
    
    if (inputFile.empty()) {
        std::cout << "Error: Image file needed" << std::endl;
        std::cout << "Usage: " << argv[0] << " <image_file> [options]" << std::endl;
        std::cout << "Use --help for more information" << std::endl;
        return 1;
    }
    
    // Check if file exists
    if (!std::filesystem::exists(inputFile)) {
        std::cout << "Error: Image could not be found: " << inputFile << std::endl;
        return 1;
    }
    
    // Check if file has valid image extension
    std::string ext = inputFile.substr(inputFile.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    if (ext != "png" && ext != "jpg" && ext != "jpeg" && ext != "bmp") {
        std::cout << "Error: Filename is not an image (png, jpg, jpeg, bmp): " << inputFile << std::endl;
        return 1;
    }
    
    if (numNails < 50 || numNails > 1000) {
        std::cout << "Error: Number of nails must be between 50 and 1000" << std::endl;
        return 1;
    }
    
    if (contrastFactor < 0.0 || contrastFactor > 2.0) {
        std::cout << "Error: Contrast factor must be between 0.0 and 2.0" << std::endl;
        return 1;
    }
    
    std::cout << "================== String Art Generator ==================" << std::endl;
    std::cout << "Converting image to nail-and-string art instructions..." << std::endl;
    std::cout << std::endl;
    std::cout << "Session Details:" << std::endl;
    std::cout << "  Timestamp: " << timestamp << std::endl;
    std::cout << "  Input image: " << inputFile << std::endl;
    
    // Generate descriptive filename based on parameters
    std::string baseFilename;
    if (outputFile.empty()) {
        // Use full input filename (including extension)
        baseFilename = inputFile;
    } else {
        baseFilename = outputFile;
    }
    
    // Add ALL parameter suffixes (including defaults) - CONCISE FORMAT
    std::stringstream suffix;
    
    // Always show nail count
    suffix << "-n" << numNails;
    
    // Always show string count (0 = unlimited)
    suffix << "-s" << maxStrings;
    
    // Always show layout type (c=circular, r=rectangular)
    if (isCircular) {
        suffix << "-c";
    } else {
        suffix << "-r";
    }
    
    // Always show contrast factor
    suffix << "-" << std::fixed << std::setprecision(1) << contrastFactor;
    
    // Always show thread thickness (remove "mm" suffix, use "h" for hairline)
    if (threadThickness == "hairline") {
        suffix << "-h";
    } else {
        // Remove "mm" from thickness values like "0.2mm" -> "0.2"
        std::string thickness = threadThickness;
        if (thickness.size() > 2 && thickness.substr(thickness.size()-2) == "mm") {
            thickness = thickness.substr(0, thickness.size()-2);
        }
        suffix << "-t" << thickness;
    }
    
    // Color mode vs Grayscale mode in filename
    if (colorMode) {
        // Color mode: show strings-per-color and color order instead of coverage strategy
        suffix << "-spc" << stringsPerColor << "-" << colorOrder;
    } else {
        // Grayscale mode: show coverage strategy
        int filenameStrategy = coverageStrategy;
        if (maxStrings == 0 && coverageStrategy != 0) {
            filenameStrategy = 0; // Unlimited strings force strategy 0
        } else if (maxStrings > 0 && coverageStrategy == 0) {
            filenameStrategy = 1; // Limited strings default to strategy 1
        }
        suffix << "-cs" << filenameStrategy;
    }
    
    outputFile = baseFilename + suffix.str() + ".txt";
    std::string svgFilename = baseFilename + suffix.str() + ".svg";
    
    std::cout << "  Output files: " << outputFile << std::endl;
    std::cout << "                " << svgFilename << std::endl;
    std::cout << "  Layout type: " << (isCircular ? "Circular" : "Rectangular") << std::endl;
    std::cout << "  Number of nails: " << numNails << std::endl;
    std::cout << "  Max strings: " << (maxStrings > 0 ? std::to_string(maxStrings) : "unlimited") << std::endl;
    std::cout << "  Contrast factor: " << contrastFactor << std::endl;
    std::cout << "  Thread thickness: " << threadThickness << std::endl;
    std::cout << std::endl;
    
    // Load and process image
    StringArtGenerator generator(contrastFactor);
    ImageData img;
    
    std::cout << "Loading and processing image..." << std::endl;
    if (colorMode) {
        std::cout << "Color mode enabled - performing CMYK separation" << std::endl;
    }
    if (!generator.loadImage(inputFile, img, colorMode)) {
        std::cout << "Error: Cannot load image: " << inputFile << std::endl;
        std::cout << "Make sure the file exists and is a supported format." << std::endl;
        std::cout << "For PNG/JPEG files, ensure Python PIL is installed: pip install Pillow" << std::endl;
        return 1;
    }
    
    std::cout << "Image loaded successfully: " << img.width << "x" << img.height << " pixels" << std::endl;
    std::cout << std::endl;
    
    // Generate string art
    std::cout << "Processing..." << std::endl;
    
    if (colorMode) {
        // Color mode: generate separate sequences for each CMYK channel
        std::cout << "Color mode: Generating " << stringsPerColor << " strings per channel" << std::endl;
        
        StringArtGenerator::ColorStringSequences colorSequences = generator.generateColorStringArt(img, numNails, isCircular, stringsPerColor);
        
        if (colorSequences.totalStrings == 0) {
            std::cout << "Error: Failed to generate color string art" << std::endl;
            return 1;
        }
        
        // Generate color text instructions
        std::ofstream txtFile(outputFile);
        if (txtFile.is_open()) {
            txtFile << "Color String Art Generator - CMYK Nail Connection Instructions\n";
            txtFile << "===================================================================\n";
            txtFile << "Generated: " << timestamp << "\n";
            txtFile << "Input image: " << inputFile << "\n";
            txtFile << "Mode: Color (CMYK separation)\n";
            txtFile << "Layout: " << (isCircular ? "Circular" : "Rectangular") << "\n";
            txtFile << "Total nails: " << numNails << "\n";
            txtFile << "Strings per color: " << stringsPerColor << "\n";
            txtFile << "Color order: " << colorOrder << "\n";
            txtFile << "Total connections: " << colorSequences.totalStrings << "\n";
            txtFile << "  - Cyan: " << colorSequences.cyanSequence.size() << " strings\n";
            txtFile << "  - Magenta: " << colorSequences.magentaSequence.size() << " strings\n";
            txtFile << "  - Yellow: " << colorSequences.yellowSequence.size() << " strings\n";
            txtFile << "  - Black: " << colorSequences.blackSequence.size() << " strings\n";
            txtFile << "Contrast factor: " << contrastFactor << "\n";
            txtFile << "Thread thickness: " << threadThickness << "\n";
            txtFile << "\n";
            txtFile << "Color String Art Instructions:\n";
            txtFile << "1. Arrange " << numNails << " nails in a " << (isCircular ? "circle" : "rectangle") << "\n";
            txtFile << "2. Number them 0 to " << (numNails-1) << " going clockwise\n";
            txtFile << "3. You will need FOUR different colored threads: CYAN, MAGENTA, YELLOW, BLACK\n";
            txtFile << "4. Follow each color sequence in the specified order (" << colorOrder << ")\n";
            txtFile << "5. Pull thread tight between each connection\n";
            txtFile << "6. Use OPAQUE threads - threads are NOT transparent!\n";
            txtFile << "\n";
            
            // Generate color sequences in user-specified order
            std::vector<ColorOrderInfo> orderSequence = getColorOrderSequence(colorOrder);
            
            for (const ColorOrderInfo& colorInfo : orderSequence) {
                const std::vector<int>& sequence = StringArtGenerator::getSequenceForColor(colorInfo.letter, colorSequences);
                
                if (!sequence.empty()) {
                    txtFile << colorInfo.displayName << " Thread Sequence (" << sequence.size() << " connections):\n";
                    for (size_t i = 0; i < sequence.size(); i++) {
                        txtFile << sequence[i];
                        if (i < sequence.size() - 1) txtFile << ",";
                        if ((i + 1) % 20 == 0) txtFile << "\n";
                    }
                    txtFile << "\n\n";
                }
            }
            
            // Generate construction tips based on color order
            txtFile << "Construction Tips:\n";
            txtFile << "* Follow the color order: " << colorOrder << "\n";
            txtFile << "* It's recommended to start with darker colors first\n";
            txtFile << "* Each color contributes to the final image - all are important!\n";
            txtFile << "* Use high-quality, opaque threads for best results\n";
            
            txtFile.close();
            std::cout << "[+] Color text instructions saved to: " << outputFile << std::endl;
        }
        
        // Generate color SVG
        generator.generateColorSVG(svgFilename, colorSequences, numNails, isCircular, img.width, img.height, threadThickness, colorOrder);
        
        std::cout << std::endl;
        std::cout << "=================== COLOR SUCCESS! ===================" << std::endl;
        std::cout << "Color string art generation completed successfully!" << std::endl;
        std::cout << "Total nail connections: " << colorSequences.totalStrings << std::endl;
        std::cout << "  Cyan: " << colorSequences.cyanSequence.size() << " strings" << std::endl;
        std::cout << "  Magenta: " << colorSequences.magentaSequence.size() << " strings" << std::endl;
        std::cout << "  Yellow: " << colorSequences.yellowSequence.size() << " strings" << std::endl;
        std::cout << "  Black: " << colorSequences.blackSequence.size() << " strings" << std::endl;
        std::cout << std::endl;
        std::cout << "Files created successfully! You can now:" << std::endl;
        std::cout << "* Open the .txt file for step-by-step CMYK instructions" << std::endl;
        std::cout << "* View the .svg file in a web browser for colored thread visualization" << std::endl;
        std::cout << "* Use CYAN, MAGENTA, YELLOW, and BLACK opaque threads!" << std::endl;
        
    } else {
        // Grayscale mode: use existing logic
        std::vector<int> nailSequence;
        
        // Adjust coverage strategy based on string limits
        int actualCoverageStrategy = coverageStrategy;
        if (maxStrings == 0 && coverageStrategy != 0) {
            // Unlimited strings: force to strategy 0
            actualCoverageStrategy = 0;
            std::cout << "Note: Coverage strategy " << coverageStrategy << " requires limited strings. Using default strategy 0 for unlimited strings." << std::endl;
        } else if (maxStrings > 0 && coverageStrategy == 0) {
            // Limited strings but default strategy: use strategy 1 instead
            actualCoverageStrategy = 1;
            std::cout << "Note: Using coverage strategy 1 (adaptive) for limited strings instead of default strategy 0." << std::endl;
        }
        
        if (actualCoverageStrategy == 0) {
            nailSequence = generator.generateStringArt(img, numNails, isCircular, maxStrings);
        } else {
            nailSequence = generator.generateStringArtExperimental(img, numNails, isCircular, maxStrings, actualCoverageStrategy);
        }
        
        if (nailSequence.empty()) {
            std::cout << "Error: Failed to generate string art" << std::endl;
            return 1;
        }
        
        // Save grayscale text file
        std::ofstream txtFile(outputFile);
        if (txtFile.is_open()) {
            txtFile << "String Art Generator - Nail Connection List\n";
            txtFile << "===========================================\n";
            txtFile << "Generated: " << timestamp << "\n";
            txtFile << "Input image: " << inputFile << "\n";
            txtFile << "Layout: " << (isCircular ? "Circular" : "Rectangular") << "\n";
            txtFile << "Total nails: " << numNails << "\n";
            txtFile << "Number of connections: " << nailSequence.size() << "\n";
            txtFile << "Contrast factor: " << contrastFactor << "\n";
            txtFile << "Thread thickness: " << threadThickness << "\n";
            txtFile << "\n";
            txtFile << "Nail sequence (follow this order to create string art):\n";
            
            for (size_t i = 0; i < nailSequence.size(); i++) {
                txtFile << nailSequence[i];
                if (i < nailSequence.size() - 1) txtFile << ",";
                if ((i + 1) % 20 == 0) txtFile << "\n";
            }
            
            txtFile << "\n\n";
            txtFile << "Instructions:\n";
            txtFile << "1. Arrange " << numNails << " nails in a " << (isCircular ? "circle" : "rectangle") << "\n";
            txtFile << "2. Number them 0 to " << (numNails-1) << " going clockwise\n";
            txtFile << "3. Connect the nails with BLACK thread in the sequence shown above\n";
            txtFile << "4. Pull thread tight between each connection\n";
            txtFile << "5. Use OPAQUE thread - threads are NOT transparent!\n";
            
            txtFile.close();
            std::cout << "[+] Text instructions saved to: " << outputFile << std::endl;
        }
        
        // Generate grayscale SVG
        generator.generateSVG(svgFilename, nailSequence, numNails, isCircular, img.width, img.height, threadThickness);
        
        std::cout << std::endl;
        std::cout << "=================== SUCCESS! ===================" << std::endl;
        std::cout << "String art generation completed successfully!" << std::endl;
        std::cout << "Total nail connections: " << nailSequence.size() << std::endl;
        std::cout << std::endl;
        std::cout << "Files created successfully! You can now:" << std::endl;
        std::cout << "* Open the .txt file for step-by-step instructions" << std::endl;
        std::cout << "* View the .svg file in a web browser for visual reference" << std::endl;
        std::cout << "* Use OPAQUE BLACK threads to create your physical string art!" << std::endl;
    }
    
    return 0;
}
