RayTracingInOneWeekend
---
 
This is my implementation of [_Ray Tracing in One Weekend_](https:/raytracing.github.io/books/RayTracingInOneWeekend.html)

Each branch implements different versions or optimizations:

- master (You are here!): The code as it appears on the website. This is an offline, non-real-time renderer.
- constexpr: As [suggested](https://youtu.be/cpdjQiRxEJ8) by Jason Turner, all things (as much as possible) have been declared `constexpr`. I already prescribe to "auto all the things" and "const all the things", so those had already been done.
- DX11PS: The code modified to run on a DirectX 11 Pixel Shader.
- DX11CS: The code modified to run on a DirectX 11 Compute Shader.
- DX12RT: The code modified to run on a DirectX 12 Ray-trace-enabled GPU. (You must have a GPU that supports ray tracing!)