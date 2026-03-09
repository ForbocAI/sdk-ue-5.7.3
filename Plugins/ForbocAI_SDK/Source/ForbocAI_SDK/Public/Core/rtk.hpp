#pragma once
#ifndef RTK_HPP
#define RTK_HPP

#include "CoreMinimal.h"
#include "functional_core.hpp"
#include <functional>
#include <memory>
#include <utility>
#include <vector>

namespace rtk {

// ==========================================
// Phase 1: Store Foundation
// ==========================================

// 1.1 Action<Payload>
template <typename Payload> struct Action {
  FString Type;
  Payload PayloadValue;
};

struct FEmptyPayload {};

template <typename Payload>
Action<Payload> makeAction(const FString &Type, const Payload &PayloadValue) {
  return Action<Payload>{Type, PayloadValue};
}

inline Action<FEmptyPayload> makeAction(const FString &Type) {
  return Action<FEmptyPayload>{Type, FEmptyPayload{}};
}

// Type-erased envelope for heterogeneous root dispatch
struct AnyAction {
  FString Type;
  std::shared_ptr<void> PayloadWrapper;

  AnyAction() {}

  AnyAction(const FString &InType, std::shared_ptr<void> InPayloadWrapper)
      : Type(InType),
        PayloadWrapper(std::move(InPayloadWrapper)) {}

  // WARNING: This performs an unchecked static_cast on the type-erased
  // payload. Callers must ensure the requested Payload type matches
  // the type originally stored. Mismatched types are undefined behavior.
  // Prefer ActionCreator::extract() which pairs type-string matching
  // with extraction for safer access.
  template <typename Payload> func::Maybe<Payload> getPayload() const {
    if (!PayloadWrapper) {
      return func::nothing<Payload>();
    }
    return func::just(*static_cast<Payload *>(PayloadWrapper.get()));
  }
};

// 1.2 Reducer
template <typename State, typename ActionT>
using Reducer = std::function<State(const State &, const ActionT &)>;

template <typename State>
using CaseReducer = std::function<State(const State &, const AnyAction &)>;

// 1.3 Store
template <typename State> struct Store {
  State CurrentState;
  CaseReducer<State> RootReducer;

  struct Subscriber {
    int64_t Id;
    std::function<void()> Callback;
  };
  std::vector<Subscriber> Subscribers;
  int64_t NextId = 1;

  Store(State InitialState, CaseReducer<State> ReducerFunc)
      : CurrentState(std::move(InitialState)),
        RootReducer(std::move(ReducerFunc)) {}

  const State &getState() const { return CurrentState; }

  AnyAction dispatch(const AnyAction &action) {
    CurrentState = RootReducer(CurrentState, action);
    // Copy subscribers so list is stable if unsubscribed during callback
    auto SubsCopy = Subscribers;
    for (const auto &sub : SubsCopy) {
      sub.Callback();
    }
    return action;
  }

  std::function<void()> subscribe(std::function<void()> Callback) {
    int64_t Id = NextId++;
    Subscribers.push_back({Id, std::move(Callback)});

    // Return unsubscribe callable Handle
    return [this, Id]() {
      for (auto it = Subscribers.begin(); it != Subscribers.end(); ++it) {
        if (it->Id == Id) {
          Subscribers.erase(it);
          break;
        }
      }
    };
  }
};

// 1.4 combineReducers
template <typename RootState> class ReducerCombiner {
  std::vector<
      std::function<bool(RootState &, const RootState &, const AnyAction &)>>
      bindings;

public:
  template <typename SliceState>
  ReducerCombiner &add(SliceState RootState::*member,
                       CaseReducer<SliceState> reducer) {
    bindings.push_back([member, reducer](RootState &nextState,
                                         const RootState &prevState,
                                         const AnyAction &action) {
      const SliceState &prevSlice = prevState.*member;
      SliceState nextSlice = reducer(prevSlice, action);
      bool changed = !(prevSlice == nextSlice);
      if (changed) {
        nextState.*member = std::move(nextSlice);
      }
      return changed;
    });
    return *this;
  }

  CaseReducer<RootState> build() const {
    auto b = bindings;
    return
        [b](const RootState &prevState, const AnyAction &action) -> RootState {
          RootState nextState = prevState;
          bool changed = false;
          for (const auto &binding : b) {
            if (binding(nextState, prevState, action)) {
              changed = true;
            }
          }
          return changed ? nextState : prevState;
        };
  }
};

template <typename RootState> ReducerCombiner<RootState> combineReducers() {
  return ReducerCombiner<RootState>();
}

// ==========================================
// Phase 2: Slice Pattern
// ==========================================

// 2.1 createAction<P> and Matchers
template <typename Payload> struct ActionCreator {
  FString Type;

  AnyAction operator()(const Payload &payload) const {
    return AnyAction(Type, std::make_shared<Payload>(payload));
  }

  bool match(const AnyAction &action) const { return action.Type == Type; }

  func::Maybe<Payload> extract(const AnyAction &action) const {
    if (action.Type == Type && action.PayloadWrapper) {
      return func::just(*static_cast<Payload *>(action.PayloadWrapper.get()));
    }
    return func::nothing<Payload>();
  }
};

template <typename Payload>
ActionCreator<Payload> createAction(const FString &Type) {
  return ActionCreator<Payload>{Type};
}

struct EmptyActionCreator {
  FString Type;

  AnyAction operator()() const {
    return AnyAction(Type, std::make_shared<FEmptyPayload>());
  }

  bool match(const AnyAction &action) const { return action.Type == Type; }
};

inline EmptyActionCreator createAction(const FString &Type) {
  return EmptyActionCreator{Type};
}

// 2.2 Slice<State>
template <typename State> struct Slice {
  FString Name;
  CaseReducer<State> Reducer;
};

// 2.3 SliceBuilder for generating slices and binding action creators locally
template <typename State> class SliceBuilder {
  FString Name;
  State InitialState;
  TMap<FString, CaseReducer<State>> Reducers;

public:
  SliceBuilder(FString InName, State InInitialState)
      : Name(MoveTemp(InName)), InitialState(MoveTemp(InInitialState)) {}

  template <typename Payload>
  ActionCreator<Payload>
  createCase(const FString &ShortName,
             std::function<State(const State &, const Action<Payload> &)>
                 ReducerFunc) {
    FString FullType = Name + TEXT("/") + ShortName;
    ActionCreator<Payload> Creator{FullType};

    Reducers.Add(FullType,
                 [Creator, ReducerFunc](const State &prevState,
                                        const AnyAction &anyAction) -> State {
                   auto payloadOpt = Creator.extract(anyAction);
                   if (payloadOpt.hasValue) {
                     return ReducerFunc(
                         prevState,
                         makeAction(anyAction.Type, payloadOpt.value));
                   }
                   return prevState;
                 });

    return Creator;
  }

  EmptyActionCreator
  createCase(const FString &ShortName,
             std::function<State(const State &, const Action<FEmptyPayload> &)>
                 ReducerFunc) {
    FString FullType = Name + TEXT("/") + ShortName;
    EmptyActionCreator Creator{FullType};

    Reducers.Add(FullType,
                 [Creator, ReducerFunc](const State &prevState,
                                        const AnyAction &anyAction) -> State {
                   if (Creator.match(anyAction)) {
                     return ReducerFunc(prevState, makeAction(anyAction.Type));
                   }
                   return prevState;
                 });

    return Creator;
  }

  template <typename Payload, typename ReducerFn>
  SliceBuilder &
  addExtraCase(const ActionCreator<Payload> &Creator, ReducerFn ReducerFunc) {
    std::function<State(const State &, const Action<Payload> &)> WrappedReducer =
        ReducerFunc;
    Reducers.Add(Creator.Type,
                 [Creator, WrappedReducer](const State &prevState,
                                           const AnyAction &anyAction) -> State {
                   auto payloadOpt = Creator.extract(anyAction);
                   if (payloadOpt.hasValue) {
                     return WrappedReducer(
                         prevState,
                         makeAction(anyAction.Type, payloadOpt.value));
                   }
                   return prevState;
                 });
    return *this;
  }

  template <typename ReducerFn>
  SliceBuilder &addExtraCase(const EmptyActionCreator &Creator,
                             ReducerFn ReducerFunc) {
    std::function<State(const State &, const Action<FEmptyPayload> &)>
        WrappedReducer = ReducerFunc;
    Reducers.Add(Creator.Type,
                 [Creator, WrappedReducer](const State &prevState,
                                           const AnyAction &anyAction) -> State {
                   if (Creator.match(anyAction)) {
                     return WrappedReducer(prevState,
                                           makeAction(anyAction.Type));
                   }
                   return prevState;
                 });
    return *this;
  }

  Slice<State> build() const {
    Slice<State> S;
    S.Name = Name;
    auto DefaultState = InitialState;
    auto Map = Reducers;

    S.Reducer = [DefaultState, Map](const State &prevState,
                                    const AnyAction &action) -> State {
      if (const CaseReducer<State> *Found = Map.Find(action.Type)) {
        return (*Found)(prevState, action);
      }
      return prevState;
    };
    return S;
  }
};

// ==========================================
// Phase 3: Entity Adapter
// ==========================================

template <typename T> struct EntityState {
  TArray<FString> ids;
  TMap<FString, T> entities;
};

template <typename T> struct EntitySelectors {
  std::function<TArray<T>(const EntityState<T> &)> selectAll;
  std::function<func::Maybe<T>(const EntityState<T> &, const FString &)>
      selectById;
  std::function<TArray<FString>(const EntityState<T> &)> selectIds;
  std::function<int32_t(const EntityState<T> &)> selectTotal;
};

template <typename T> struct EntityAdapterOps {
  std::function<FString(const T &)> selectId;

  EntityState<T> getInitialState() const { return EntityState<T>{{}, {}}; }

  // Mutators (Pure functions returning a new state)
  EntityState<T> addOne(const EntityState<T> &state, const T &entity) const {
    EntityState<T> next = state;
    FString id = selectId(entity);
    if (!next.entities.Find(id)) {
      next.ids.Add(id);
      next.entities.Add(id, entity);
    }
    return next;
  }

  EntityState<T> addMany(const EntityState<T> &state,
                         const TArray<T> &newEntities) const {
    EntityState<T> next = state;
    for (const auto &entity : newEntities) {
      FString id = selectId(entity);
      if (!next.entities.Find(id)) {
        next.ids.Add(id);
        next.entities.Add(id, entity);
      }
    }
    return next;
  }

  EntityState<T> setOne(const EntityState<T> &state, const T &entity) const {
    EntityState<T> next = state;
    FString id = selectId(entity);
    if (!next.entities.Find(id)) {
      next.ids.Add(id);
    }
    next.entities.Add(id, entity);
    return next;
  }

  EntityState<T> setAll(const EntityState<T> &state,
                        const TArray<T> &newEntities) const {
    EntityState<T> next;
    for (const auto &entity : newEntities) {
      FString id = selectId(entity);
      next.ids.Add(id);
      next.entities.Add(id, entity);
    }
    return next;
  }

  EntityState<T> upsertOne(const EntityState<T> &state, const T &entity) const {
    return setOne(state, entity);
  }

  EntityState<T> upsertMany(const EntityState<T> &state,
                            const TArray<T> &entitiesToUpsert) const {
    EntityState<T> next = state;
    for (const auto &entity : entitiesToUpsert) {
      next = setOne(next, entity);
    }
    return next;
  }

  EntityState<T> removeOne(const EntityState<T> &state,
                           const FString &id) const {
    EntityState<T> next = state;
    if (next.entities.Remove(id) > 0) {
      next.ids.Remove(id);
    }
    return next;
  }

  EntityState<T> removeMany(const EntityState<T> &state,
                            const TArray<FString> &removeIds) const {
    EntityState<T> next = state;
    for (const auto &id : removeIds) {
      if (next.entities.Remove(id) > 0) {
        next.ids.Remove(id);
      }
    }
    return next;
  }

  EntityState<T> removeAll(const EntityState<T> & /*state*/) const {
    return getInitialState();
  }

  EntityState<T> updateOne(const EntityState<T> &state, const FString &id,
                           std::function<T(const T &)> patch) const {
    EntityState<T> next = state;
    if (const T *existing = next.entities.Find(id)) {
      next.entities.Add(id, patch(*existing));
    }
    return next;
  }

  // Selectors factory
  EntitySelectors<T> getSelectors() const {
    return EntitySelectors<T>{
        /* selectAll */
        [](const EntityState<T> &state) -> TArray<T> {
          TArray<T> result;
          for (const auto &id : state.ids) {
            if (const T *ent = state.entities.Find(id)) {
              result.Add(*ent);
            }
          }
          return result;
        },
        /* selectById */
        [](const EntityState<T> &state, const FString &id) -> func::Maybe<T> {
          if (const T *ent = state.entities.Find(id)) {
            return func::just(*ent);
          }
          return func::nothing<T>();
        },
        /* selectIds */
        [](const EntityState<T> &state) -> TArray<FString> {
          return state.ids;
        },
        /* selectTotal */
        [](const EntityState<T> &state) -> int32_t {
          return state.ids.Num();
        }};
  }
};

template <typename T>
EntityAdapterOps<T>
createEntityAdapter(std::function<FString(const T &)> selectId) {
  return EntityAdapterOps<T>{std::move(selectId)};
}

// ==========================================
// Phase 4: Async Thunks
// ==========================================

template <typename State> struct ThunkApi {
  std::function<AnyAction(const AnyAction &)> dispatch;
  std::function<State()> getState;
};

template <typename Result, typename State>
using ThunkAction = std::function<func::AsyncResult<Result>(
    std::function<AnyAction(const AnyAction &)>, std::function<State()>)>;

template <typename Result, typename Arg, typename State>
struct AsyncThunkConfig {
  FString TypePrefix;
  ActionCreator<Arg> pending;
  ActionCreator<Result> fulfilled;
  ActionCreator<FString> rejected;

  std::function<ThunkAction<Result, State>(const Arg &)> thunkActionCreator;

  ThunkAction<Result, State> operator()(const Arg &arg) const {
    return thunkActionCreator(arg);
  }
};

template <typename Result, typename Arg, typename State>
AsyncThunkConfig<Result, Arg, State> createAsyncThunk(
    const FString &TypePrefix,
    std::function<func::AsyncResult<Result>(const Arg &,
                                            const ThunkApi<State> &)>
        PayloadCreator) {
  auto pending = createAction<Arg>(TypePrefix + TEXT("/pending"));
  auto fulfilled = createAction<Result>(TypePrefix + TEXT("/fulfilled"));
  auto rejected = createAction<FString>(TypePrefix + TEXT("/rejected"));

  auto thunkActionCreator = [pending, fulfilled, rejected, PayloadCreator](
                                const Arg &arg) -> ThunkAction<Result, State> {
    return [pending, fulfilled, rejected, PayloadCreator,
            arg](std::function<AnyAction(const AnyAction &)> dispatch,
                 std::function<State()> getState) -> func::AsyncResult<Result> {
      // 1. Dispatch pending synchronously
      dispatch(pending(arg));

      // 2. Build the ThunkApi surface
      ThunkApi<State> api{dispatch, getState};

      // 3. Execute payload creator
      auto result = PayloadCreator(arg, api);

      // 4. Chain lifecycle actions using FP core AsyncChain.
      //    The returned AsyncResult carries both the fulfilled chain and
      //    the catch_ error handler. The caller is responsible for calling
      //    .execute() on the returned result to trigger the full chain.
      return func::AsyncChain::then<Result, Result>(
                 result,
                 [dispatch, fulfilled](Result res) {
                   dispatch(fulfilled(res));
                   return func::AsyncResult<Result>::create(
                       [res](std::function<void(Result)> resolve,
                             std::function<void(std::string)> reject) {
                         resolve(res);
                       });
                 })
          .catch_([dispatch, rejected](std::string err) {
            dispatch(rejected(FString(UTF8_TO_TCHAR(err.c_str()))));
          });
    };
  };

  return AsyncThunkConfig<Result, Arg, State>{TypePrefix, pending, fulfilled,
                                              rejected, thunkActionCreator};
}

// ==========================================
// Phase 5: Middleware
// ==========================================

template <typename State> struct MiddlewareApi {
  std::function<AnyAction(const AnyAction &)> dispatch;
  std::function<State()> getState;
};

// Dispatch function type used throughout the middleware chain.
using Dispatcher = std::function<AnyAction(const AnyAction &)>;

template <typename State>
using Middleware =
    std::function<std::function<Dispatcher(Dispatcher)>(
        const MiddlewareApi<State> &)>;

template <typename State>
Dispatcher
applyMiddleware(Dispatcher baseDispatch,
                std::function<State()> getState,
                const std::vector<Middleware<State>> &middlewares) {
  // Use shared_ptr so middleware closures can reference the final enhanced
  // dispatch through indirection, matching RTK's actual behavior.
  auto enhancedDispatch = std::make_shared<Dispatcher>(baseDispatch);

  auto api = MiddlewareApi<State>{
      // Trampoline: always calls through the fully composed chain
      [enhancedDispatch](const AnyAction &action) -> AnyAction {
        return (*enhancedDispatch)(action);
      },
      getState};

  Dispatcher currentDispatch = baseDispatch;

  // Compose in reverse so that the first middleware in the vector wraps the
  // later ones
  for (auto it = middlewares.rbegin(); it != middlewares.rend(); ++it) {
    currentDispatch = (*it)(api)(currentDispatch);
  }

  // Point the shared dispatch at the fully composed chain
  *enhancedDispatch = currentDispatch;

  return currentDispatch;
}

template <typename State> struct ListenerMiddleware {
  using EffectCallback = std::function<void(const AnyAction &action,
                                            const MiddlewareApi<State> &api)>;

  TMap<FString, TArray<EffectCallback>> listeners;

  void addListener(const FString &actionType, EffectCallback effect) {
    listeners.FindOrAdd(actionType).Add(effect);
  }

  Middleware<State> getMiddleware() const {
    // Capture listeners by value to avoid lifetime dependence on this
    auto listenersCopy = listeners;
    return [listenersCopy](const MiddlewareApi<State> &api)
               -> std::function<Dispatcher(Dispatcher)> {
      return [listenersCopy, api](Dispatcher next) -> Dispatcher {
        return [listenersCopy, api, next](const AnyAction &action) -> AnyAction {
          // Propagate to reducers first
          auto resultAction = next(action);

          // Then execute listener effects
          if (const auto *activeListeners = listenersCopy.Find(action.Type)) {
            for (const auto &effect : *activeListeners) {
              effect(action, api);
            }
          }

          return resultAction;
        };
      };
    };
  }
};

// ==========================================
// Phase 6: Selectors
// ==========================================

namespace detail {
template <typename Result, typename State, typename Combiner,
          typename InputTuple, size_t... Is>
Result evaluateSelector(const State &state, Combiner &combiner,
                        const InputTuple &inputs, func::seq<Is...>) {
  return combiner(std::get<Is>(inputs)(state)...);
}
} // namespace detail

template <typename State, typename Result, typename... InSelectors>
std::function<Result(const State &)> createSelector(
    const std::tuple<InSelectors...> &inputSelectors,
    std::function<
        Result(decltype(std::declval<InSelectors>()(
            std::declval<const State &>()))...)>
        combiner) {
  // Memoize the combiner using FP core's memoizeLast.
  // It will automatically track the tuple of inputs across calls.
  auto memoizedCombiner = func::memoizeLast(combiner);

  return
      [inputSelectors, memoizedCombiner](const State &state) mutable -> Result {
        return detail::evaluateSelector<Result>(
            state, memoizedCombiner, inputSelectors,
            func::gen_seq<sizeof...(InSelectors)>());
      };
}

// ==========================================
// Phase 7: RTK Query Equivalent (API Slice)
// ==========================================

struct FApiEndpointTag {
  FString Type;
  FString Id;
};

template <typename Arg, typename Result> struct ApiEndpoint {
  FString EndpointName;
  TArray<FApiEndpointTag> ProvidesTags;
  TArray<FApiEndpointTag> InvalidatesTags;

  // Abstract request builder/executor
  std::function<func::AsyncResult<func::HttpResult<Result>>(const Arg &)>
      RequestBuilder;
};

// Simplified dynamic slice registry mapped by string path
template <typename State> struct ApiSlice {
  FString ReducerPath;
  TArray<FString> TagTypes;

  // We store standard AsyncThunk configs under the hood
  // so we get pending/fulfilled/rejected out-of-the-box
  // Note: True RTK Query has explicit "Endpoints" that bundle query/mutation
  // behavior, but at the base level they map directly to thunks.

  template <typename Arg, typename Result>
  AsyncThunkConfig<Result, Arg, State>
  injectEndpoint(const ApiEndpoint<Arg, Result> &EndpointDesc) {
    FString ThunkPrefix = ReducerPath + TEXT("/") + EndpointDesc.EndpointName;

    return createAsyncThunk<Result, Arg, State>(
        ThunkPrefix,
        [EndpointDesc](const Arg &arg, const ThunkApi<State> &api)
            -> func::AsyncResult<Result> {
          // Execute the provided HTTP Request
          return func::AsyncChain::then<func::HttpResult<Result>, Result>(
              EndpointDesc.RequestBuilder(arg),
              [](func::HttpResult<Result> httpRes) {
                // RTK Query unwraps the payload on success or rejects on
                // HTTP failure
                if (httpRes.bSuccess) {
                  return func::AsyncResult<Result>::create(
                      [httpRes](std::function<void(Result)> Resolve,
                                std::function<void(std::string)> Reject) {
                        Resolve(httpRes.data);
                      });
                } else {
                  return func::AsyncResult<Result>::create(
                      [httpRes](std::function<void(Result)> Resolve,
                                std::function<void(std::string)> Reject) {
                        Reject(httpRes.error);
                      });
                }
              });
        });
  }
};

// ==========================================
// Phase 8: Store Configuration
// ==========================================

template <typename State> struct EnhancedStore {
  std::shared_ptr<Store<State>> CoreStore;
  Dispatcher Dispatch;

  const State &getState() const { return CoreStore->getState(); }

  std::function<void()> subscribe(std::function<void()> Callback) {
    return CoreStore->subscribe(std::move(Callback));
  }

  AnyAction dispatch(const AnyAction &action) const { return Dispatch(action); }

  template <typename Result>
  func::AsyncResult<Result>
  dispatch(const ThunkAction<Result, State> &thunk) const {
    auto dispatchAny = Dispatch;
    auto getState = [this]() -> State { return CoreStore->getState(); };
    return thunk(dispatchAny, getState);
  }
};

template <typename State>
EnhancedStore<State>
configureStore(CaseReducer<State> rootReducer, State preloadedState,
               const std::vector<Middleware<State>> &middlewares) {
  EnhancedStore<State> enhanced;
  auto coreStore = std::make_shared<Store<State>>(std::move(preloadedState),
                                                  std::move(rootReducer));
  enhanced.CoreStore = coreStore;

  Dispatcher coreDispatch = [coreStore](const AnyAction &action) -> AnyAction {
    return coreStore->dispatch(action);
  };

  auto getState = [coreStore]() -> State { return coreStore->getState(); };

  enhanced.Dispatch =
      applyMiddleware<State>(coreDispatch, getState, middlewares);

  return enhanced;
}

template <typename State>
EnhancedStore<State> configureStore(CaseReducer<State> rootReducer,
                                    State preloadedState) {
  return configureStore<State>(rootReducer, preloadedState,
                               std::vector<Middleware<State>>());
}

} // namespace rtk

#endif // RTK_HPP
