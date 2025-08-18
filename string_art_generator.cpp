#include "string_art_generator.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <chrono>
#include <filesystem>

StringArtGenerator::StringArtGenerator(double contrastFactor) : m_contrastFactor(contrastFactor) {}

// Grayscale image loading (backward compatibility)
bool StringArtGenerator::loadImage(const std::string& filename, ImageData& img) {
    return loadImage(filename, img, false);
}

// Enhanced image loading with color mode support
bool StringArtGenerator::loadImage(const std::string& filename, ImageData& img, bool colorMode) {
    std::string ext = filename.substr(filename.find_last_of(".") + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    bool loadSuccess = false;
    
    if (colorMode) {
        // Color mode: preserve RGB information
        if (ext == "bmp") {
            loadSuccess = loadBMPColor(filename, img);
        } else if (ext == "png") {
            loadSuccess = loadPNGColor(filename, img);
        } else if (ext == "jpg" || ext == "jpeg") {
            loadSuccess = loadJPEGColor(filename, img);
        }
    } else {
        // Grayscale mode 
        if (ext == "bmp") {
            std::vector<unsigned char> imageData;
            int width, height;
            
            if (loadBMP(filename, imageData, width, height)) {
                img = ImageData(width, height);
                img.data = imageData;
                loadSuccess = true;
            }
        } else if (ext == "png") {
            std::vector<unsigned char> imageData;
            int width, height;
            
            if (loadPNG(filename, imageData, width, height)) {
                img = ImageData(width, height);
                img.data = imageData;
                loadSuccess = true;
            }
        } else if (ext == "jpg" || ext == "jpeg") {
            std::vector<unsigned char> imageData;
            int width, height;
            
            if (loadJPEG(filename, imageData, width, height)) {
                img = ImageData(width, height);
                img.data = imageData;
                loadSuccess = true;
            }
        }
    }
    
    // Resize image for optimal processing if loading was successful
    if (loadSuccess) {
        img.resizeForProcessing();
    }
    
    return loadSuccess;
}

std::vector<int> StringArtGenerator::generateStringArt(const ImageData& img, int numNails, bool isCircular, int maxStrings) {
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
std::vector<int> StringArtGenerator::generateStringArtExperimental(const ImageData& img, int numNails, bool isCircular, int maxStrings, int coverageStrategy) {
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

std::vector<int> StringArtGenerator::generateRectangularStringArt(const ImageData& img, int numNails, int maxStrings) {
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

// ColorStringSequences implementation
StringArtGenerator::ColorStringSequences::ColorStringSequences() : totalStrings(0) {}

// Static helper function to get color sequence by letter
const std::vector<int>& StringArtGenerator::getSequenceForColor(char colorLetter, const ColorStringSequences& sequences) {
    switch(colorLetter) {
        case 'C': return sequences.cyanSequence;
        case 'M': return sequences.magentaSequence;
        case 'Y': return sequences.yellowSequence;
        case 'K': return sequences.blackSequence;
        default: return sequences.cyanSequence; // Fallback
    }
}

StringArtGenerator::ColorStringSequences StringArtGenerator::generateColorStringArt(const ImageData& img, int numNails, bool isCircular, int stringsPerColor) {
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

double StringArtGenerator::calculateLineScore(const ImageData& img, const std::vector<std::vector<double>>& coverage, 
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

void StringArtGenerator::markLineCoverage(std::vector<std::vector<double>>& coverage, 
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
