# deepeye
一个简单的基于TCP的图片服务器程序，实现了图片流的接收和发送。

Prerequisites
--
* JSON library for C
* OpenCV 2.X or OpenCV 3.X

Complie Instruction
--
    gcc qbr.c -o qbr -lpthread -ljson
    gcc qbr_s.c -o qbr_s -lpthread -ljson
