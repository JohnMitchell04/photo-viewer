#pragma once

#include <string>
#include <unordered_map>

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

		// Pixel struct large enough to hold any pixel value
		struct Pixel {
			uint16_t R;
			uint16_t G;
			uint16_t B;
			uint16_t A;
		};

		template <typename T>
		concept IntegerType = std::is_integral<T>::value && !std::same_as<T, bool>;

		template <IntegerType T>
		void ExtractBigEndian(T& dest, uint8_t* src, int bytes) {
			dest = 0;
			for (int i = 0; i < bytes; i++) {
				dest |= ((T)src[i] << (8 * (bytes - 1 - i)));
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