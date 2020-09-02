# 二维弹球
游戏玩法极其简单，大部分提示在开始窗口中给出。

使用 C++ 20 标准，在 Windows 10, Linux, macOS X 下编译通过。使用了我开发的另一个 GUI 封装库 [XamlCpp](https://github.com/Berrysoft/XamlCpp)。

[![Azure DevOps builds](https://strawberry-vs.visualstudio.com/Balls/_apis/build/status/Berrysoft.Balls?branch=master)](https://strawberry-vs.visualstudio.com/Balls/_build)

## 游戏特点
球的碰撞是弹性碰撞，如果碰到直角会以45°反弹。

方块的颜色随上面的数字变化，由蓝到紫再到红。

## 编译
使用 VS 2019, GCC 10.2, Clang 10.0 以上的编译器。

可使用静态链接来减小游戏体积，或使用动态链接提高扩展性。
