# 解缠方法复杂度分析

本文基于仓库中当前实现分析以下三种相位解缠方法的复杂度：

- MCF（最小费用流，`src/phaseUnwrap/mcf.cpp`）
- Branch Cut（枝切法，`src/phaseUnwrap/branchCut.cpp`）
- Quality Map Guided（质量图引导，`src/phaseUnwrap/qualityMapGuideMethod.cpp`）

---

## 记号约定

- 设相位图尺寸为 `M × N`
- 设像素总数 `P = M*N`
- 设残差点数量 `R`
- 设 MCF 中总供需量（或总发送流量上界）为 `F`（代码中迭代上界与 `total_abs_R` 同阶）

---

## 1) MCF（mcf::Method::unwrap）

### 主要步骤
1. 梯度与残差计算：线性扫描网格  
2. 构建对偶网络：节点/边数量均与网格规模同阶  
3. 逐次最短路求最小费用流（Dijkstra + 势能）  
4. 流修正与积分重建

### 时间复杂度
- 预处理与后处理：`O(P)`
- 每次最短路：`O(E log V)`，其中 `V = O(P)`，`E = O(P)`，所以为 `O(P log P)`
- 迭代次数：`O(F)`

**总时间复杂度：`O(F * P log P + P)`，主导项为 `O(F * P log P)`**  
若极端情况下 `F = O(P)`，则最坏可达 **`O(P^2 log P)`**。

### 空间复杂度
需要存储梯度、残差、网络边、流、势能、邻接表等，均为网格同阶：  
**`O(P)`**

---

## 2) Branch Cut（branchCut::Method::unwrap）

### 主要步骤
1. `locateResidues`：扫描 2x2 回路检测残差  
2. `placeBranchCuts`：  
   - 残差点入四叉树  
   - 迭代寻找最近未平衡残差或连接边界，并画枝切线  
3. `unwrapAroundCuts`：在非枝切区域 BFS 解缠，再对枝切线像素做两遍补处理

### 时间复杂度
- 残差定位：`O(P)`
- 四叉树构建（R 个残差）：平均 `O(R log R)`
- 最近邻查询与配平：
  - 平均可看作 `O(R log R)` 量级
  - 最坏（退化/剪枝效果差）可达 `O(R^2)`
- BFS 解缠与补处理：`O(P)`

**综合：平均约 `O(P + R log R)`；最坏约 `O(P + R^2)`**  
（画线开销与线段总长度相关，通常不超过上述主项量级判断）

### 空间复杂度
- 残差矩阵、枝切矩阵、解缠标记、队列、四叉树等：  
**`O(P + R)`**（通常可记为 `O(P)`）

---

## 3) Quality Map Guided（qualityMapGuideMethod::Method::unwrap）

### 主要步骤
1. `setMask`：阈值掩膜生成  
2. `getBestPoint`：寻找起始高质量点  
3. 优先队列驱动的区域扩展解缠（邻点入队、出队、访问标记）

### 时间复杂度
- 掩膜与起点搜索：`O(P)`
- 每个有效像素最多入队/出队有限次，优先队列操作为 `log P`

**总时间复杂度：`O(P log P)`**

### 空间复杂度
- 掩膜、flag、结果图、visited 集合、优先队列：  
**`O(P)`**

---

## 对比结论（实现视角）

- **MCF**：理论最重，瓶颈在最小费用流迭代（`F * P log P`）；精度/全局一致性强，但计算成本高。  
- **Branch Cut**：复杂度受残差分布影响明显；残差较少时较快，最坏可退化到 `R^2`。  
- **Quality Map Guided**：实现上最稳定，典型为 `P log P`，工程上常更易获得较好速度。

如果你需要，我可以再补一版“在常见 InSAR 场景（低噪/高噪、密集残差）下的复杂度与效果选型建议”。
