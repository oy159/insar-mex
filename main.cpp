#include "src/phaseUnwrap/snaphu.h"
#include "src/utils/utils.h"
#include <vector>
#include <chrono>
#include <random>
#include <cmath>

using namespace utils;

int main() {
    Log("========================================");
    Log("SNAPHU Phase Unwrapping Test (Windows)");
    Log("========================================");

    const int M = 2000;
    const int N = 2000;
    Log("Image size: %d x %d", M, N);

    /* generate a synthetic wrapped phase: ramp + noise */
    MatrixD wrappedPhase(M, N);
    std::mt19937 gen(42);
    std::uniform_real_distribution<> noise(-0.1, 0.1);

    for (int i = 0; i < M; ++i) {
        for (int j = 0; j < N; ++j) {
            double phi = 0.1 * (i + j) + noise(gen);
            while (phi > PI) phi -= 2 * PI;
            while (phi < -PI) phi += 2 * PI;
            wrappedPhase(i, j) = phi;
        }
    }

    /* magnitude = all ones */
    MatrixD magnitude(M, N);
    for (int i = 0; i < M; ++i)
        for (int j = 0; j < N; j++)
            magnitude(i, j) = 1.0;

    /* test MST initialization first (SNAPHU default) */
    Log("\n--- SNAPHU MST init, TOPO mode ---");
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = snaphu::unwrap(wrappedPhase, snaphu::MST, snaphu::TOPO, &magnitude);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> dur = end - start;

        if (result.rows() == 0) {
            Log("FAILED!");
        } else {
            Log("Done in %.3f s", dur.count() / 1000.0);
            Log("  (0,0)     = %.4f  expect ~0.0",   result(0, 0));
            Log("  (100,100) = %.4f  expect ~20.0",  result(100, 100));
            Log("  (199,199) = %.4f  expect ~39.8",  result(199, 199));
        }
    }

    /* test MCF initialization */
    Log("\n--- SNAPHU MCF init, TOPO mode ---");
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = snaphu::unwrap(wrappedPhase, snaphu::MCF, snaphu::TOPO, &magnitude);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> dur = end - start;

        if (result.rows() == 0) {
            Log("FAILED!");
        } else {
            Log("Done in %.3f s", dur.count() / 1000.0);
            Log("  (0,0)     = %.4f  expect ~0.0",   result(0, 0));
            Log("  (100,100) = %.4f  expect ~20.0",  result(100, 100));
            Log("  (199,199) = %.4f  expect ~39.8",  result(199, 199));

            /* check monotonicity along diagonal */
            bool monotonic = true;
            for (int k = 1; k < std::min(M, N); ++k) {
                if (result(k, k) < result(k-1, k-1) - 0.5) {
                    monotonic = false;
                    break;
                }
            }
            Log("  Monotonic along diagonal: %s", monotonic ? "YES" : "NO");
        }
    }

    /* test MST initialization */
    Log("\n--- SNAPHU MST init, TOPO mode ---");
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = snaphu::unwrap(wrappedPhase, snaphu::MST, snaphu::TOPO, &magnitude);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> dur = end - start;

        if (result.rows() == 0) {
            Log("FAILED!");
        } else {
            Log("Done in %.3f s", dur.count() / 1000.0);
            Log("  (0,0)     = %.4f  expect ~0.0",   result(0, 0));
            Log("  (100,100) = %.4f  expect ~20.0",  result(100, 100));
            Log("  (199,199) = %.4f  expect ~39.8",  result(199, 199));
        }
    }

    /* test SMOOTH mode with MCF */
    Log("\n--- SNAPHU MCF init, SMOOTH mode ---");
    {
        auto start = std::chrono::high_resolution_clock::now();
        auto result = snaphu::unwrap(wrappedPhase, snaphu::MCF, snaphu::SMOOTH, &magnitude);
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> dur = end - start;

        if (result.rows() == 0) {
            Log("FAILED!");
        } else {
            Log("Done in %.3f s", dur.count() / 1000.0);
            Log("  (0,0)     = %.4f", result(0, 0));
            Log("  (100,100) = %.4f", result(100, 100));
            Log("  (199,199) = %.4f", result(199, 199));
        }
    }

    Log("\n========================================");
    Log("All tests complete.");
    Log("========================================");
    return 0;
}
