# String Art Generator

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/)
[![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)](https://github.com/yourusername/string-art-generator)

> Transform your digital images into stunning physical string art with step-by-step nail connection instructions.

## ✨ Features

### 🎯 **Dual Modes**
- **Grayscale Mode**: Single black thread with advanced coverage strategies
- **Color Mode**: CMYK separation using cyan, magenta, yellow, and black threads

### 🔧 **Layout Options**
- **Circular**: Nails arranged in a perfect circle
- **Rectangular**: Nails distributed around rectangle perimeter

### ⚡ **Advanced Algorithms**
- Greedy optimization algorithm for optimal string paths
- Multiple coverage strategies (adaptive, dynamic, exploration boost)
- Adjustable contrast enhancement
- Anti-repetition mechanisms

### 📐 **Customizable Parameters**
- Number of nails (50-1000)
- Maximum strings (unlimited or limited)
- Thread thickness visualization
- Color channel ordering
- Contrast adjustment

### 📄 **Output Files**
- **Text Instructions** (.txt): Step-by-step nail connection sequence
- **SVG Visualization** (.svg): Professional-grade vector format for printing and CNC machining

### 🏭 **Professional Features**
- **Paper Size Optimization**: Auto-scales to 609.6×914.4mm paper format (24×36 banana units)
- **Physical Thread Accuracy**: Thread thickness maintained at exact physical dimensions
- **CNC Machine Compatible**: Clean SVG output without interfering background elements
- **Built-in PNG Decoder**: Full PNG support with dynamic Huffman compression decoding

## 🚀 Quick Start

### Prerequisites
- C++17 compatible compiler (MinGW-w64, GCC, MSVC, Clang)
- Windows or Linux operating system
- **No external dependencies required** - PNG, JPEG, and BMP support built-in

### Installation

1. **Clone the repository**
   ```bash
   git clone https://github.com/yourusername/string-art-generator.git
   cd string-art-generator
   ```

2. **Build the project**
   ```bash
   # Windows
   .\build.bat
   
   # Linux/Mac
   g++ -std=c++17 -O2 -o string_art String_Art.cpp image_processing.cpp string_art_generator.cpp svg_generator.cpp
   ```

3. **Run with an image**
   ```bash
   # Basic grayscale string art
   .\String_Art.exe your_image.bmp -n 400 -s 2000
   
   # Color string art with CMYK
   .\String_Art.exe your_image.bmp --color -n 300 --strings-per-color 1500
   ```

## 📖 User Manual

### Basic Usage

```bash
String_Art.exe <image_file> [options]
```

### Command Line Options

| Option | Description | Default | Range/Options |
|--------|-------------|---------|---------------|
| `image_file` | Input image file | Required | PNG, JPG, JPEG, BMP |
| `-n, --nails <num>` | Number of nails | 400 | 50-1000 |
| `-s, --strings <num>` | Maximum strings (0=unlimited) | 0 | 0+ |
| `-o, --output <file>` | Output filename base | Auto-generated | Any valid filename |
| `-c, --circular` | Use circular layout | ✓ Default | - |
| `-r, --rectangular` | Use rectangular layout | - | - |
| `--contrast <factor>` | Contrast adjustment | 0.5 | 0.0-2.0 |
| `--thread <thickness>` | Physical thread thickness | 0.1mm | 0.1mm, 0.2mm, 0.3mm, 0.5mm |
| `--coverage-strategy <n>` | Coverage strategy | 0 | 0=default, 1=adaptive, 2=dynamic, 3=exploration |
| `--color [order]` | Color mode with CMYK | Off | CMYK, MYKC, YKCM, etc. |
| `--strings-per-color <n>` | Strings per color channel | 2500 | 1-2500 |

### Examples

#### 🖤 **Grayscale String Art**
```bash
# Basic circular grayscale with 400 nails and 2000 strings
String_Art.exe portrait.bmp -n 400 -s 2000

# Rectangular layout with adaptive coverage
String_Art.exe landscape.png -r -n 300 -s 1500 --coverage-strategy 1

# High contrast with thick thread visualization
String_Art.exe photo.jpg --contrast 1.5 --thread 0.3mm

# Low contrast for subtle, soft string patterns
String_Art.exe portrait.bmp --contrast 0.2 -n 500
```

#### 🌈 **Color String Art**
```bash
# Default CMYK color order
String_Art.exe portrait.bmp --color -n 300 --strings-per-color 2000

# Custom color order: Magenta → Yellow → Black → Cyan
String_Art.exe artwork.png --color MYKC --strings-per-color 1500

# Rectangular color layout
String_Art.exe landscape.jpg --color -r -n 400
```

### Understanding Contrast Parameter

The `--contrast` parameter (range: 0.0-2.0, default: 0.5) significantly affects how the algorithm prioritizes darker areas in your image:

#### How Contrast Works
- **Low values (0.0-0.5)**: Subtle enhancement, softer string patterns
  - Produces more evenly distributed strings
  - Good for portraits with smooth gradations
  - Results in gentler, more organic-looking string art

- **Medium values (0.5-1.0)**: Balanced enhancement (recommended for most images)
  - Default value provides good detail without over-emphasis
  - Works well for most photographs and artwork

- **High values (1.0-2.0)**: Strong enhancement, dramatic contrast
  - Heavily emphasizes dark areas over light areas
  - Creates bold, high-contrast string art
  - Best for images with strong shadows or graphic designs
  - May create clustering of strings in very dark regions

#### Technical Details
The contrast formula enhances pixel darkness using: `darkness × (1.0 + darkness × contrast_factor)`

This means:
- **Contrast 0.0**: No enhancement (darkness remains unchanged)
- **Contrast 1.0**: Dark areas (0.8 darkness) become 1.44× darker, light areas (0.2 darkness) become 1.04× darker
- **Contrast 2.0**: Maximum enhancement - dark areas get dramatically prioritized

#### Practical Examples
```bash
# Soft, even coverage for portraits
String_Art.exe portrait.jpg --contrast 0.3

# Standard balanced approach
String_Art.exe photo.bmp --contrast 0.8

# Bold, dramatic effect for graphic images
String_Art.exe logo.png --contrast 1.8
```

### Understanding Output Files

The program generates descriptively named files based on your parameters:

#### Filename Format
```
image.ext-n[nails]-s[strings]-[layout]-[contrast]-[thread]-[mode_params].txt/.svg
```

#### Examples
- **Grayscale**: `portrait.bmp-n400-s2000-c-0.5-h-cs1.txt`
- **Color**: `photo.png-n300-c-0.8-t0.2-spc1500-CMYK.svg`

#### File Contents

**Text File (.txt)**
- Complete nail connection sequence
- Setup instructions
- Material specifications
- Construction tips

**SVG File (.svg)**
- Professional vector format scaled to 609.6×914.4mm paper (24×36 banana units)
- Accurate physical thread thickness maintained regardless of scaling
- CNC machine compatible (no background interference)
- Minimal nail markers for reference only
- Color-coded threads (for color mode)
- Ready for professional printing or laser cutting

## 🔨 Building Physical String Art

### Materials Needed

**For Grayscale:**
- Wooden board or canvas
- Small nails (finishing nails work well)
- Black thread (opaque, not transparent!)
- Hammer
- Ruler or compass (for nail placement)

**For Color Mode:**
- Same as above, plus:
- Cyan thread
- Magenta thread  
- Yellow thread
- Black thread

### Step-by-Step Process

1. **Prepare the Board**
   - **Recommended size**: 609.6×914.4mm (matches SVG scaling, or 24×36 banana units)
   - Alternatively: any large format (300mm+ minimum diameter/width)
   - Mark nail positions using the chosen layout

2. **Install Nails**
   - Hammer nails around perimeter
   - Leave ~5mm of nail exposed
   - Number nails 0 to N-1 going clockwise

3. **Follow Instructions**
   - Open the generated .txt file
   - Follow the nail sequence exactly
   - Pull thread tight between connections
   - For color mode: complete each color sequence in order

4. **Finishing**
   - Secure final thread end
   - Trim excess thread
   - Optionally frame your artwork

### 💡 Pro Tips

- **All formats process equally fast** - PNG, JPEG, and BMP are all natively supported
- **Use opaque thread** - transparency reduces contrast significantly
- **Higher nail count** = more detail but longer construction time
- **SVG files scale perfectly** - designed for 609.6×914.4mm professional printing
- **Thread thickness is physically accurate** - 0.1mm setting = actual 0.1mm threads
- **CNC compatible** - SVG files work directly with laser cutters and plotters
- **Save your settings** - descriptive filenames help reproduce results

## 🏗️ Building from Source

### Project Structure
```
string-art-generator/
├── String_Art.cpp           # Main program and CLI
├── image_processing.h/cpp   # Image loading and processing
├── string_art_generator.h/cpp # Core string art algorithms
├── svg_generator.h/cpp      # SVG output generation
├── build.bat               # Windows build script
└── README.md              # This file
```

### Build Requirements

#### Windows
- MinGW-w64 or Visual Studio 2019+
- C++17 support

#### Linux/macOS
- GCC 7+ or Clang 5+
- C++17 support

### Build Commands

#### Windows (Batch Script)
```cmd
.\build.bat
```

#### Manual Compilation
```bash
# Windows with MinGW
g++ -std=c++17 -static-libgcc -static-libstdc++ -O2 -o String_Art.exe String_Art.cpp image_processing.cpp string_art_generator.cpp svg_generator.cpp

# Linux/macOS
g++ -std=c++17 -O2 -o string_art String_Art.cpp image_processing.cpp string_art_generator.cpp svg_generator.cpp
```

### Image Format Support

The application includes **built-in support** for all major image formats:
- **PNG**: Full support including dynamic Huffman compression decoding
- **JPEG**: Complete JPEG decoder with all compression modes
- **BMP**: Native Windows bitmap support

**No external dependencies required** - all image processing is handled internally.

## 🧠 Algorithm Details

### String Path Optimization

The program uses a **greedy algorithm** with several optimizations:

1. **Line Score Calculation**
   - Samples points along potential string lines
   - Calculates darkness coverage from original image
   - Applies contrast enhancement
   - Considers existing string coverage to avoid overuse

2. **Coverage Strategies**
   - **Default (0)**: Constant moderate coverage
   - **Adaptive (1)**: Gradually decreasing coverage
   - **Dynamic (2)**: Progress-based threshold adjustment
   - **Exploration (3)**: Distance bonuses for longer connections

3. **Anti-Repetition**
   - Avoids recently used nails (lookback window)
   - Detects oscillating patterns
   - Forces exploration when stagnant

### Color Separation

Color mode uses **CMYK color separation**:
- Converts RGB to CMYK color space
- Processes each channel independently
- Generates separate string sequences
- Supports custom color ordering

## 🐛 Troubleshooting

### Common Issues

**"Cannot load image" error**
- Ensure file exists and path is correct
- All formats (PNG, JPEG, BMP) work without external dependencies
- Check image file is not corrupted

**Build errors**
- Verify C++17 compiler support
- Check all source files are present
- On Windows: Use the provided build.bat script

**Poor string art quality**
- Increase number of nails (-n option)
- Adjust contrast factor (--contrast)
- Try different coverage strategies
- Ensure high-contrast source images

### Performance Notes

- Images are automatically resized (400px max on short side)
- Processing time scales with nail count and string count
- Color mode takes ~4x longer than grayscale
- Large nail counts (800+) may take several minutes

## 🤝 Contributing

We welcome contributions! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests if applicable
5. Submit a pull request

### Development Guidelines
- Follow existing code style
- Maintain C++17 compatibility
- Update documentation for new features
- Test on multiple platforms when possible

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Inspired by traditional string art techniques
- Algorithm concepts from computational geometry
- Community feedback and testing

## 📞 Support

- 🐛 **Issues**: [GitHub Issues](https://github.com/yourusername/string-art-generator/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/yourusername/string-art-generator/discussions)

---

<div align="center">
  <p><strong>Transform pixels into physical art, one string at a time! 🧵✨</strong></p>
</div>



