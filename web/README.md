# Web 版说明

本分支提供 GitHub Pages 可直接托管的静态 Web 版点云分割系统。

部署方式：

1. 推送 `web` 分支到 GitHub。
2. 在仓库 `Settings -> Pages` 中选择 `Deploy from a branch`。
3. Branch 选择 `web`，目录选择 `/ (root)`。

本地预览：

```bash
python3 -m http.server 8000
```

然后访问：

```text
http://localhost:8000
```

说明：

- Web 版使用 Three.js 进行点云渲染。
- PLY 读取、法向量估计、区域生长分割和 PLY 导出均在浏览器端完成。
- 由于浏览器运行环境和 C++ 原版不同，分割结果会尽量复刻原逻辑，但无法保证与 C++ 版逐点完全一致。
- 大点云文件如 `stress_large_100000.ply`、`stanford_dragon_res2.ply` 会有明显计算耗时。
