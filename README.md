# 3D 点云目标分割系统 Web 版

这是 `web` 分支，面向 GitHub Pages 部署。页面为静态站点，不需要 CMake、C++ 编译器或后端服务。

## 使用方式

本地预览：

```bash
python3 -m http.server 8000
```

打开：

```text
http://localhost:8000
```

GitHub Pages 部署：

1. 推送 `web` 分支。
2. 进入仓库 `Settings -> Pages`。
3. Source 选择 `Deploy from a branch`。
4. Branch 选择 `web`，目录选择 `/ (root)`。

## 内容

- `index.html`：页面入口。
- `web/app.js`：浏览器端 PLY 读取、点云显示、法向量估计、区域生长分割、导出 PLY。
- `web/styles.css`：复刻本地 GUI 的控制面板样式。
- `data/`：页面下拉框可直接选择的点云文件。

## 说明

Web 版使用 Three.js 渲染点云，并在浏览器端复刻 C++ 版的主要算法流程。由于运行环境和数值实现不同，分割结果会尽量接近本地版，但不保证逐点完全一致。大点云文件会有明显计算耗时。
