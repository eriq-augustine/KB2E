#include "common/loader.h"

#include <cstdio>
#include <functional>
#include <iostream>
#include <map>
#include <string>

#include "common/constants.h"

namespace common {

// TODO(eriq): Better error handling with files.

void loadIdFile(std::string path, std::map<std::string, int>& idMap) {
   int intId;
   char idStringBuf[ID_STRING_MAX_LEN];

   FILE* idFile = fopen(path.c_str(), "r");
   while (fscanf(idFile, "%s\t%d", idStringBuf, &intId) == 2) {
      idMap[std::string(idStringBuf)] = intId;
   }
   fclose(idFile);
}

void loadTripleFile(std::string path,
                    std::map<std::string, int>& entityIdMap, std::map<std::string, int>& relationIdMap,
                    std::function<void(int, int, int)> callback) {
   char headIdStringBuf[ID_STRING_MAX_LEN];
   char tailIdStringBuf[ID_STRING_MAX_LEN];
   char relationIdStringBuf[ID_STRING_MAX_LEN];

   FILE* tripleFile = fopen(path.c_str(), "r");
   while (fscanf(tripleFile, "%s\t%s\t%s", headIdStringBuf, tailIdStringBuf, relationIdStringBuf) == 3) {
      std::string headIdString = headIdStringBuf;
      std::string tailIdString = tailIdStringBuf;
      std::string relationIdString = relationIdStringBuf;

      bool fail = false;
      if (entityIdMap.count(headIdString) == 0) {
         std::cout << "Head entity found in triple file that was not found in the identity file: " << headIdString << std::endl;
         fail = true;
      }

      if (entityIdMap.count(tailIdString) == 0) {
         std::cout << "Tail entity found in triple file that was not found in the identity file: " << tailIdString << std::endl;
         fail = true;
      }

      if (relationIdMap.count(relationIdString) == 0) {
         std::cout << "Relation found in triple file that was not found in the identity file: " << relationIdString << std::endl;
         fail = true;
      }

      if (fail) {
         continue;
      }

      callback(entityIdMap[headIdString], entityIdMap[tailIdString], relationIdMap[relationIdString]);
   }
   fclose(tripleFile);
}

}   // namespace common
