# Real Point Cloud Samples

这些文件来自 Stanford 3D Scanning Repository，并已转换成当前项目最稳妥的点云格式：
ASCII PLY，仅保留 `x/y/z` 顶点坐标和统一 RGB 颜色，不包含 mesh face。

来源：

- Stanford 3D Scanning Repository: https://graphics.stanford.edu/data/3Dscanrep/
- Bunny archive: https://graphics.stanford.edu/pub/3Dscanrep/bunny.tar.gz
- Dragon reconstruction archive: https://graphics.stanford.edu/pub/3Dscanrep/dragon/dragon_recon.tar.gz
- Armadillo PLY: https://graphics.stanford.edu/pub/3Dscanrep/armadillo/Armadillo.ply.gz

文件：

| 文件 | 点数 | 用途 |
|---|---:|---|
| `stanford_bunny_res3.ply` | 1,889 | 快速调试 |
| `stanford_bunny_res2.ply` | 8,171 | 轻量演示 |
| `stanford_bunny.ply` | 35,947 | Bunny 完整重建点云 |
| `stanford_dragon_res3.ply` | 22,998 | 中等复杂曲面 |
| `stanford_dragon_res2.ply` | 100,250 | 大规模真实模型压力测试 |
| `stanford_armadillo_50000.ply` | 50,000 | 从 Armadillo 抽样出的压力测试 |

这些文件会出现在 Web 页面左侧“载入点云”的下拉框中。选择文件并点击“载入文件”即可在浏览器端查看和分割。
