# 实习报告（LaTeX）构建说明

本报告基于西北农林科技大学 `nwafupaper` 文档类（`expl3`/`ctex`），**必须用 XeLaTeX 编译**。

## 编译

```bash
cd docs/report
xelatex -interaction=nonstopmode main.tex
biber main
xelatex -interaction=nonstopmode main.tex
xelatex -interaction=nonstopmode main.tex   # 共 3 遍，解决目录与交叉引用
```

产物为 `main.pdf`（已随仓库提交一份）。

## 中文字体

`main.tex` 中设置了 `cjk-font = fandol`，使用 TeX Live 自带的 **Fandol** 中文字体，
无需安装商业字体。

- 在 **Linux / Windows** 的完整 TeX Live 上，fontconfig 会索引 texmf 目录，可直接按名找到 Fandol，无需额外操作。
- 在 **macOS**（XeTeX 经 Core Text 找字体，默认不索引 texmf 目录）上，若编译报
  “mktextfm FandolSong/OT”一类错误，把 Fandol 字体装到用户字体目录即可：

  ```bash
  cp "$(dirname "$(kpsewhich FandolSong-Regular.otf)")"/Fandol*.otf ~/Library/Fonts/
  ```

  也可改用系统自带中文字体：把 `main.tex` 里的 `cjk-font = fandol` 改为 `cjk-font = mac`
  （需要系统装有 STSong 等字体）。

## 目录

- `main.tex`：入口，已填好封面信息、设为 Fandol 字体、引入正文与附录。
- `contents/abstract.tex`：摘要。
- `contents/report.tex`：报告正文（目的/任务/设计/实现/测试/辅助工具说明/日志/总结）。
- `contents/app-requirements.tex`：附录·程序设计与报告要求（模板自带）。
- `contents/app-codelists.tex`：附录·核心代码清单（引入 `code/` 下的源文件）。
- `code/`：从工程 `core/` 复制的关键源文件，供代码清单引用。
- `figs/gui_*.png`：图形界面实际渲染输出的插图。
