#pragma once

#include "Core/ThunkDetail.h"

namespace rtk {

// ---------------------------------------------------------------------------
// API thunks
// ---------------------------------------------------------------------------

inline ThunkAction<FApiStatusResponse, FSDKState> doctorThunk() {
  return [](std::function<AnyAction(const AnyAction &)> Dispatch,
            std::function<FSDKState()> GetState)
             -> func::AsyncResult<FApiStatusResponse> {
    return APISlice::Endpoints::getApiStatus()(Dispatch, GetState);
  };
}

inline ThunkAction<FApiStatusResponse, FSDKState> checkApiStatusThunk() {
  return doctorThunk();
}

} // namespace rtk
