# 数据说明与推荐分割参数

本目录包含项目演示点云、合成压力测试点云和真实公开点云样例。所有文件均为当前程序可直接读取的 PLY 点云。除 `scene_segmented.ply` 这类示例结果外，输入点云使用统一灰色，颜色由程序分割后生成。不同文件的点密度、噪声和几何结构不同，建议按下表设置分割参数。

推荐参数按“彩色覆盖率较高、段数不过度碎片化、运行时间可接受”的标准选取。彩色覆盖率表示最终导出层中非灰色点的比例；灰色点通常是被过滤的小段或噪声点。

## 数据与参数

| 点云文件 | 点数 | k | levels | 最粗夹角 | 最细夹角 | 曲率阈值 | 最小段点数 | 最终段数 | 覆盖率 | 说明 |
|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---|
| `scene.ply` | 9,000 | 16 | 3 | 30 | 10 | 0.08 | 30 | 5 | 96.7% | 项目原始演示场景 |
| `scene_segmented.ply` | 9,000 | - | - | - | - | - | - | - | - | `scene.ply` 的示例分割结果，仅用于查看效果 |
| `stress_close_parallel_28000.ply` | 28,000 | 16 | 1 | 45 | 45 | 0.15 | 30 | 8 | 85.1% | 近距离双层平面，适合观察粗分割稳定性 |
| `stress_complex_35200.ply` | 35,200 | 20 | 3 | 32 | 12 | 0.10 | 80 | 7 | 92.9% | 地面、墙面、球、圆柱、圆环混合 |
| `stress_sparse_noisy_14800.ply` | 14,800 | 24 | 2 | 40 | 20 | 0.18 | 80 | 6 | 81.5% | 稀疏、强噪声、离群点多 |
| `stress_large_100000.ply` | 100,000 | 24 | 3 | 32 | 14 | 0.10 | 150 | 5 | 93.1% | 大规模性能压力测试 |
| `real/stanford_bunny_res3.ply` | 1,889 | 12 | 2 | 35 | 18 | 0.12 | 10 | 5 | 87.8% | 快速调试 |
| `real/stanford_bunny_res2.ply` | 8,171 | 16 | 2 | 35 | 18 | 0.12 | 20 | 2 | 98.3% | 轻量真实模型 |
| `real/stanford_bunny.ply` | 35,947 | 16 | 3 | 28 | 8 | 0.08 | 30 | 3 | 96.0% | Bunny 完整点云，曲面 patch 分割 |
| `real/stanford_dragon_res3.ply` | 22,998 | 18 | 3 | 35 | 16 | 0.12 | 40 | 3 | 94.5% | Dragon 中等点数 |
| `real/stanford_dragon_res2.ply` | 100,250 | 16 | 3 | 28 | 8 | 0.08 | 80 | 10 | 86.3% | Dragon 大规模真实模型 |
| `real/stanford_armadillo_50000.ply` | 50,000 | 16 | 3 | 28 | 8 | 0.08 | 50 | 42 | 81.6% | Armadillo 抽样真实模型 |

## 使用方式

GUI 中在“载入点云”输入框填入表中的路径，例如：

```text
data/stress_complex_35200.ply
```

然后按表格设置 `k`、`levels`、`最粗夹角`、`最细夹角`、`种子曲率阈值`、`最小段点数`，点击“运行分割”。

命令行运行示例：

```bash
./build/pcseg_cli data/stress_complex_35200.ply \
  -k 20 --levels 3 --coarse 32 --fine 12 --curv 0.10 --min 80 \
  -o build/stress_complex_segmented.ply

./build/pcseg_cli data/real/stanford_dragon_res2.ply \
  -k 16 --levels 3 --coarse 28 --fine 8 --curv 0.08 --min 80 \
  -o build/stanford_dragon_res2_segmented.ply
```

## 调参原则

- 灰色点过多：通常是小段被过滤，优先降低“最小段点数”。
- 彩色碎片过多：优先提高“最小段点数”，或适当提高“最细夹角”。
- 平面距离很近或结构很薄：适当降低 `k`、夹角阈值和曲率阈值，避免误合并。
- 噪声和离群点较多：适当提高 `k`、曲率阈值和最小段点数，减少碎片。
- 真实单体模型：分割结果通常表现为曲面 patch，不一定对应语义部件。

## 数据来源

`real/` 下的 Stanford Bunny、Dragon、Armadillo 来自 Stanford 3D Scanning Repository：

https://graphics.stanford.edu/data/3Dscanrep/

原始模型已转换为当前项目可直接读取的 ASCII PLY 点云格式，仅保留顶点坐标和颜色。
