# 二维弹球
游戏玩法极其简单，大部分提示在开始窗口中给出。

使用C++ 17标准，在Windows 10 + VS 2017下编译通过。使用了我开发的另一个Windows API封装库[SimpleWindows](https://github.com/Berrysoft/SimpleWindows)。

为了有可能添加的本地化支持，使用了[StreamFormat](https://github.com/Berrysoft/StreamFormat)。
## 游戏特点
球的碰撞是弹性碰撞，如果碰到直角会以45°反弹。

方块的颜色随上面的数字变化，由蓝到紫再到红。

三个难度各有不同，一般来说，简单可以无尽，正常最高大概13000分，困难最高大概9000分。
