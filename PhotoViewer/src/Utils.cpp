#include "Utils.h"

namespace ImageLibrary {
	namespace Utils {
		PNGChunkIdentifier StringToFormat(std::string string) {
			// Convert string specifier to know chunk enum
			// TODO: Decide which ancilliary chunks will be treated as unknown
			std::unordered_map<std::string, Utils::PNGChunkIdentifier> table{ {"IHDR", Utils::IHDR}, {"PLTE", Utils::PLTE} };
			auto it = table.find(string);
			Utils::PNGChunkIdentifier chunkSpecifier = Utils::INVALID;

			if (it != table.end()) {
				chunkSpecifier = it->second;
			}

			// If chunk is ancilliary or private it does not matter that it can't be identified
			if (!isupper(string[0]) || !isupper(string[1])) {
				chunkSpecifier = Utils::UNKOWN;
			}

			return chunkSpecifier;
		}
	}
}