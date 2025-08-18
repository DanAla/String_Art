#include "svg_generator.h"
#include <fstream>
#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ColorOrderInfo implementation
ColorOrderInfo::ColorOrderInfo(char l, const std::string& n, const std::string& d, const std::string& c)
    : letter(l), name(n), displayName(d), svgColor(c) {}

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

void generateColorSVG(const std::string& filename, const StringArtGenerator::ColorStringSequences& colorSequences, 
                     int numNails, bool isCircular, int imgWidth, int imgHeight, const std::string& threadThickness, const std::string& colorOrder, double paperWidth, double paperHeight) {
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
    
    // Scale to fit paper size
    double paperWidthMM = paperWidth;
    double paperHeightMM = paperHeight;
    
    // Calculate scale factor to fit image inside paper dimensions
    double scaleX = paperWidthMM / svgWidth;
    double scaleY = paperHeightMM / svgHeight;
    double scale = std::min(scaleX, scaleY);
    
    // Determine thread width based on thickness parameter - adjusted for scale to maintain physical size
    double physicalThreadMM = 0.1;  // hairline default in mm
    if (threadThickness == "0.1mm") physicalThreadMM = 0.1;
    else if (threadThickness == "0.2mm") physicalThreadMM = 0.2;
    else if (threadThickness == "0.3mm") physicalThreadMM = 0.3;
    else if (threadThickness == "0.5mm") physicalThreadMM = 0.5;
    
    // Convert to viewBox units: divide by scale to counteract the SVG scaling
    double strokeWidthInViewBox = physicalThreadMM / scale;
    std::string strokeWidth = std::to_string(strokeWidthInViewBox);
    
    // Apply scale to SVG dimensions
    svgWidth = (int)(svgWidth * scale);
    svgHeight = (int)(svgHeight * scale);
    
    // Write SVG header with millimeter units
    svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    svgFile << "width=\"" << svgWidth << "mm\" height=\"" << svgHeight << "mm\" ";
    svgFile << "viewBox=\"0 0 " << (imgWidth + 40) << " " << (imgHeight + 40) << "\">\n";
    svgFile << "  <title>Color String Art - " << colorSequences.totalStrings << " total connections</title>\n";
    svgFile << "  <desc>Generated color string art with " << numNails << " nails in " 
            << (isCircular ? "circular" : "rectangular") << " layout (CMYK mode)</desc>\n\n";
    
    // No background - paper is already white and background interferes with CNC machines
    
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
    
    // Draw nails group - small reference points only
    svgFile << "  <g id=\"Nails\" fill=\"#999\" stroke=\"none\">\n";
    for (size_t i = 0; i < nails.size(); i++) {
        svgFile << "    <circle id=\"nail-" << i << "\" cx=\"" << nails[i].first 
                << "\" cy=\"" << nails[i].second
                << "\" r=\"0.3\">\n";
        svgFile << "      <title>Nail " << i << "</title>\n";
        svgFile << "    </circle>\n";
    }
    svgFile << "  </g>\n\n";
    svgFile << "</svg>\n";
    
    svgFile.close();
    std::cout << "[+] Color SVG visualization saved to: " << filename << std::endl;
}

void generateSVG(const std::string& filename, const std::vector<int>& nailSequence, 
                 int numNails, bool isCircular, int imgWidth, int imgHeight, const std::string& threadThickness, double paperWidth, double paperHeight) {
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
    
    // Scale to fit paper size
    double paperWidthMM = paperWidth;
    double paperHeightMM = paperHeight;
    
    // Calculate scale factor to fit image inside paper dimensions
    double scaleX = paperWidthMM / svgWidth;
    double scaleY = paperHeightMM / svgHeight;
    double scale = std::min(scaleX, scaleY);
    
    // Determine thread width based on thickness parameter - adjusted for scale to maintain physical size
    double physicalThreadMM = 0.1;  // hairline default in mm
    if (threadThickness == "0.1mm") physicalThreadMM = 0.1;
    else if (threadThickness == "0.2mm") physicalThreadMM = 0.2;
    else if (threadThickness == "0.3mm") physicalThreadMM = 0.3;
    else if (threadThickness == "0.5mm") physicalThreadMM = 0.5;
    
    // Convert to viewBox units: divide by scale to counteract the SVG scaling
    double strokeWidthInViewBox = physicalThreadMM / scale;
    std::string strokeWidth = std::to_string(strokeWidthInViewBox);
    
    // Apply scale to SVG dimensions
    svgWidth = (int)(svgWidth * scale);
    svgHeight = (int)(svgHeight * scale);
    
    // Write SVG header with millimeter units
    svgFile << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    svgFile << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
    svgFile << "width=\"" << svgWidth << "mm\" height=\"" << svgHeight << "mm\" ";
    svgFile << "viewBox=\"0 0 " << (imgWidth + 40) << " " << (imgHeight + 40) << "\">\n";
    svgFile << "  <title>String Art - " << nailSequence.size() << " connections</title>\n";
    svgFile << "  <desc>Generated string art with " << numNails << " nails in " 
            << (isCircular ? "circular" : "rectangular") << " layout</desc>\n\n";
    
    // No background - paper is already white and background interferes with CNC machines
    
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
    
    // Draw nails group - small reference points only
    svgFile << "  <g id=\"Nails\" fill=\"#999\" stroke=\"none\">\n";
    for (size_t i = 0; i < nails.size(); i++) {
        svgFile << "    <circle id=\"nail-" << i << "\" cx=\"" << nails[i].first 
                << "\" cy=\"" << nails[i].second
                << "\" r=\"0.3\">\n";
        svgFile << "      <title>Nail " << i << "</title>\n";
        svgFile << "    </circle>\n";
    }
    svgFile << "  </g>\n\n";
    svgFile << "</svg>\n";
    
    svgFile.close();
    std::cout << "[+] SVG visualization saved to: " << filename << std::endl;
}
