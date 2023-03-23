## 该程序旨在对图像应用高斯模糊。 它是通过使用 OpenGL 3.3 版在 Ubuntu 上实现的。

## 高斯模糊有 3 种不同的实现方式。

     * Naive implementation：
         - 在 O(n^2) 时间内运行。

     * Two-pass implementation:
         - 使用高斯函数的属性。 在不同的通道中垂直和水平应用相同的效果。 因此，它的运行时间为 O(2*n)。

     * Two-pass with bilinear filtering:：
         - 除了两次通过属性外，还使用硬件实现的双线性过滤。 它减少了像素获取的数量。

## 用法：
###  首先调用下面2条命令进行编译。

     * make clean

     * make
### 之后执行下面这条语句：
     * ./blur <name_of_the_texture_image> <type_of_implementation>  # 输入例子:./blur wall.jpg 3

     * <type_of_implementation> 运行简单的实现类型 1。

     * <type_of_implementation> 运行两遍实现类型 2。

     * <type_of_implementation> 使用双线性过滤实现类型 3 运行两次。

### 想要在Ubuntu上使用 glew、GLFW和glm，以及其他openGL相关库，你可能需要执行以下操作。
* sudo apt-get install build-essential libgl1-mesa-dev
* sudo apt-get install freeglut3-dev
* sudo apt-get install libglew-dev libsdl2-dev libsdl2-image-dev libglm-dev libfreetype6-dev