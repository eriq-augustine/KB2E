#ifndef COMMON_LOADER_H_
#define COMMON_LOADER_H_

#include <functional>
#include <map>
#include <string>

namespace common {

// Load the file into |idMap|.
void loadIdFile(std::string path, std::map<std::string, int>& idMap);

// Loads one of our standard tripple map that is of the form "head\ttail\trelation"
// Note that we are expecting string identifiers in the file, but will convert them to ints.
// |callback| is called for each triple read (in the order they are read) unless the
// triple is not valid.
void loadTripleFile(std::string path,
                    std::map<std::string, int>& entityIdMap, std::map<std::string, int>& relationIdMap,
                    std::function<void(int, int, int)> callback);

}   // namespace common

#endif  // COMMON_LOADER_H_
