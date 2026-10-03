#pragma once
#include <iostream>
#include <string>

namespace SectorG {

    class Log {
    public:
        static void Info(const std::string& msg) {
            std::cout << "[INFO]  " << msg << std::endl;
        }
        static void Warn(const std::string& msg) {
            std::cout << "[WARN]  " << msg << std::endl;
        }
        static void Error(const std::string& msg) {
            std::cerr << "[ERROR] " << msg << std::endl;
        }
    };

}