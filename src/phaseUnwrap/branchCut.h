#ifndef RADAR_ALGORITHM_BRANCHCUT_H
#define RADAR_ALGORITHM_BRANCHCUT_H

#include "../utils/utils.h"
#include <memory>

namespace branchCut {


    class Method {
    private:

        struct ResidueNode {
            int r, c;
            int charge;
            bool balanced;
        };

        utils::Matrix<int> _residues;    // 残差点矩阵 (+1/-1/0)
        utils::MatrixB _branchCuts;      // 枝切线标记矩阵 (0/1)
        int _num_residues = 0;



        static double wrap2pi(double phase);

        void locateResidues(const utils::MatrixD& phaseMap);
        void placeBranchCuts();
        void unwrapAroundCuts(const utils::MatrixD& phaseMap, utils::MatrixD& result);

    public:
        Method() = default;
        ~Method() = default;

        /**
         * @brief 执行完整的枝切法相位解缠流程 (对应 branchsCutPhaseUnwrap.m)
         *
         * @param wrappedPhase 输入的折叠相位图
         * @return std::unique_ptr<utils::MatrixD> 解缠后的相位图
         */
        std::unique_ptr<utils::MatrixD> unwrap(const utils::MatrixD& wrappedPhase);

        /**
         * @brief 获取残差点矩阵 (用于调试或可视化)
         * @return 返回残差点矩阵的副本
         */
        std::unique_ptr<utils::Matrix<int>> getResidues();

        /**
         * @brief 获取枝切线矩阵 (用于调试或可视化)
         * @return 返回枝切线矩阵的副本
         */
        std::unique_ptr<utils::MatrixB> getBranchCuts();

        int getNumResidues();
    };

}

#endif //RADAR_ALGORITHM_BRANCHCUT_H