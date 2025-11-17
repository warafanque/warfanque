# 球体Gouraud光照模型实验

这是一个使用C++和Win32 API实现的3D球体渲染程序，具有Gouraud光照模型和Z-Buffer消隐算法。

## 功能特性

1. **坐标系 (Coordinate System)**: 
   - 在屏幕客户区中心建立右手三维坐标系
   - X轴水平向右为正
   - Y轴垂直向上为正
   - Z轴垂直屏幕指向观察者为正

2. **地理划分 (Geographic Subdivision)**:
   - 采用经纬网格法细分球面
   - 构建三角形网格
   - 30个纬度分段，40个经度分段

3. **消隐 (Hidden Surface Removal)**:
   - 使用Z-Buffer算法进行消隐
   - 球体中心与坐标系原点重合

4. **光照 (Lighting)**:
   - 实现Gouraud着色（顶点着色插值）
   - 单点光源位于球体右上方
   - 支持环境光、漫反射光和镜面反射光

5. **背景色 (Background Color)**:
   - 固定为RGB(128, 0, 0) - 深红色

6. **交互 (Interaction)**:
   - 方向键旋转球体：
     - ← 左箭头：向左旋转
     - → 右箭头：向右旋转
     - ↑ 上箭头：向上旋转
     - ↓ 下箭头：向下旋转

7. **动画 (Animation)**:
   - 工具栏"动画"按钮控制
   - 点击播放/停止球体自动旋转

## 构建说明

### Windows + MinGW/GCC
```bash
make
```

### Windows + Visual Studio
使用Visual Studio命令提示符运行：
```bash
build.bat
```

或者使用cl编译器直接编译：
```bash
cl /EHsc /O2 /W3 /D_UNICODE /DUNICODE sphere_gouraud.cpp /link user32.lib gdi32.lib comctl32.lib /SUBSYSTEM:WINDOWS /OUT:sphere_gouraud.exe
```

## 运行

编译后运行生成的可执行文件：
```bash
sphere_gouraud.exe
```

## 技术实现

### 球体网格生成
使用球坐标系（θ, φ）生成球面顶点：
- θ (theta): 纬度角，范围[0, π]
- φ (phi): 经度角，范围[0, 2π]
- 顶点坐标：
  - x = R * sin(θ) * cos(φ)
  - y = R * cos(θ)
  - z = R * sin(θ) * sin(φ)

### Gouraud着色
1. 在每个顶点计算光照颜色
2. 光照模型包含：
   - 环境光 (Ambient)
   - 漫反射 (Diffuse)
   - 镜面反射 (Specular, Blinn-Phong)
3. 三角形内部通过重心坐标插值顶点颜色

### Z-Buffer算法
1. 为每个像素维护深度值
2. 渲染时比较当前片段深度与缓冲区深度
3. 只绘制更靠近观察者的片段

### 背面剔除
通过叉积计算三角形法向量，只渲染面向观察者的三角形。

## 依赖

- Windows SDK (for Win32 API)
- C++11或更高版本

## 文件说明

- `sphere_gouraud.cpp`: 主程序源代码
- `Makefile`: GCC/MinGW构建脚本
- `build.bat`: Visual Studio构建脚本
- `README.md`: 本文档

## 许可

本项目为教育实验项目。
