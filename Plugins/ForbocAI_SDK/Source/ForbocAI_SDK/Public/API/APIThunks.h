#pragma once

#include "Core/ThunkDetail.h"

namespace rtk {

// ---------------------------------------------------------------------------
// API thunks
// ---------------------------------------------------------------------------

inline ThunkAction<FApiStatusResponse, FStoreState> doctorThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FStoreState()> GetState)
             -> func::AsyncResult<FApiStatusResponse> {
    return APISlice::Endpoints::getApiStatus()(Dispatch, GetState);
  };
}

inline ThunkAction<FApiStatusResponse, FStoreState> checkApiStatusThunk() {
  return doctorThunk();
}

} // namespace rtk
