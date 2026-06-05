# Qt 跨平台网络文件传输工具

## 项目简介

本项目是一个基于 Qt 的跨平台网络文件传输工具，使用 TCP 协议实现 Windows 平台与 Linux 虚拟机平台之间的文件互传。

由于实验环境没有真实 ARM 开发板，因此本实验使用 CentOS 7 虚拟机模拟嵌入式 Linux 平台，实现与 Windows 平台之间的基础文件传输功能。

## 开发环境

- Windows + Qt Creator
- CentOS 7 64 位虚拟机
- Qt 5.9.7
- qmake-qt5
- g++

## 主要功能

- 启动接收端并监听指定端口
- 输入目标 IP 和端口连接接收端
- 选择本地文件
- 发送文件
- 接收文件并保存到 received_files 文件夹
- 支持 Windows 与 Linux 虚拟机双向传输

## 编译方法

Linux 下进入工程目录后执行：

```bash
qmake-qt5 test.pro
make
./test
