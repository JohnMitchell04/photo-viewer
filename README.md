# This Repsoitory
An image viewer built to gain a greater understanding of C++ it is built using [Walnut](https://github.com/StudioCherno/Walnut) which is an application framework around [Dear ImGui](https://github.com/ocornut/imgui). The image decoder is built from scratch using no libraries other than [zlib](https://www.zlib.net/) for PNG decomoression. Currently only support for the [PNG](https://www.w3.org/TR/png-3/#) specification is supported but support for JPEG and other common formats is planned. Additionally support for some basic image editing functionality is planned.

# References
This project was devloped making reference to Vulkan code developed by Yan Chernovik in [Walnut](https://github.com/StudioCherno/Walnut), understanding of the format was taken from [the specification](https://www.w3.org/TR/png-3/#) and implementation of Adam7 interlacing was helped by reference to the javascript [TNG](https://github.com/chjj/tng/tree/master) library.

# Build
Run the `setup.bat` file in the scripts folder and a VS22 sln file will be created for the project that can be ran.
