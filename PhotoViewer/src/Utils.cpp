#include "Utils.h"

namespace ImageLibrary {
	namespace Utils {
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