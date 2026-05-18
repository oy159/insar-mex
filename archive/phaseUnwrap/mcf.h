#ifndef MCF_H
#define MCF_H

#include "../utils/utils.h"
#include <vector>
#include <memory>
#include <cstddef>

namespace mcf {

struct DebugData {
    std::unique_ptr<utils::MatrixD> reliability;
    std::unique_ptr<utils::MatrixD> residues;
};

class Method {
public:
    bool debugMode = false;
    DebugData debugData;

    void setDebugMode(bool debug) { debugMode = debug; }

    // MCF phase unwrapping (Costantini 1998)
    std::unique_ptr<utils::MatrixD> unwrap(const utils::MatrixD& wrappedMap);
};

}

#endif // MCF_H
