#include "src/phaseUnwrap/qualityMapGuideMethod.h"
#include "src/phaseUnwrap/mcf.h"
#include <iostream>
#include <fstream>
#include "src/utils/utils.h"
#include <vector>
#include <chrono>
#include <random>
#include <cmath>

using namespace utils;
using namespace qualityMapGuideMethod;

int main() {
    Log("Starting MCF phase unwrapping performance test...");

    const int M = 500;
    const int N = 500;
    Log("Image size: %d x %d", M, N);

    // Generate random synthetic wrapped phase
    MatrixD wrappedPhase(M, N);
    std::mt19937 gen(42); // fixed seed for reproducibility
    std::uniform_real_distribution<> dis(-PI, PI);

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            wrappedPhase(i, j) = dis(gen);
        }
    }

    mcf::Method mcf_method;
    mcf_method.setDebugMode(false); // Disable debug mode for pure performance

    Log("Running MCF unwrap...");
    auto start = std::chrono::high_resolution_clock::now();

    auto unwrapped = mcf_method.unwrap(wrappedPhase);

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    Log("MCF unwrap finished in %.3f seconds.", duration.count() / 1000.0);

    return 0;
}