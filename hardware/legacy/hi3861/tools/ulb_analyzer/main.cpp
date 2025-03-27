#include <iostream>
#include <fstream>
#include <array>
#include <cmath>
#include <format>
#include <charconv>
#include <string_view>
#include <algorithm>
#include <spdlog/spdlog.h>

#define R05D_LEAD_PULSE   4400
#define R05D_LEAD_SPACE   4400
#define R05D_STOP_PULSE    540
#define R05D_STOP_SPACE   5220
#define R05D_BIT_0_PULSE   540
#define R05D_BIT_0_SPACE   540
#define R05D_BIT_1_PULSE   540
#define R05D_BIT_1_SPACE  1620

constexpr int DELTA_MAX = 500;
constexpr int MIN_CNT = 100;

int getClosestValue(int input) {
    constexpr std::array<int, 8> values{
        R05D_LEAD_PULSE,
        R05D_LEAD_SPACE,
        R05D_STOP_PULSE,
        R05D_STOP_SPACE,
        R05D_BIT_0_PULSE,
        R05D_BIT_0_SPACE,
        R05D_BIT_1_PULSE,
        R05D_BIT_1_SPACE
    };
    auto comp = [input](int a, int b) {
        return std::abs(a - input) < std::abs(b - input);
    };
    return *std::min_element(values.begin(), values.end(), comp);
}

bool decode(char byte) {
    return byte & 0b00000001;
}

int cast(int cnt) {
    return static_cast<int>(std::round(cnt * 0.0416));
}

int main(int argc, char** argv) {
    if (argc < 3) {
        spdlog::error("Usage: {} <input file> <output file> [seek offset]", argv[0]);
        return 1;
    }

    int seek_offset = 232;
    if (argc >= 4) {
        std::string_view offsetStr(argv[3]);
        int temp = 0;
        auto [ptr, ec] = std::from_chars(offsetStr.data(), offsetStr.data() + offsetStr.size(), temp);
        if (ec == std::errc{}) {
            seek_offset = temp;
            spdlog::info("Using provided seek offset: {}", seek_offset);
        } else {
            spdlog::warn("Invalid seek offset provided, using default: {}", seek_offset);
        }
    } else {
        spdlog::info("Using default seek offset: {}", seek_offset);
    }

    spdlog::info("Opening input file: {}", argv[1]);
    std::ifstream ifs(argv[1], std::ios::binary);
    if (!ifs) {
        spdlog::error("Failed to open input file: {}", argv[1]);
        return 1;
    }
    spdlog::info("Opening output file: {}", argv[2]);
    std::ofstream ofs(argv[2]);
    if (!ofs) {
        spdlog::error("Failed to open output file: {}", argv[2]);
        return 1;
    }

    ifs.seekg(seek_offset);
    spdlog::info("Start processing file at offset {}", seek_offset);

    char byte{};
    int cnt = 1;
    bool last_b = true;
    bool b = true;
    int cast_us = 0, closest_us = 0, delta = 0;
    while (ifs.get(byte)) {
        b = decode(byte);
        if (b != last_b) {
            spdlog::debug("Level transition from {} to {} after {} bytes", (last_b ? "true" : "false"), (b ? "true" : "false"), cnt);
            if (cnt <= MIN_CNT) {
                ++cnt;
            } else {
                cast_us = cast(cnt);
                closest_us = getClosestValue(cast_us);
                delta = cast_us - closest_us;
                if (delta > DELTA_MAX) {
                    spdlog::warn("Delta too big! cast_us = {}, delta = {}", cast_us, delta);
                    ofs << std::format("//delta too big! cast_us = {}, delta = {}\n", cast_us, delta);
                } else {
                    //spdlog::info("Writing PWM command: littleswan_pwm_{} with delay {}", (b ? "enable" : "disable"), closest_us);
                    ofs << std::format("littleswan_pwm_{}();\nhi_udelay({});//cast_us = {}, delta = {}\n",
                                       (b ? "enable" : "disable"), closest_us, cast_us, delta);
                }
                cnt = 1;
            }
        } else {
            ++cnt;
        }
        last_b = b;
    }
    ofs << "littleswan_pwm_disable();\n";
    spdlog::info("Finished processing. Output written to {}", argv[2]);
    return 0;
}
