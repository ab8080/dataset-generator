#pragma once

#include <iostream>
#include <string>
#include <opencv2/opencv.hpp>
#include <cstdlib>
#include <cmath>
#include <map>
#include <vector>
#include <memory>

using namespace cv;

const int MAX_INTENSIVITY = 255;
const int MIN_INTENSIVITY = 0;

class Modifier {
public:
    virtual void ModifyImage(Mat& image) = 0;
};

class Printer : public Modifier {
public:
    Printer() : radius_x(0), radius_y(0) {}

    Printer(int radius_x, int radius_y, int x_lim_, int y_lim_, int density, bool black, float intensivity, bool use_memory) :
            radius_x(radius_x), radius_y(radius_y), x_lim(x_lim_), y_lim(y_lim_),
            density(density), black(black), intensivity(intensivity), 
            use_memory(use_memory) {
        new_intensivity = black ? -255 : 255;
        new_intensivity = static_cast<int>(static_cast<float>(new_intensivity) * intensivity);
        if (x_lim <= 0) {
            x_lim = std::numeric_limits<int>::max();
        }
        if (y_lim <= 0) {
            y_lim = std::numeric_limits<int>::max();
        }
        if (radius_x == 0 && radius_y == 0) {
            use_memory = false;
        }
    }

    void ModifyImage(Mat& image) {
        int rows = 0, cols = 0;
        rows = image.rows;
        cols = image.cols;
        for (int x = 0; x < rows; ++x) {
            uchar* p = image.ptr<uchar>(x);
            for (int y = 0; y < cols; ++y) {
                if (x <= x_lim && y <= y_lim) {
                    p[y] = normalize(p[y], getDistortion(x, y));
                }
            }
        }
    }

protected:

    int getDistortion(int x, int y) {
        if (radius_x >= 1) {
            x = x % radius_x;
        }
        if (radius_y >= 1) {
            y = y % radius_y;
        }
        return changePixelFromMemory(x, y);
    }

    int changePixelFromMemory(int x, int y) {
        if (!use_memory) {
            return changePixel(x, y);
        }
        if (memory.find({x, y}) == memory.end()) {
            memory[{x, y}] = changePixel(x, y);
        }
        return memory.at({x, y});
    }

    uchar normalize(uchar pixel, int distortion) {
        if (distortion > 0) {
            return static_cast<uchar>(std::min(static_cast<int>(pixel) + distortion, MAX_INTENSIVITY));
        }
        return static_cast<uchar>(std::max(static_cast<int>(pixel) + distortion, MIN_INTENSIVITY));
    }

    virtual int changePixel(int x, int y) = 0; // x - vertical, y - horizontal

    int radius_x;
    int radius_y;

    int x_lim;
    int y_lim;

    int density;
    bool black;
    float intensivity;

    int new_intensivity;

    bool use_memory;
    std::map<std::pair<int, int>, int> memory;
};

class LinesPrinter : public Printer {
public:
    LinesPrinter() = delete;

    LinesPrinter(int radius_x, int radius_y, int x_lim, int y_lim,
                      int density, bool black, float intensivity, 
                      int start_, int end_, bool horizontal_,
                      bool use_memory = false) : 
                        Printer(radius_x, radius_y, x_lim, y_lim, density, black, intensivity, use_memory) {
        horizontal = horizontal_;
        start = start_;
        end = end_;
    }
private:
    int changePixel(int x, int y) {
        int coordinate = horizontal ? x : y;
        if ((coordinate >= start && coordinate <= end) && (std::rand() % 100 < density)) {
            return new_intensivity;
        } else {
            return 0; 
        }
    }

    bool horizontal;
    int start;
    int end;
};

class BlobPrinter : public Printer {
public:
    BlobPrinter() = delete;

    BlobPrinter(int radius_x, int radius_y, int x_lim, int y_lim,
                      int density, bool black, float intensivity, 
                      int point_x_, int point_y_, int radius_a_, int radius_b_,
                      bool use_memory = false) : 
                        Printer(radius_x, radius_y, x_lim, y_lim, density, black, intensivity, use_memory) {
        point_x = point_x_;
        point_y = point_y_;
        radius_a = radius_a_;
        radius_b = radius_b_;
    }

private:
    int changePixel(int x, int y) {
        int a_pow = radius_a * radius_a;
        int b_pow = radius_b * radius_b;
        int length = ((x - point_x) * (x - point_x)) * b_pow + ((y - point_y) * (y - point_y)) * a_pow;
        if (length < a_pow * b_pow && (std::rand() % 100 <= density)) {
            return new_intensivity;
        } else {
            return 0; 
        }
    }

    int point_x;
    int point_y;
    int radius_a;
    int radius_b;
};

class SinPrinter : public Printer {
public:
    SinPrinter() = delete;

    SinPrinter(int radius_x, int radius_y, int x_lim, int y_lim,
                      int density, bool black, float intensivity,
                      int start_, int shift_, int amplitude_, float period_, bool horizontal_,
                      bool use_memory = false) : 
                        Printer(radius_x, radius_y, x_lim, y_lim, density, black, intensivity, use_memory) {
        start = start_;
        shift = shift_;
        amplitude = amplitude_;
        period = period_;
        horizontal = horizontal_;
    }

private:
    int changePixel(int x, int y) {
        if (horizontal) {
            std::swap(x, y);
        }
        if ((y > start) && (std::abs(sin(static_cast<float>(x - shift) / period)) * amplitude > y - start) && (std::rand() % 100 < density)) {
            return new_intensivity;
        } else {
            return 0; 
        }
    }

    int start;
    int shift;
    int amplitude;
    float period;
    bool horizontal;
};

class BlurPrinter : public Modifier {
public:
    BlurPrinter() = delete;

    BlurPrinter(float intensivity) : intensivity(intensivity) {}

    void ModifyImage(Mat& image) {
        blur(image, image, Size(image.rows * intensivity, image.cols * intensivity));
    }

private:
    float intensivity;
};

class PrinterStack {
public:
    PrinterStack() {}

    PrinterStack(const PrinterStack&) = delete;

    void AddLayer(std::unique_ptr<Modifier> printer) {
        // Should move layers in here, to avoid copy and referance invalidation
        layers.push_back(std::move(printer));
    }

    void ProcessImage(Mat& image) const {
        for (auto&& i : layers) {
            i->ModifyImage(image);
        }
    }

    void Clear() {
        layers.clear();
    }

private:
    std::vector<std::unique_ptr<Modifier>> layers;
};
