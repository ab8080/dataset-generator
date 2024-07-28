#include <string>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <vector> 

namespace fs = std::filesystem;

#include <Distortions.hpp>

std::vector<std::string> split(std::string s, std::string delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = s.find(delimiter, pos_start)) != std::string::npos) {
        token = s.substr (pos_start, pos_end - pos_start);
        pos_start = pos_end + delim_len;
        res.push_back (token);
    }

    res.push_back (s.substr (pos_start));
    return res;
}

void ProcessFile(const std::string& image_path, const std::string& dir_path, const PrinterStack& stack, const std::string& stack_name) {
    std::string filename = fs::path(image_path).stem();
    std::string extension = fs::path(image_path).extension();
    if (extension != ".jpg" && extension != ".jpeg") {
        std::cout << "WARNING: NOT VALID FORMAT : " << extension << '\n';
        return;
    }
    Mat image = imread(image_path, IMREAD_GRAYSCALE);
    //std::cout << filename + "_" + stack_name << "  " << image.rows << 'x' << image.cols << std::endl;
    stack.ProcessImage(image);
    imwrite(dir_path + "/" + filename + "_" + stack_name + extension, image);
}

int main(int argc, char *argv[]) {
    if (argc < 4) {
        std::cout << "Wrong argument amount\n";
        return 1;
    }

    std::string image_path = argv[1];
    std::string dir_path = argv[2];
    fs::create_directory(dir_path);
    std::string noize_config_path = argv[3];

    

    std::string buffer;
    std::fstream noize_config(noize_config_path);

    PrinterStack stack;
    std::string stack_name;

    for (std::string line; std::getline(noize_config, line);) {
        if (line.size() > 0 && line[0] == '/') {
            continue;
        }
        if (line.size() > 0 && line != "\n") {
            std::vector parsed = split(line, " ");
            if (parsed.size() == 1) {
                stack_name = parsed[0];
                continue;
            }
            int r_x, r_y, x_lim, y_lim, density;
            bool black;
            float intensivity;
            if (parsed.size() > 7) {
                r_x = std::stoi(parsed[1]);
                r_y = std::stoi(parsed[2]);
                x_lim = std::stoi(parsed[3]);
                x_lim = std::stoi(parsed[4]);
                density = std::stoi(parsed[5]);
                black = parsed[6] == "0" ? false : true;
                intensivity = std::stof(parsed[7]);
            }
            if (parsed[0] == "Line") {
                if (parsed.size() < 11) {
                    std::cout << "Warning : " << line << "  //Doesn't satisfy format\n";
                    continue;
                }
                int start = std::stoi(parsed[8]);
                int end = std::stoi(parsed[9]);
                int horizontal = parsed[10] ==  "0" ? false : true;
                bool memory = false;
                if (parsed.size() > 11) {
                    memory = parsed[11] ==  "0" ? false : true;
                }
                auto p = std::make_unique<LinesPrinter>(r_x, r_y, x_lim, y_lim, density, black, intensivity, start, end, horizontal, memory);
                stack.AddLayer(std::move(p));
            }
            else if (parsed[0] == "Blob") {
                if (parsed.size() < 12) {
                    std::cout << "Warning : " << line << "  //Doesn't satisfy format\n";
                    continue;
                }
                int point_x = std::stoi(parsed[8]);
                int point_y = std::stoi(parsed[9]);
                int radius_a = std::stoi(parsed[10]);
                int radius_b = std::stoi(parsed[11]);
                bool memory = false;
                if (parsed.size() > 12) {
                    memory = parsed[12] ==  "0" ? false : true;
                }
                auto p = std::make_unique<BlobPrinter>(r_x, r_y, x_lim, y_lim, density, black, intensivity, point_x, point_y, radius_a, radius_b, memory);
                stack.AddLayer(std::move(p));
            }
            else if (parsed[0] == "Sin") {
                if (parsed.size() < 13) {
                    std::cout << "Warning : " << line << "  //Doesn't satisfy format\n";
                    continue;
                }
                int start = std::stoi(parsed[8]);
                int shift = std::stoi(parsed[9]);
                int amplitude = std::stoi(parsed[10]);
                float period = std::stof(parsed[11]);
                int horizontal = parsed[12] ==  "0" ? false : true;
                bool memory = false;
                if (parsed.size() > 13) {
                    memory = parsed[13] ==  "0" ? false : true;
                }
                auto p = std::make_unique<SinPrinter>(r_x, r_y, x_lim, y_lim, density, black, intensivity, start, shift, amplitude, period, horizontal, memory);
                stack.AddLayer(std::move(p));
            }
            else if (parsed[0] == "Blur") {
                if (parsed.size() < 2) {
                    std::cout << "Warning : " << line << "  //Doesn't satisfy format\n";
                    continue;
                }
                intensivity = std::stof(parsed[1]);
                auto p = std::make_unique<BlurPrinter>(intensivity);
                stack.AddLayer(std::move(p));

            }        
        } else {
            if (fs::is_directory(image_path)) {
                for (const auto& entry : fs::directory_iterator(image_path)) {
                    ProcessFile(entry.path(), dir_path, stack, stack_name);
                }
            } else if (fs::is_regular_file(image_path)) {
                ProcessFile(image_path, dir_path, stack, stack_name);
            } else {
                std::cout << "Wrong Input\n";
                return 2;
            }
            stack.Clear();
        }
    }
}

