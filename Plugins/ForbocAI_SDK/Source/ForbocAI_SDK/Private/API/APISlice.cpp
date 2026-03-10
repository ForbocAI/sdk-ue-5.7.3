#include "API/APISlice.h"
#include "RuntimeStore.h"

// Define the global API slice instance
rtk::ApiSlice<FStoreState> APISlice::ForbocAiApi = []() {
  rtk::ApiSlice<FStoreState> Api;
  Api.ReducerPath = TEXT("forbocApi");
  Api.TagTypes = {TEXT("NPC"),  TEXT("Memory"), TEXT("Cortex"), TEXT("Ghost"),
                  TEXT("Soul"), TEXT("Bridge"), TEXT("Rule")};
  return Api;
}();
