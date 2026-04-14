#include "branchCut.h"

#include <algorithm>
#include <limits>
#include <functional>
#include <cmath>

namespace {
    struct Point {
        int r, c;
        int idx; // 在 activeResidues 中的索引
    };

    struct AABB {
        double r, c, h, w; // r, c 为左上角坐标; h 为高(行方向), w 为宽(列方向)

        // 计算点到矩形区域的最短欧几里得距离平方
        double distSq(int pr, int pc) const {
            double dr = 0;
            if (pr < r) dr = r - pr;
            else if (pr > r + h) dr = pr - (r + h);

            double dc = 0;
            if (pc < c) dc = c - pc;
            else if (pc > c + w) dc = pc - (c + w);

            return dr * dr + dc * dc;
        }

        bool contains(int pr, int pc) const {
            return pr >= r && pr <= r + h && pc >= c && pc <= c + w;
        }
    };

    class QuadTree {
        struct Node {
            AABB bounds;
            std::vector<Point> points;
            std::unique_ptr<Node> children[4]; // UL, UR, LL, LR
            bool isLeaf = true;

            explicit Node(AABB b) : bounds(b) {}
        };

        std::unique_ptr<Node> root;
        static const int CAPACITY = 8; // 叶子节点最大容量

        void split(Node* node) {
            double subH = node->bounds.h / 2.0;
            double subW = node->bounds.w / 2.0;
            double r = node->bounds.r;
            double c = node->bounds.c;

            node->children[0] = std::make_unique<Node>(AABB{r, c, subH, subW});             // UL
            node->children[1] = std::make_unique<Node>(AABB{r, c + subW, subH, subW});      // UR
            node->children[2] = std::make_unique<Node>(AABB{r + subH, c, subH, subW});      // LL
            node->children[3] = std::make_unique<Node>(AABB{r + subH, c + subW, subH, subW}); // LR
            node->isLeaf = false;

            for (const auto& p : node->points) {
                insertRecursive(node, p);
            }
            node->points.clear();
        }

        void insertRecursive(Node* node, const Point& p) {
            if (node->isLeaf) {
                node->points.push_back(p);
                if (node->points.size() > CAPACITY) {
                    split(node);
                }
                return;
            }

            // 找到包含点的子象限
            for (int i = 0; i < 4; ++i) {
                if (node->children[i]->bounds.contains(p.r, p.c)) {
                    insertRecursive(node->children[i].get(), p);
                    return;
                }
            }
        }

        void searchRecursive(Node* node, int targetR, int targetC,
                             int& bestIdx, double& minDstSq,
                             const std::function<bool(int)>& isValid) const {
            // 剪枝：如果当前节点包使得最近距离都大于当前最小距离，则跳过
            double dBox = node->bounds.distSq(targetR, targetC);
            if (dBox >= minDstSq) return;

            if (node->isLeaf) {
                for (const auto& p : node->points) {
                    if (!isValid(p.idx)) continue; // 跳过已平衡的点

                    double dr = p.r - targetR;
                    double dc = p.c - targetC;
                    double d2 = dr * dr + dc * dc;
                    if (d2 < minDstSq) {
                        minDstSq = d2;
                        bestIdx = p.idx;
                    }
                }
            } else {
                // 优先搜索距离目标点更近的子节点
                struct ChildDist {
                    int index;
                    double d;
                };
                ChildDist childrenDist[4];
                for (int i = 0; i < 4; ++i) {
                    childrenDist[i] = {i, node->children[i]->bounds.distSq(targetR, targetC)};
                }

                // 简单的排序，最近的先访问
                std::sort(std::begin(childrenDist), std::end(childrenDist),
                          [](const ChildDist& a, const ChildDist& b) { return a.d < b.d; });

                for (const auto& cd : childrenDist) {
                    searchRecursive(node->children[cd.index].get(), targetR, targetC, bestIdx, minDstSq, isValid);
                }
            }
        }

    public:
        QuadTree(int rows, int cols) {
            root = std::make_unique<Node>(AABB{0, 0, (double)rows, (double)cols});
        }

        void insert(int r, int c, int idx) {
            insertRecursive(root.get(), Point{r, c, idx});
        }

        int findNearest(int r, int c, double& outDist, const std::function<bool(int)>& isValid) {
            int bestIdx = -1;
            double minDstSq = std::numeric_limits<double>::max();
            searchRecursive(root.get(), r, c, bestIdx, minDstSq, isValid);
            outDist = std::sqrt(minDstSq);
            return bestIdx;
        }
    };
}

double branchCut::Method::wrap2pi(double phase) {
    return atan2(sin(phase), cos(phase));
}

void branchCut::Method::locateResidues(const utils::MatrixD &phaseMap) {
    int rows = phaseMap.rows();
    int cols = phaseMap.cols();
    _residues = utils::Matrix<int>(rows, cols, 0);
    _num_residues = 0;
    for (int i = 0; i < rows - 1; ++i) {
        for (int j = 0; j < cols - 1; ++j) {
            double p1 = phaseMap(i, j);
            double p2 = phaseMap(i, j + 1);
            double p3 = phaseMap(i + 1, j + 1);
            double p4 = phaseMap(i + 1, j);

            double d1 = wrap2pi(p2 - p1);
            double d2 = wrap2pi(p3 - p2);
            double d3 = wrap2pi(p4 - p3);
            double d4 = wrap2pi(p1 - p4);
            double sum = d1 + d2 + d3 + d4;

            int charge = static_cast<int>(std::round(sum / (2.0 * utils::PI)));

            if (charge != 0) {
                _residues(i, j) = charge;
                _num_residues++;
            }
        }
    }
}

void branchCut::Method::placeBranchCuts() {
    int rows = _residues.rows();
    int cols = _residues.cols();

    _branchCuts = utils::MatrixB(rows, cols, 0);

   std::vector<ResidueNode> activeResidues;
    activeResidues.reserve(_num_residues);

    // 2. 初始化四叉树
    QuadTree qTree(rows, cols);

    int idxCounter = 0;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (_residues(i, j) != 0) {
                activeResidues.push_back({i, j, _residues(i, j), false});
                qTree.insert(i, j, idxCounter++);
            }
        }
    }

    // Lambda: 画线工具 (Bresenham)
    auto drawLine = [&](int r0, int c0, int r1, int c1) {
        int dr = std::abs(r1 - r0);
        int dc = std::abs(c1 - c0);
        int sr = (r0 < r1) ? 1 : -1;
        int sc = (c0 < c1) ? 1 : -1;
        int err = (dr > dc ? dr : -dc) / 2;

        while (true) {
            if (r0 >= 0 && r0 < rows && c0 >= 0 && c0 < cols) {
                _branchCuts(r0, c0) = 1;
            }
            if (r0 == r1 && c0 == c1) break;
            int e2 = err;
            if (e2 > -dr) { err -= dc; r0 += sr; }
            if (e2 < dc) { err += dr; c0 += sc; }
        }
    };

    // 3. 执行 Goldstein 算法
    for (size_t i = 0; i < activeResidues.size(); ++i) {
        if (activeResidues[i].balanced) continue;

        int currentR = activeResidues[i].r;
        int currentC = activeResidues[i].c;
        int totalCharge = activeResidues[i].charge;
        activeResidues[i].balanced = true;

        while (totalCharge != 0) {
                double minResidueDist = std::numeric_limits<double>::max();
            auto isValid = [&](int candidateIdx) {
                // 注意：在递归搜索时，目标点就是 recent point，它在上一轮或循环开始时已被标记为 balanced。
                // 因此只要 check !balanced 即可自然排除自己和已处理的点。
                return !activeResidues[candidateIdx].balanced;
            };

            int bestIdx = qTree.findNearest(currentR, currentC, minResidueDist, isValid);

            // B. 检查到边界的距离
            double distLeft = currentC;
            double distRight = cols - 1 - currentC;
            double distTop = currentR;
            double distBottom = rows - 1 - currentR;
            double minBorderDist = std::min({distLeft, distRight, distTop, distBottom});

            // C. 决策：连接边界 vs 连接下一个残差点
            if (bestIdx == -1 || minBorderDist < minResidueDist) {
                // 边界更近，或者找不到其他残差点 -> 向边界放电
                int targetR = currentR;
                int targetC = currentC;

                if (std::abs(minBorderDist - distLeft) < 1e-5) targetC = 0;
                else if (std::abs(minBorderDist - distRight) < 1e-5) targetC = cols - 1;
                else if (std::abs(minBorderDist - distTop) < 1e-5) targetR = 0;
                else targetR = rows - 1;

                drawLine(currentR, currentC, targetR, targetC);
                totalCharge = 0;
            } else {
                // 残差点更近 -> 合并
                ResidueNode& next = activeResidues[bestIdx];
                drawLine(currentR, currentC, next.r, next.c);

                totalCharge += next.charge;
                next.balanced = true; // 标记该点已处理

                // 移动 Pivot
                currentR = next.r;
                currentC = next.c;
            }
        }
    }
}

void branchCut::Method::unwrapAroundCuts(const utils::MatrixD &phaseMap, utils::MatrixD &result) {
    int rows = phaseMap.rows();
    int cols = phaseMap.cols();
    
    // 初始化解缠结果矩阵和标志位矩阵
    // result = phaseMap; // 不需要全拷贝，只需要在访问时赋值
    utils::MatrixB unwrappedFlags(rows, cols, 0); // 0: 未解缠, 1: 已解缠

    // Lambda: Itoh 1D 解缠辅助函数
    auto itohUnwrapNeighbor = [](double currentVal, double neighborWrapped) {
        double diff = neighborWrapped - currentVal;
        return currentVal + atan2(sin(diff), cos(diff)); // wrap diff to [-pi, pi]
    };

    // BFS 队列
    std::vector<std::pair<int, int>> queue;
    queue.reserve(rows * cols); // 预分配内存以优化性能

    // 1. 处理由枝切线分割的主要连通区域 (相当于 bwconncomp + 区域内解缠)
    for (int r = 0; r < rows; ++r) {
        for (int c = 0; c < cols; ++c) {
            // 如果该点不是枝切线点，且尚未解缠，则为一个新区域的起点
            if (_branchCuts(r, c) == 0 && unwrappedFlags(r, c) == 0) {
                // 初始化区域起点
                queue.clear();
                queue.push_back({r, c});
                unwrappedFlags(r, c) = 1;
                result(r, c) = phaseMap(r, c);

                size_t head = 0;
                while (head < queue.size()) {
                    auto [currR, currC] = queue[head++];
                    double currPhase = result(currR, currC);

                    // 检查 4 邻域 (上、下、左、右)
                    const int dr[] = {-1, 1, 0, 0};
                    const int dc[] = {0, 0, -1, 1};

                    for (int k = 0; k < 4; ++k) {
                        int nr = currR + dr[k];
                        int nc = currC + dc[k];

                        // 边界检查
                        if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                            // 如果邻域点不是枝切线且未解缠
                            if (_branchCuts(nr, nc) == 0 && unwrappedFlags(nr, nc) == 0) {
                                result(nr, nc) = itohUnwrapNeighbor(currPhase, phaseMap(nr, nc));
                                unwrappedFlags(nr, nc) = 1;
                                queue.push_back({nr, nc});
                            }
                        }
                    }
                }
            }
        }
    }

    // 2. 解缠枝切线上的像素 (Post-processing)
    // 枝切线像素可以通过其邻居（非枝切线且已解缠）来解缠
    // 根据 UnwrapAroundCuts.m，通常做两遍扫描以覆盖可能的复杂情况
    for (int iter = 0; iter < 2; ++iter) {
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                if (unwrappedFlags(r, c) == 0) {
                    // 按优先级尝试寻找即邻接的已解缠点: 左, 上, 右, 下
                    // 一旦找到一个有效邻居，就进行解缠并标记
                    
                    if (c > 0 && unwrappedFlags(r, c - 1)) { // 左
                        result(r, c) = itohUnwrapNeighbor(result(r, c - 1), phaseMap(r, c));
                        unwrappedFlags(r, c) = 1;
                    } 
                    else if (r > 0 && unwrappedFlags(r - 1, c)) { // 上
                        result(r, c) = itohUnwrapNeighbor(result(r - 1, c), phaseMap(r, c));
                        unwrappedFlags(r, c) = 1;
                    }
                    else if (c < cols - 1 && unwrappedFlags(r, c + 1)) { // 右
                        result(r, c) = itohUnwrapNeighbor(result(r, c + 1), phaseMap(r, c));
                        unwrappedFlags(r, c) = 1;
                    }
                    else if (r < rows - 1 && unwrappedFlags(r + 1, c)) { // 下
                        result(r, c) = itohUnwrapNeighbor(result(r + 1, c), phaseMap(r, c));
                        unwrappedFlags(r, c) = 1;
                    }
                }
            }
        }
    }
}

std::unique_ptr<utils::MatrixD> branchCut::Method::unwrap(const utils::MatrixD &wrappedPhase) {
    locateResidues(wrappedPhase);
    utils::Log("Located %d residues.", _num_residues);
    placeBranchCuts();
    utils::Log("Place branch cuts finished.");

    auto result = std::make_unique<utils::MatrixD>(wrappedPhase.rows(), wrappedPhase.cols());
    unwrapAroundCuts(wrappedPhase, *result);

    return result; 
}

std::unique_ptr<utils::Matrix<int>> branchCut::Method::getResidues() {
    return std::make_unique<utils::Matrix<int>>(_residues);
}

std::unique_ptr<utils::MatrixB> branchCut::Method::getBranchCuts() {
    return std::make_unique<utils::MatrixB>(_branchCuts);
}

int branchCut::Method::getNumResidues() {
    return _num_residues;
}
