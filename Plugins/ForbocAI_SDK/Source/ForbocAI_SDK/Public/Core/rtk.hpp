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

/**
 * 1.1 Action<Payload>
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename Payload> struct Action {
  FString Type;
  Payload PayloadValue;
};

struct FEmptyPayload {};

/**
 * Builds a typed action payload envelope.
 * User Story: As reducer and thunk code, I need a typed action envelope so
 * payload-bearing actions move through the store with explicit shape.
 */
template <typename Payload>
Action<Payload> makeAction(const FString &Type, const Payload &PayloadValue) {
  return Action<Payload>{Type, PayloadValue};
}

/**
 * Builds an action that carries no payload data.
 * User Story: As reducer and thunk code, I need an empty-payload action shape
 * so simple lifecycle actions can dispatch without custom structs.
 */
inline Action<FEmptyPayload> makeAction(const FString &Type) {
  return Action<FEmptyPayload>{Type, FEmptyPayload{}};
}

/**
 * Type-erased envelope for heterogeneous root dispatch
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
struct AnyAction {
  FString Type;
  std::shared_ptr<void> PayloadWrapper;

  /**
   * Constructs an empty type-erased action envelope.
   * User Story: As root dispatch infrastructure, I need a default AnyAction so
   * containers and return paths can be initialized before payload assignment.
   */
  AnyAction() {}

  /**
   * Constructs a type-erased action envelope from a type tag and payload wrapper.
   * User Story: As root dispatch infrastructure, I need a type-erased action
   * constructor so heterogeneous payloads can move through one dispatch channel.
   */
  AnyAction(const FString &InType, std::shared_ptr<void> InPayloadWrapper)
      : Type(InType),
        PayloadWrapper(std::move(InPayloadWrapper)) {}

  /**
   * Extracts a typed payload from the type-erased storage.
   * User Story: As root dispatch consumers, I need a direct payload accessor so
   * infrastructure code can recover stored action data when type ownership is known.
   * Warning: This performs an unchecked static_cast. Callers must ensure the requested
   * payload type matches the stored value. Prefer ActionCreator::extract() when possible.
   */
  template <typename Payload> func::Maybe<Payload> getPayload() const {
    if (!PayloadWrapper) {
      return func::nothing<Payload>();
    }
    return func::just(*static_cast<Payload *>(PayloadWrapper.get()));
  }
};

/**
 * 1.2 Reducer
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */
template <typename State, typename ActionT>
using Reducer = std::function<State(const State &, const ActionT &)>;

template <typename State>
using CaseReducer = std::function<State(const State &, const AnyAction &)>;

/**
 * 1.3 Store
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename State> struct Store {
  State CurrentState;
  CaseReducer<State> RootReducer;

  struct Subscriber {
    int64_t Id;
    std::function<void()> Callback;
  };
  std::vector<Subscriber> Subscribers;
  int64_t NextId = 1;

  /**
   * Constructs a store from initial state and a root reducer.
   * User Story: As runtime bootstrapping, I need a store constructor so a root
   * reducer and its initial state can start handling dispatched actions.
   */
  Store(State InitialState, CaseReducer<State> ReducerFunc)
      : CurrentState(std::move(InitialState)),
        RootReducer(std::move(ReducerFunc)) {}

  /**
   * Returns the current store state snapshot.
   * User Story: As store consumers, I need the current state exposed so
   * dispatchers, selectors, and subscribers can inspect runtime data.
   */
  const State &getState() const { return CurrentState; }

  /**
   * Applies an action and notifies subscribers after the reducer runs.
   * User Story: As store consumers, I need dispatch to update state and notify
   * listeners so runtime flows can react to reducer changes.
   */
  AnyAction dispatch(const AnyAction &action) {
    CurrentState = RootReducer(CurrentState, action);
    /**
     * Copy subscribers so list is stable if unsubscribed during callback
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
    auto SubsCopy = Subscribers;
    for (const auto &sub : SubsCopy) {
      sub.Callback();
    }
    return action;
  }

  /**
   * Registers a callback and returns an unsubscribe function.
   * User Story: As store consumers, I need subscriptions with unsubscribe
   * handles so runtime code can observe state safely.
   */
  std::function<void()> subscribe(std::function<void()> Callback) {
    int64_t Id = NextId++;
    Subscribers.push_back({Id, std::move(Callback)});

    /**
     * Return unsubscribe callable Handle
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
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

/**
 * 1.4 combineReducers
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename RootState> class ReducerCombiner {
  std::vector<
      std::function<bool(RootState &, const RootState &, const AnyAction &)>>
      bindings;

public:
  /**
   * Adds a reducer binding for a specific slice member on the root state.
   * User Story: As root-store assembly, I need slice bindings declared
   * fluently so reducers can be composed without boilerplate glue.
   */
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

  /**
   * Produces a root reducer that fans an action out to every bound slice.
   * User Story: As root-store assembly, I need one reducer combiner output so
   * bound slices receive the same dispatched action coherently.
   */
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

/**
 * Creates an empty reducer combiner for a root state.
 * User Story: As root-store assembly, I need a reducer combiner entry point so
 * slices can be registered incrementally.
 */
template <typename RootState> ReducerCombiner<RootState> combineReducers() {
  return ReducerCombiner<RootState>();
}

/**
 * 2.1 createAction<P> and Matchers
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename Payload> struct ActionCreator {
  FString Type;

  AnyAction operator()(const Payload &payload) const {
    return AnyAction(Type, std::make_shared<Payload>(payload));
  }

  /**
   * Reports whether an AnyAction matches this creator's type tag.
   * User Story: As reducer helpers, I need action-type matching so handlers can
   * confirm payload shape before extraction.
   */
  bool match(const AnyAction &action) const { return action.Type == Type; }

  /**
   * Extracts a typed payload when the action type matches this creator.
   * User Story: As reducer helpers, I need safe payload extraction so typed
   * reducers can consume AnyAction without repeating casts.
   */
  func::Maybe<Payload> extract(const AnyAction &action) const {
    if (action.Type == Type && action.PayloadWrapper) {
      return func::just(*static_cast<Payload *>(action.PayloadWrapper.get()));
    }
    return func::nothing<Payload>();
  }
};

/**
 * Creates a typed action creator for a namespaced action type.
 * User Story: As slice code, I need typed action creators so reducers and
 * thunks can share stable action contracts.
 */
template <typename Payload>
ActionCreator<Payload> createAction(const FString &Type) {
  return ActionCreator<Payload>{Type};
}

struct EmptyActionCreator {
  FString Type;

  AnyAction operator()() const {
    return AnyAction(Type, std::make_shared<FEmptyPayload>());
  }

  /**
   * Reports whether an AnyAction matches this empty action creator.
   * User Story: As reducer helpers, I need empty-action matching so lifecycle
   * actions can be recognized without custom payload structs.
   */
  bool match(const AnyAction &action) const { return action.Type == Type; }
};

/**
 * Creates an empty-payload action creator for a namespaced action type.
 * User Story: As slice code, I need empty action creators so simple lifecycle
 * events can reuse the same RTK-style creation pattern.
 */
inline EmptyActionCreator createAction(const FString &Type) {
  return EmptyActionCreator{Type};
}

/**
 * 2.2 Slice<State>
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename State> struct Slice {
  FString Name;
  CaseReducer<State> Reducer;
};

/**
 * 2.3 SliceBuilder for generating slices and binding action creators locally
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename State> class SliceBuilder {
  FString Name;
  State InitialState;
  TMap<FString, CaseReducer<State>> Reducers;

public:
  /**
   * Constructs a slice builder with a name and initial state.
   * User Story: As slice authors, I need a builder constructor so slice naming
   * and initial state are defined before reducer cases are registered.
   */
  SliceBuilder(FString InName, State InInitialState)
      : Name(MoveTemp(InName)), InitialState(MoveTemp(InInitialState)) {}

  /**
   * Registers a typed reducer case and returns its local action creator.
   * User Story: As slice authors, I need local case registration so reducers
   * and action creators can be declared together.
   */
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

  /**
   * Registers an empty-payload reducer case and returns its action creator.
   * User Story: As slice authors, I need empty case registration so lifecycle
   * reducers can be declared without custom payload types.
   */
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

  /**
   * Binds an externally created typed action to this slice.
   * User Story: As slice authors, I need externally owned actions bindable into
   * a slice so cross-slice lifecycle handling stays ergonomic.
   */
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

  /**
   * Binds an externally created empty action to this slice.
   * User Story: As slice authors, I need external empty actions bindable into a
   * slice so lifecycle wiring stays consistent across modules.
   */
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

  /**
   * Finalizes the slice and its reducer lookup table.
   * User Story: As slice authors, I need a final build step so the slice can be
   * exported with its reducer map intact.
   */
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

/**
 * Phase 3: Entity Adapter
 * User Story: As a maintainer, I need this implementation note so I can understand which milestone behavior the surrounding code is preserving.
 */

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

  /**
   * Returns an empty entity-state container.
   * User Story: As entity-backed slices, I need a canonical empty entity state
   * so adapters can initialize predictable reducer storage.
   */
  EntityState<T> getInitialState() const { return EntityState<T>{{}, {}}; }

  /**
   * Adds a single entity when its id is not already present.
   * User Story: As entity-backed slices, I need single-entity insertion so new
   * records can be added without mutating existing adapter state.
   */
  EntityState<T> addOne(const EntityState<T> &state, const T &entity) const {
    EntityState<T> next = state;
    FString id = selectId(entity);
    if (!next.entities.Find(id)) {
      next.ids.Add(id);
      next.entities.Add(id, entity);
    }
    return next;
  }

  /**
   * Adds each missing entity from a batch without replacing existing entries.
   * User Story: As entity-backed slices, I need batch insertion so collections
   * can be seeded while preserving existing records.
   */
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

  /**
   * Inserts or replaces a single entity by id.
   * User Story: As entity-backed slices, I need single-entity replacement so
   * reducers can upsert records deterministically.
   */
  EntityState<T> setOne(const EntityState<T> &state, const T &entity) const {
    EntityState<T> next = state;
    FString id = selectId(entity);
    if (!next.entities.Find(id)) {
      next.ids.Add(id);
    }
    next.entities.Add(id, entity);
    return next;
  }

  /**
   * Replaces the full entity set with the provided collection.
   * User Story: As entity-backed slices, I need whole-collection replacement so
   * reducers can resync adapter state from remote payloads.
   */
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

  /**
   * Upserts a single entity by delegating to setOne.
   * User Story: As entity-backed slices, I need a semantic upsert helper so
   * reducers can express intent without duplicating adapter logic.
   */
  EntityState<T> upsertOne(const EntityState<T> &state, const T &entity) const {
    return setOne(state, entity);
  }

  /**
   * Upserts a batch of entities by id.
   * User Story: As entity-backed slices, I need batch upsert so synced payloads
   * can merge into adapter state efficiently.
   */
  EntityState<T> upsertMany(const EntityState<T> &state,
                            const TArray<T> &entitiesToUpsert) const {
    EntityState<T> next = state;
    for (const auto &entity : entitiesToUpsert) {
      next = setOne(next, entity);
    }
    return next;
  }

  /**
   * Removes a single entity and its id when present.
   * User Story: As entity-backed slices, I need record removal so deleted items
   * disappear from both entity maps and id orderings.
   */
  EntityState<T> removeOne(const EntityState<T> &state,
                           const FString &id) const {
    EntityState<T> next = state;
    if (next.entities.Remove(id) > 0) {
      next.ids.Remove(id);
    }
    return next;
  }

  /**
   * Removes all entities whose ids appear in the supplied list.
   * User Story: As entity-backed slices, I need batch removal so reducers can
   * clear multiple records in one pure operation.
   */
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

  /**
   * Clears every entity from the adapter state.
   * User Story: As entity-backed slices, I need a reset helper so adapters can
   * return to a clean initial state predictably.
   */
  EntityState<T> removeAll(const EntityState<T> &) const {
    return getInitialState();
  }

  /**
   * Replaces one entity with the result of a patch function.
   * User Story: As entity-backed slices, I need targeted patching so one record
   * can be updated without rebuilding the full collection manually.
   */
  EntityState<T> updateOne(const EntityState<T> &state, const FString &id,
                           std::function<T(const T &)> patch) const {
    EntityState<T> next = state;
    if (const T *existing = next.entities.Find(id)) {
      next.entities.Add(id, patch(*existing));
    }
    return next;
  }

  /**
   * Builds selector helpers for the current adapter shape.
   * User Story: As entity-backed slices, I need selector helpers so callers can
   * read ids, entities, and totals without hand-rolled lookup code.
   */
  EntitySelectors<T> getSelectors() const {
    const auto SelectAll = [](const EntityState<T> &state) -> TArray<T> {
      TArray<T> result;
      for (const auto &id : state.ids) {
        if (const T *ent = state.entities.Find(id)) {
          result.Add(*ent);
        }
      }
      return result;
    };

    const auto SelectById =
        [](const EntityState<T> &state, const FString &id) -> func::Maybe<T> {
      if (const T *ent = state.entities.Find(id)) {
        return func::just(*ent);
      }
      return func::nothing<T>();
    };

    const auto SelectIds = [](const EntityState<T> &state) -> TArray<FString> {
      return state.ids;
    };

    const auto SelectTotal = [](const EntityState<T> &state) -> int32_t {
      return state.ids.Num();
    };

    return EntitySelectors<T>{
        SelectAll, SelectById, SelectIds, SelectTotal};
  }
};

/**
 * Creates entity-adapter operations from an id selector.
 * User Story: As slice authors, I need adapter factories so entity state
 * management can be generated from one id-selection rule.
 */
template <typename T>
EntityAdapterOps<T>
createEntityAdapter(std::function<FString(const T &)> selectId) {
  return EntityAdapterOps<T>{std::move(selectId)};
}

/**
 * Phase 4: Async Thunks
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

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

/**
 * Creates a thunk config with pending, fulfilled, and rejected lifecycle actions.
 * User Story: As async thunk authors, I need lifecycle action wiring generated
 * automatically so pending and result dispatch stay consistent.
 */
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
      /**
       * 1. Dispatch pending synchronously
       * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
       */
      dispatch(pending(arg));

      /**
       * 2. Build the ThunkApi surface
       * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
       */
      ThunkApi<State> api{dispatch, getState};

      /**
       * 3. Execute payload creator
       * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
       */
      auto result = PayloadCreator(arg, api);

      /**
       * 4. Chain lifecycle actions using FP core AsyncChain.
       *    The returned AsyncResult carries both the fulfilled chain and
       *    the catch_ error handler. The caller is responsible for calling
       *    .execute() on the returned result to trigger the full chain.
       * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
       */
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

/**
 * Phase 5: Middleware
 * User Story: As a maintainer, I need this implementation note so I can understand which milestone behavior the surrounding code is preserving.
 */

template <typename State> struct MiddlewareApi {
  std::function<AnyAction(const AnyAction &)> dispatch;
  std::function<State()> getState;
};

/**
 * Dispatch function type used throughout the middleware chain.
 * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
 */
using Dispatcher = std::function<AnyAction(const AnyAction &)>;

template <typename State>
using Middleware =
    std::function<std::function<Dispatcher(Dispatcher)>(
        const MiddlewareApi<State> &)>;

/**
 * Wraps a dispatcher with middleware while preserving RTK-style composition order.
 * User Story: As store configuration, I need middleware composition so dispatch
 * can be enhanced without changing reducer semantics.
 */
template <typename State>
Dispatcher
applyMiddleware(Dispatcher baseDispatch,
                std::function<State()> getState,
                const std::vector<Middleware<State>> &middlewares) {
  /**
   * Use shared_ptr so middleware closures can reference the final enhanced
   * dispatch through indirection, matching RTK's actual behavior.
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  auto enhancedDispatch = std::make_shared<Dispatcher>(baseDispatch);

  auto api = MiddlewareApi<State>{
      /**
       * Trampoline: always calls through the fully composed chain
       * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
       */
      [enhancedDispatch](const AnyAction &action) -> AnyAction {
        return (*enhancedDispatch)(action);
      },
      getState};

  Dispatcher currentDispatch = baseDispatch;

  /**
   * Compose in reverse so that the first middleware in the vector wraps the
   * later ones
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  for (auto it = middlewares.rbegin(); it != middlewares.rend(); ++it) {
    currentDispatch = (*it)(api)(currentDispatch);
  }

  /**
   * Point the shared dispatch at the fully composed chain
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  *enhancedDispatch = currentDispatch;

  return currentDispatch;
}

template <typename State> struct ListenerMiddleware {
  using EffectCallback = std::function<void(const AnyAction &action,
                                            const MiddlewareApi<State> &api)>;

  TMap<FString, TArray<EffectCallback>> listeners;

  /**
   * Registers a side-effect callback for a specific action type.
   * User Story: As listener middleware users, I need type-scoped listeners so
   * side effects can subscribe to reducer events declaratively.
   */
  void addListener(const FString &actionType, EffectCallback effect) {
    listeners.FindOrAdd(actionType).Add(effect);
  }

  /**
   * Materializes middleware that executes listeners after reducers run.
   * User Story: As listener middleware users, I need middleware output so
   * listeners can run after state updates with access to dispatch and state.
   */
  Middleware<State> getMiddleware() const {
    /**
     * Capture listeners by value to avoid lifetime dependence on this
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
    auto listenersCopy = listeners;
    return [listenersCopy](const MiddlewareApi<State> &api)
               -> std::function<Dispatcher(Dispatcher)> {
      return [listenersCopy, api](Dispatcher next) -> Dispatcher {
        return [listenersCopy, api, next](const AnyAction &action) -> AnyAction {
          /**
           * Propagate to reducers first
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
          auto resultAction = next(action);

          /**
           * Then execute listener effects
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
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

/**
 * Phase 6: Selectors
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

namespace detail {
/**
 * Evaluates a selector combiner against each input selector in a tuple.
 * User Story: As memoized selectors, I need tuple-driven combiner evaluation
 * so composed selectors can reuse one generic implementation.
 */
template <typename Result, typename State, typename Combiner,
          typename InputTuple, size_t... Is>
Result evaluateSelector(const State &state, Combiner &combiner,
                        const InputTuple &inputs, func::seq<Is...>) {
  return combiner(std::get<Is>(inputs)(state)...);
}
} // namespace detail

/**
 * Creates a memoized selector from input selectors and a combiner.
 * User Story: As selector authors, I need memoized selector composition so
 * derived state only recomputes when its inputs change.
 */
template <typename State, typename Result, typename... InSelectors>
std::function<Result(const State &)> createSelector(
    const std::tuple<InSelectors...> &inputSelectors,
    std::function<
        Result(decltype(std::declval<InSelectors>()(
            std::declval<const State &>()))...)>
        combiner) {
  /**
   * Memoize the combiner using FP core's memoizeLast.
   * It will automatically track the tuple of inputs across calls.
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  auto memoizedCombiner = func::memoizeLast(combiner);

  return
      [inputSelectors, memoizedCombiner](const State &state) mutable -> Result {
        return detail::evaluateSelector<Result>(
            state, memoizedCombiner, inputSelectors,
            func::gen_seq<sizeof...(InSelectors)>());
      };
}

/**
 * Phase 7: RTK Query Equivalent (API Slice)
 * User Story: As a maintainer, I need this implementation note so I can understand which milestone behavior the surrounding code is preserving.
 */

struct FApiEndpointTag {
  FString Type;
  FString Id;
};

template <typename Arg, typename Result> struct ApiEndpoint {
  FString EndpointName;
  TArray<FApiEndpointTag> ProvidesTags;
  TArray<FApiEndpointTag> InvalidatesTags;

  /**
   * Abstract request builder/executor
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  std::function<func::AsyncResult<func::HttpResult<Result>>(const Arg &)>
      RequestBuilder;
};

/**
 * Simplified dynamic slice registry mapped by string path
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename State> struct ApiSlice {
  FString ReducerPath;
  TArray<FString> TagTypes;

  /**
   * Injects an endpoint as an async thunk under this API slice's reducer path.
   * User Story: As API-slice authors, I need endpoint injection so network
   * operations inherit standard thunk lifecycle behavior automatically.
   */
  template <typename Arg, typename Result>
  AsyncThunkConfig<Result, Arg, State>
  injectEndpoint(const ApiEndpoint<Arg, Result> &EndpointDesc) {
    FString ThunkPrefix = ReducerPath + TEXT("/") + EndpointDesc.EndpointName;

    return createAsyncThunk<Result, Arg, State>(
        ThunkPrefix,
        [EndpointDesc](const Arg &arg, const ThunkApi<State> &api)
            -> func::AsyncResult<Result> {
          /**
           * Execute the provided HTTP Request
           * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
           */
          return func::AsyncChain::then<func::HttpResult<Result>, Result>(
              EndpointDesc.RequestBuilder(arg),
              [](func::HttpResult<Result> httpRes) {
                /**
                 * RTK Query unwraps the payload on success or rejects on
                 * HTTP failure
                 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
                 */
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

/**
 * Phase 8: Store Configuration
 * User Story: As a maintainer, I need this implementation note so I can understand which milestone behavior the surrounding code is preserving.
 */

template <typename State> struct EnhancedStore {
  std::shared_ptr<Store<State>> CoreStore;
  Dispatcher Dispatch;

  /**
   * Returns the current root-state snapshot.
   * User Story: As enhanced-store consumers, I need root-state access so
   * middleware, thunks, and selectors can inspect current runtime data.
   */
  const State &getState() const { return CoreStore->getState(); }

  /**
   * Registers a subscriber on the underlying core store.
   * User Story: As enhanced-store consumers, I need subscription support so
   * callers can react to state changes after middleware composition.
   */
  std::function<void()> subscribe(std::function<void()> Callback) {
    return CoreStore->subscribe(std::move(Callback));
  }

  /**
   * Dispatches a plain AnyAction through the enhanced middleware chain.
   * User Story: As enhanced-store consumers, I need action dispatch routed
   * through middleware so store behavior matches RTK-style semantics.
   */
  AnyAction dispatch(const AnyAction &action) const { return Dispatch(action); }

  /**
   * Dispatches a thunk using the enhanced dispatch and current getState accessors.
   * User Story: As thunk callers, I need thunk dispatch integrated with the
   * enhanced store so async flows can reuse middleware and state access.
   */
  template <typename Result>
  func::AsyncResult<Result>
  dispatch(const ThunkAction<Result, State> &thunk) const {
    auto dispatchAny = Dispatch;
    auto getState = [this]() -> State { return CoreStore->getState(); };
    return thunk(dispatchAny, getState);
  }
};

/**
 * Configures an enhanced store with middleware and preloaded state.
 * User Story: As runtime bootstrapping, I need a configured enhanced store so
 * reducers, middleware, and preload state come together in one helper.
 */
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

/**
 * Configures an enhanced store without middleware.
 * User Story: As simple runtime bootstrapping, I need a no-middleware overload
 * so tests and small setups can create stores with less ceremony.
 */
template <typename State>
EnhancedStore<State> configureStore(CaseReducer<State> rootReducer,
                                    State preloadedState) {
  return configureStore<State>(rootReducer, preloadedState,
                               std::vector<Middleware<State>>());
}

} // namespace rtk

#endif // RTK_HPP
