#pragma once

#include "string_art_generator.h"
#include <string>
#include <vector>

// Color order helper functions
struct ColorOrderInfo {
    char letter;
    std::string name;
    std::string displayName;
    std::string svgColor;
    
    ColorOrderInfo(char l, const std::string& n, const std::string& d, const std::string& c);
};

std::vector<ColorOrderInfo> getColorOrderSequence(const std::string& colorOrder);

void generateColorSVG(const std::string& filename, const StringArtGenerator::ColorStringSequences& colorSequences, 
                     int numNails, bool isCircular, int imgWidth, int imgHeight, const std::string& threadThickness = "hairline", const std::string& colorOrder = "CMYK", double paperWidth = 609.6, double paperHeight = 914.4);

void generateSVG(const std::string& filename, const std::vector<int>& nailSequence, 
                 int numNails, bool isCircular, int imgWidth, int imgHeight, const std::string& threadThickness = "hairline", double paperWidth = 609.6, double paperHeight = 914.4);
