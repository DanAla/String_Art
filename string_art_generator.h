#pragma once

#include "image_processing.h"
#include <vector>
#include <string>

class StringArtGenerator {
private:
    double m_contrastFactor;
    
public:
    StringArtGenerator(double contrastFactor = 0.5);
    
    // Grayscale image loading (backward compatibility)
    bool loadImage(const std::string& filename, ImageData& img);
    
    // Enhanced image loading with color mode support
    bool loadImage(const std::string& filename, ImageData& img, bool colorMode);
    
    std::vector<int> generateStringArt(const ImageData& img, int numNails, bool isCircular, int maxStrings = 0);
    
    // EXPERIMENTAL coverage strategies - DO NOT modify original generateStringArt
    std::vector<int> generateStringArtExperimental(const ImageData& img, int numNails, bool isCircular, int maxStrings = 0, int coverageStrategy = 1);
    
    std::vector<int> generateRectangularStringArt(const ImageData& img, int numNails, int maxStrings);
    
    // Color string art generation - generates separate sequences for each CMYK channel
    struct ColorStringSequences {
        std::vector<int> cyanSequence;
        std::vector<int> magentaSequence;
        std::vector<int> yellowSequence;
        std::vector<int> blackSequence;
        int totalStrings;
        
        ColorStringSequences();
    };
    
    // Static helper function to get color sequence by letter
    static const std::vector<int>& getSequenceForColor(char colorLetter, const ColorStringSequences& sequences);
    
    ColorStringSequences generateColorStringArt(const ImageData& img, int numNails, bool isCircular, int stringsPerColor);

private:
    double calculateLineScore(const ImageData& img, const std::vector<std::vector<double>>& coverage, 
                             const std::pair<double, double>& nail1, const std::pair<double, double>& nail2);
    
    void markLineCoverage(std::vector<std::vector<double>>& coverage, 
                         const std::pair<double, double>& nail1, const std::pair<double, double>& nail2,
                         double strength);
};
