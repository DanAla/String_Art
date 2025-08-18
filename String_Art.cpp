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
    std::cout << "  --thread <thickness>     Thread thickness (0.1mm,0.2mm,0.3mm,0.5mm, default: 0.1mm)" << std::endl;
    std::cout << "  --coverage-strategy <n>  Coverage strategy (0=default, 1=adaptive, 2=dynamic, 3=exploration, default: 0)" << std::endl;
    std::cout << "  --color [order]          Generate color string art with CMYK separation (default order: CMYK)" << std::endl;
    std::cout << "                           Optional order: CMYK, MYKC, YKCM, etc. (default: grayscale mode)" << std::endl;
    std::cout << "  --strings-per-color <n>  Strings per color channel in color mode (default: 2500, max: 2500)" << std::endl;
    std::cout << "  --paper-size <wxh>       Paper size in mm (default: 609.6x914.4mm, A4: 210x297, A3: 297x420)" << std::endl;
    std::cout << "  -h, --help               Show this help message" << std::endl;
    std::cout << std::endl;
    std::cout << "Output Files:" << std::endl;
    std::cout << "  The program generates two descriptive files based on parameters:" << std::endl;
    std::cout << "  * Text file (.txt) - Step-by-step nail connection instructions" << std::endl;
    std::cout << "  * SVG file (.svg)  - Visual diagram with threads" << std::endl;
    std::cout << "  Grayscale: image.png-n400-s2000-c-0.8-t0.2-cs1.txt" << std::endl;
    std::cout << "  Color:     image.png-n400-c-0.8-t0.1-spc2500-CMYK.txt" << std::endl;
    std::cout << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << programName << " image.png -n 400 -s 2000                # Grayscale with 2000 strings" << std::endl;
    std::cout << "  " << programName << " photo.png --color MYKC --strings-per-color 1500  # Color with custom order, 1500 per color" << std::endl;
    std::cout << "  " << programName << " portrait.png --color                            # Color with default CMYK order, 2500 per color" << std::endl;
    std::cout << std::endl;
    std::cout << "Note: Always use PNG files for testing! BMP files are natively supported." << std::endl;
    std::cout << "      For PNG/JPEG support, ensure appropriate image libraries are available." << std::endl;
}

int main(int argc, char* argv[]) {
    std::string timestamp = generateTimestamp();
    
    std::string inputFile = "";
    std::string outputFile = "";
    int numNails = 400;
    int maxStrings = 0;
    bool isCircular = true;
    double contrastFactor = 0.5;
    std::string threadThickness = "0.1mm";
    int coverageStrategy = 0; // 0=current/default, 1=adaptive_coverage, 2=dynamic_threshold, 3=exploration_boost
    
    // Color mode options
    bool colorMode = false;
    int stringsPerColor = 2500;  // Default strings per color channel
    std::string colorOrder = "CMYK";  // Default color order
    
    // Paper size options (in millimeters)
    double paperWidth = 609.6;   // Default: 24x36 banana units
    double paperHeight = 914.4;
    
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
        else if (arg == "--paper-size") {
            if (i + 1 < argc) {
                std::string sizeStr = argv[++i];
                size_t xPos = sizeStr.find('x');
                if (xPos == std::string::npos) {
                    std::cout << "Error: --paper-size requires format like 609.6x914.4" << std::endl;
                    return 1;
                }
                try {
                    paperWidth = std::stod(sizeStr.substr(0, xPos));
                    paperHeight = std::stod(sizeStr.substr(xPos + 1));
                    if (paperWidth <= 0 || paperHeight <= 0) {
                        std::cout << "Error: Paper dimensions must be positive" << std::endl;
                        return 1;
                    }
                } catch (const std::exception&) {
                    std::cout << "Error: Invalid paper size format. Use like: 609.6x914.4" << std::endl;
                    return 1;
                }
            } else {
                std::cout << "Error: --paper-size requires dimensions like 609.6x914.4" << std::endl;
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
    
    // Always show thread thickness (remove "mm" suffix)
    // Remove "mm" from thickness values like "0.2mm" -> "0.2"
    std::string thickness = threadThickness;
    if (thickness.size() > 2 && thickness.substr(thickness.size()-2) == "mm") {
        thickness = thickness.substr(0, thickness.size()-2);
    }
    suffix << "-t" << thickness;
    
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
        std::cout << "For PNG/JPEG files, ensure appropriate image libraries are available." << std::endl;
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
        generateColorSVG(svgFilename, colorSequences, numNails, isCircular, img.width, img.height, threadThickness, colorOrder, paperWidth, paperHeight);
        
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
        generateSVG(svgFilename, nailSequence, numNails, isCircular, img.width, img.height, threadThickness, paperWidth, paperHeight);
        
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
