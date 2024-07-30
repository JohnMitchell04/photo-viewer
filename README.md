# Image Viewer Project

This repository contains a PNG image viewer built as an educational project to improve my understanding of C++. The application uses the [Walnut](https://github.com/StudioCherno/Walnut) framework, which is built around [Dear ImGui](https://github.com/ocornut/imgui). The PNG image decoder is implemented from scratch, following the [PNG specification](https://www.w3.org/TR/png-3/#) and using [zlib](https://www.zlib.net/) for decompression. The decoder includes support for deinterlacing based on the Adam7 algorithm.

## Educational Outcomes

This project was developed to enhance my knowledge in several areas of C++ programming, including:
- Smart pointers for effective memory management.
- Using a major application framework like Dear ImGui.
- C++ exception handling for robust error management.

Additionally this project provided exposure to implementing detailed specifications for major technology standards. 

## References

This project was developed with reference to Vulkan code by Yan Chernikov in [Walnut](https://github.com/StudioCherno/Walnut). Understanding of the PNG format was guided by [the specification](https://www.w3.org/TR/png-3/#). The implementation of Adam7 interlacing was aided by the JavaScript [TNG](https://github.com/chjj/tng/tree/master) library.

## Build

To build the project, run the `setup.bat` file in the `scripts` folder. This will create a Visual Studio 2022 solution file that can be used to run the project.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.
