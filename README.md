# 二维弹球
游戏玩法极其简单，大部分提示在开始窗口中给出。

使用 Rust，在 Windows 11, Linux, Mac OS X 下编译通过。使用了我开发的另一个 GUI 封装库 [winio](https://crates.io/crates/winio)。

[![Azure DevOps builds](https://strawberry-vs.visualstudio.com/Balls/_apis/build/status/Berrysoft.Balls?branch=master)](https://strawberry-vs.visualstudio.com/Balls/_build)

## 游戏特点
球的碰撞是弹性碰撞，如果碰到直角会以45°反弹。

方块的颜色随上面的数字变化，由蓝到紫再到红。

## 编译
```
cargo build --release
```
