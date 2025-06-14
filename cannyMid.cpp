#include <cstdint>
#include <cmath>
#include <algorithm>

extern "C" {

// Gaussian Blur 3x3
void GaussianBlur3x3(const uint8_t* input, int width, int height, uint8_t* output) {
    int kernel[3][3] = {
        {1, 2, 1},
        {2, 4, 2},
        {1, 2, 1}
    };
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int sum = 0;
            for (int ky = -1; ky <= 1; ++ky)
                for (int kx = -1; kx <= 1; ++kx)
                    sum += input[(y + ky) * width + (x + kx)] * kernel[ky + 1][kx + 1];
            output[y * width + x] = sum / 16; // kernel weight sum = 16
        }
    }
}

// Quantize gradient direction to 4 angles (0°,45°,90°,135°)
int QuantizeDirection(float angle) {
    angle = angle * 180.0f / 3.14159f;
    if (angle < 0) angle += 180;
    if ((angle >= 0 && angle < 22.5) || (angle >= 157.5 && angle <= 180))
        return 0;
    else if (angle >= 22.5 && angle < 67.5)
        return 45;
    else if (angle >= 67.5 && angle < 112.5)
        return 90;
    else
        return 135;
}

// Mid-tier fast canny
void FastCannyEdge(const uint8_t* gray, int width, int height, uint8_t* outMask, uint8_t threshold = 50) {
    // Buffers
    std::vector<uint8_t> blurred(width * height, 0);
    std::vector<float> mag(width * height, 0);
    std::vector<float> angle(width * height, 0);

    GaussianBlur3x3(gray, width, height, blurred.data());

    auto at = [&](int y, int x) -> uint8_t { return blurred[y * width + x]; };
    auto set = [&](int y, int x, uint8_t val) { outMask[y * width + x] = val; };

    // Sobel Gradient
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            int gx = -at(y - 1, x - 1) + at(y - 1, x + 1)
                     - 2 * at(y, x - 1) + 2 * at(y, x + 1)
                     - at(y + 1, x - 1) + at(y + 1, x + 1);
            int gy = -at(y - 1, x - 1) - 2 * at(y - 1, x) - at(y - 1, x + 1)
                     + at(y + 1, x - 1) + 2 * at(y + 1, x) + at(y + 1, x + 1);

            mag[y * width + x] = std::sqrt(gx * gx + gy * gy);
            angle[y * width + x] = std::atan2(gy, gx);
        }
    }

    // Non-Maximum Suppression (8-dir thinning)
    for (int y = 1; y < height - 1; ++y) {
        for (int x = 1; x < width - 1; ++x) {
            float m = mag[y * width + x];
            if (m < threshold) {
                set(y, x, 0);
                continue;
            }
            int dir = QuantizeDirection(angle[y * width + x]);
            bool keep = false;
            if (dir == 0)
                keep = (m >= mag[y * width + (x - 1)] && m >= mag[y * width + (x + 1)]);
            else if (dir == 45)
                keep = (m >= mag[(y - 1) * width + (x + 1)] && m >= mag[(y + 1) * width + (x - 1)]);
            else if (dir == 90)
                keep = (m >= mag[(y - 1) * width + x] && m >= mag[(y + 1) * width + x]);
            else // 135
                keep = (m >= mag[(y - 1) * width + (x - 1)] && m >= mag[(y + 1) * width + (x + 1)]);

            set(y, x, keep ? 255 : 0);
        }
    }
}

} // extern "C"


// ---------------- fast_canny_ui.cpp ---------------- #include <cstdint> #include <vector> #include <cmath> #include <cstring> #include <algorithm> #include <opencv2/imgproc.hpp> #include <opencv2/core.hpp> #include <opencv2/imgcodecs.hpp>

extern "C" {

struct Rect { int x, y, width, height; };

void FastCannyEdge(const uint8_t* gray, int width, int height, uint8_t* outMask, uint8_t threshold = 50) { auto at = [&](int y, int x) -> uint8_t { return gray[y * width + x]; }; auto set = [&](int y, int x, uint8_t val) { outMask[y * width + x] = val; };

std::vector<uint8_t> mag(width * height, 0);
std::vector<uint8_t> dir(width * height, 0);

for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
        int gx = 0, gy = 0;
        gx += -1 * at(y - 1, x - 1) + 1 * at(y - 1, x + 1);
        gx += -2 * at(y, x - 1)     + 2 * at(y, x + 1);
        gx += -1 * at(y + 1, x - 1) + 1 * at(y + 1, x + 1);

        gy += -1 * at(y - 1, x - 1) + -2 * at(y - 1, x) + -1 * at(y - 1, x + 1);
        gy +=  1 * at(y + 1, x - 1) +  2 * at(y + 1, x) +  1 * at(y + 1, x + 1);

        int magnitude = std::min(255, std::abs(gx) + std::abs(gy));
        mag[y * width + x] = magnitude;
        dir[y * width + x] = (std::abs(gx) > std::abs(gy)) ? 0 : 1;
    }
}

for (int y = 1; y < height - 1; ++y) {
    for (int x = 1; x < width - 1; ++x) {
        uint8_t m = mag[y * width + x];
        if (m < threshold) continue;
        bool keep = false;
        if (dir[y * width + x] == 0)
            keep = m >= mag[y * width + (x - 1)] && m >= mag[y * width + (x + 1)];
        else
            keep = m >= mag[(y - 1) * width + x] && m >= mag[(y + 1) * width + x];
        if (keep) set(y, x, 255);
    }
}

}

int ExtractRectsFromEdgeMask(const uint8_t* edgeMask, int width, int height, Rect* outRects, int maxRects, float minArea = 100.0f, float minAspect = 0.3f, float maxAspect = 3.0f, float minSolidity = 0.5f) { cv::Mat mask(height, width, CV_8UC1, (void*)edgeMask); std::vector<std::vectorcv::Point> contours; cv::findContours(mask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

int count = 0;
for (const auto& contour : contours) {
    float area = cv::contourArea(contour);
    if (area < minArea) continue;

    cv::Rect bb = cv::boundingRect(contour);
    float aspect = (float)bb.width / bb.height;
    if (aspect < minAspect || aspect > maxAspect) continue;

    float solidity = area / (bb.width * bb.height);
    if (solidity < minSolidity) continue;

    if (count < maxRects) {
        outRects[count++] = { bb.x, bb.y, bb.width, bb.height };
    }
}
return count;

}

} // extern "C"

