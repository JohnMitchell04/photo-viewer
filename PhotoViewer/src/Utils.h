#pragma once

#include <string>

namespace ImageLibrary {
	namespace Utils {
		// The max PNG size would be far too large for this application to handle so create an arbitrary limit
		constexpr uint32_t PNG_SPEC_MAX_DIMENSION = 2147483647;
		constexpr uint32_t PNG_APP_MAX_DIMENSION = 16384;

		enum FormatSignatures {
			PNG_SIGNATURE = 0xC7
		};

		enum PNGChunkIdentifier {
			IHDR,
			PLTE,
			IDAT,
			INVALID
		};

		struct PNGChunk {
			PNGChunkIdentifier identifier;
			int position;
		};
	}
}