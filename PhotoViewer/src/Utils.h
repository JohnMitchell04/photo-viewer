#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>

namespace ImageLibrary {
	namespace Utils {
		// The max PNG size would be far too large for this application to handle so create an arbitrary limit
		inline constexpr uint32_t PNG_SPEC_MAX_DIMENSION = 2147483647;
		inline constexpr uint32_t PNG_APP_MAX_DIMENSION = 16384;

		// Signatures of different image formats
		enum FormatSignatures {
			PNG_SIGNATURE = 0xC7
		};

		// Pixel format types
		enum PixelFormat {
			RGB8,
			RGB16,
			RGBA8,
			RGBA16,
			INVALID
		};

		int GetPixelFormatByteSize(PixelFormat pixelFormat);
		int GetChannelByteSize(PixelFormat pixelFormat);
		bool HasAlphaChannel(PixelFormat pixelFormat);

		// Pixel struct large enough to hold any pixel value
		struct Pixel { uint16_t R = 0, G = 0, B = 0, A = 0; };

		template <typename T>
		concept IntegerType = std::is_integral<T>::value && !std::same_as<T, bool>;

		template <IntegerType T>
		void ExtractBigEndianBytes(T& dest, uint8_t* src, int number) {
			if (sizeof(dest) < number) { throw new std::invalid_argument("Error: Destination cannot hold number of bytes"); }
			dest = 0;
			for (int i = 0; i < number; i++) {
				dest |= ((T)src[i] << (8 * (number - 1 - i)));
			}
		}

		template <typename T>
		concept EvenSizeIntegerType = IntegerType<T> && (sizeof(T) % 2 == 0);

		template <EvenSizeIntegerType T>
		void FlipEndian(T& dest, T& src) {
			T temp = src;
			dest = 0;
			for (int i = 0; i < sizeof(T) / 2; i++) {
				int shiftAmount = sizeof(T) * 8 - (i + 1) * 8;
				dest |= (temp << shiftAmount & 0xff << shiftAmount) | (temp >> shiftAmount & (0xff << (sizeof(T) - 1) * 8) >> shiftAmount);
			}
		}

		namespace PNG {
			// TODO: Decide which ancilliary chunks will be treated as unknown
			enum ChunkIdentifier {
				IHDR = 100,
				PLTE = 101,
				IDAT = 102,
				IEND = 103,
				cHRM = 200,
				cICP = 201,
				gAMA = 202,
				iCCP = 203,
				mDCv = 204,
				cLLi = 205,
				sBIT = 206,
				sRGB = 207,
				bKGD = 208,
				hIST = 209,
				tRNS = 210,
				eXIf = 211,
				pHYs = 212,
				sPLT = 213,
				tIME = 214,
				iTXt = 215,
				tEXt = 216,
				zTXt = 217,
				UNKOWN = 0,
				INVALID = -1
			};

			struct Chunk {
				ChunkIdentifier identifier;
				int position;
			};

			ChunkIdentifier StringToFormat(std::string string);
		}
	}
}