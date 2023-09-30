#include "Utils.h"

namespace ImageLibrary {
	namespace Utils {
		int GetPixelFormatByteSize(PixelFormat pixelFormat) {
			return GetChannelByteSize(pixelFormat) * (HasAlphaChannel(pixelFormat) ? 4 : 3);
		}

		int GetChannelByteSize(PixelFormat pixelFormat) {
			switch (pixelFormat) {
			case RGB8:
				[[fallthrough]];
			case RGBA8:
				return 1;
			case RGB16:
				[[fallthrough]];
			case RGBA16:
				return 2;
			}
		}

		bool HasAlphaChannel(PixelFormat pixelFormat) {
			switch (pixelFormat) {
			case RGBA8:
				[[fallthrough]];
			case RGBA16:
				return true;
			default:
				return false;
			}
		}

		namespace PNG {
			ChunkIdentifier StringToFormat(std::string string) {
				// Convert string specifier to know chunk enum
				// TODO: Decide which ancilliary chunks will be treated as unknown
				std::unordered_map<std::string, ChunkIdentifier> table{
					{"IHDR", IHDR}, {"PLTE", PLTE}, {"IDAT", IDAT}, {"IEND", IEND}
				};
				auto it = table.find(string);
				ChunkIdentifier chunkSpecifier = INVALID;

				if (it != table.end()) {
					chunkSpecifier = it->second;
				}

				// If chunk is ancilliary or private it does not matter that it can't be identified
				if (!isupper(string[0]) || !isupper(string[1])) {
					chunkSpecifier = UNKOWN;
				}

				return chunkSpecifier;
			}
		}
	}
}