# Functional C++11 Guide

`Codex // Functional_Paradigm`

> A guide to functional programming in **strict C++11** for the ForbocAI SDK.
> All code targets C++11 only -- no C++14/17 features.
>
> **Library**: This guide accompanies [`functional_core.hpp`](Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp), which provides production-ready implementations of Maybe, Either, Currying, Lazy evaluation, Pipeline, Composition, and fmap.

---

## Core Principles

| OOP (avoid)               | FP (prefer)                          |
|---------------------------|--------------------------------------|
| `class` with methods      | `struct` with public `const` data    |
| Constructors              | **Factory functions** in namespaces  |
| `this->mutate()`          | Return a **new value**               |
| Inheritance / virtual     | Higher-order functions / composition |
| `void` methods            | Pure functions that **always return**  |
| `for` loops with mutation | `std::transform` / `std::accumulate` |
| `new` / `delete`          | Stack values or `std::shared_ptr`    |

---

## 1. Data vs. Behavior (Structs, not Classes)

In OOP, you encapsulate data and methods. In FP, you **separate** them. Use `struct` purely as data carriers and `namespace` to group the functions that operate on them.

**The Rule:** Structs have no methods (except constructors for convenience). All members should ideally be `const`. All logic lives in free functions inside namespaces.

```cpp
#include <string>
#include <vector>

// Pure Data — no logic, no methods
struct Player {
    const std::string name;
    const int hp;
    const int x;
    const int y;
};

// Pure Data
struct WorldState {
    const std::vector<Player> players;
    const int turnCount;
};
```

---

## 2. The Factory Pattern (Replaces Constructors)

Instead of class constructors, use **factory functions** that return instances. Factories separate creation logic (validation, defaults, ID generation) from the data layout. This is the primary construction pattern in the ForbocAI SDK.

```cpp
// Factory namespace — replaces class constructors entirely
namespace PlayerFactory {
    // Pure factory function: inputs -> Player
    Player create(const std::string& name, int startX, int startY) {
        // Validation, defaults, and computation happen here
        return Player{name, 100, startX, startY};
    }

    // Factory from serialized data
    Player fromJson(const std::string& json) {
        // Parse and return — no class needed
        return Player{"Parsed", 100, 0, 0};
    }
}

// Usage: always construct through factories
auto hero = PlayerFactory::create("Hero", 0, 0);
```

**Why factories over constructors?**
- Logic is testable independently of data layout
- Multiple named factories for different creation paths (`create`, `fromJson`, `fromSave`)
- No hidden `this` pointer -- all inputs and outputs are explicit
- Composable with other functions (a factory IS a function)

---

## 3. Immutability & Transformations (Copy-on-Write)

In pure FP, you never mutate state -- you return a **new** value. In C++11, Move Semantics and Return Value Optimization (RVO) make this performant.

```cpp
// Operations namespace — stateless pure functions
namespace PlayerOps {
    // Pure: old Player -> new Player (no side effects)
    Player move(const Player& p, int dx, int dy) {
        return Player{p.name, p.hp, p.x + dx, p.y + dy};
    }

    Player takeDamage(const Player& p, int amount) {
        int newHp = (p.hp - amount < 0) ? 0 : p.hp - amount;
        return Player{p.name, newHp, p.x, p.y};
    }

    Player heal(const Player& p, int amount) {
        int newHp = (p.hp + amount > 100) ? 100 : p.hp + amount;
        return Player{p.name, newHp, p.x, p.y};
    }
}

// Original is never modified
auto p1 = PlayerFactory::create("Hero", 0, 0);
auto p2 = PlayerOps::move(p1, 10, 0);       // p1 unchanged
auto p3 = PlayerOps::takeDamage(p2, 25);     // p2 unchanged
```

---

## 4. Higher-Order Functions (HOF)

C++11's `std::function` and lambdas allow functions to accept and return other functions. This replaces the Strategy Pattern, Observer Pattern, and similar OOP indirection.

```cpp
#include <functional>

// HOF: applies any effect to a player
Player applyEffect(const Player& p, std::function<Player(const Player&)> effect) {
    return effect(p);
}

int main() {
    auto p1 = PlayerFactory::create("Hero", 0, 0);

    // Lambda as a pure function
    auto poison = [](const Player& p) {
        return PlayerOps::takeDamage(p, 5);
    };

    // Chaining transformations (immutably)
    auto p2 = applyEffect(p1, poison);
    auto p3 = PlayerOps::move(p2, 10, 0);

    // p1 is untouched — immutability preserved
}
```

---

## 5. Maybe Monad (Optional Values)

`Maybe<T>` from `functional_core.hpp` represents a value that may or may not exist. Use the `Just()` and `Nothing()` **factory functions** -- never construct directly.

```cpp
#include "Core/functional_core.hpp"
using func::Maybe;

// --- Factory construction ---
auto health = Maybe<int>::Just(100);
auto noHealth = Maybe<int>::Nothing();

// --- Transform with map (functor) ---
auto doubled = health.map([](int hp) { return hp * 2; });
// doubled == Just(200)

auto nope = noHealth.map([](int hp) { return hp * 2; });
// nope == Nothing (transformation skipped safely)

// --- Chain with bind (monad / flatMap) ---
auto safeDivide = [](int x) -> Maybe<int> {
    if (x == 0) return Maybe<int>::Nothing();
    return Maybe<int>::Just(100 / x);
};
auto result = health.bind(safeDivide);  // Just(1)
auto fail   = Maybe<int>::Just(0).bind(safeDivide);  // Nothing

// --- Extract with default ---
int hp   = health.orElse(0);     // 100
int noHp = noHealth.orElse(0);   // 0
```

**When to use Maybe:**
- Function that might not find a result (lookup, search)
- Optional configuration values
- Replacing null pointers with a safe algebraic type

---

## 6. Either Monad (Error Handling)

`Either<E, T>` replaces exceptions and error codes. Convention: **Left = error**, **Right = success**. Use factory functions only.

```cpp
#include "Core/functional_core.hpp"
using func::Either;

typedef Either<std::string, Player> PlayerResult;

// --- Factory construction ---
auto ok  = PlayerResult::Right(PlayerFactory::create("Hero", 0, 0));
auto err = PlayerResult::Left(std::string("Invalid name"));

// --- Transform the success value ---
auto moved = ok.map([](const Player& p) {
    return PlayerOps::move(p, 5, 0);
});
// moved == Right(Player at x=5)

auto stillErr = err.map([](const Player& p) {
    return PlayerOps::move(p, 5, 0);
});
// stillErr == Left("Invalid name") — error propagates automatically

// --- Chain operations that can fail ---
auto validateAndMove = [](const Player& p) -> PlayerResult {
    if (p.hp <= 0)
        return PlayerResult::Left(std::string("Dead players cannot move"));
    return PlayerResult::Right(PlayerOps::move(p, 1, 0));
};
auto result = ok.bind(validateAndMove);
```

**When to use Either:**
- Operations that can fail with a reason (API calls, validation)
- Replacing try/catch with composable error propagation
- Chaining fallible steps where early failure short-circuits

---

## 7. Currying & Partial Application

`curry<N>()` from `functional_core.hpp` converts an N-argument function into a chain of single-argument calls. This enables **partial application** -- fixing some arguments to create specialized functions.

```cpp
#include "Core/functional_core.hpp"

// --- Using the curry<N>() factory function ---
auto damage = [](int base, float multiplier, const Player& p) {
    int amount = static_cast<int>(base * multiplier);
    return PlayerOps::takeDamage(p, amount);
};

auto curriedDamage = func::curry<3>(damage);

// Partial application: fix base and multiplier
auto fireDamage = curriedDamage(20)(1.5f);   // base=20, mult=1.5
auto iceDamage  = curriedDamage(15)(0.8f);   // base=15, mult=0.8

// Apply to players
auto p1 = PlayerFactory::create("Mage", 0, 0);
auto p2 = fireDamage(p1);   // 30 fire damage
auto p3 = iceDamage(p1);    // 12 ice damage
```

**Manual partial application** (without the library) uses capturing lambdas:

```cpp
// Factory that returns a function (partial application by hand)
std::function<Player(const Player&)> makeDamageEffect(int amount) {
    return [amount](const Player& p) {
        return PlayerOps::takeDamage(p, amount);
    };
}

auto heavyHit = makeDamageEffect(50);
auto p4 = heavyHit(p1);
```

---

## 8. Lazy Evaluation

`Lazy<T>` defers a computation until its result is needed, then caches it. Use the `lazy()` **factory function**.

```cpp
#include "Core/functional_core.hpp"

// Nothing runs yet — the thunk is stored, not evaluated
auto expensiveState = func::lazy([]() {
    // Imagine: database query, file I/O, AI inference...
    return WorldState{{}, 0};
});

// First call evaluates the thunk and caches the result
const WorldState& state1 = expensiveState.get();

// Second call returns the cached result (no recomputation)
const WorldState& state2 = expensiveState.get();
```

**When to use Lazy:**
- Expensive computations that may not be needed
- Breaking circular initialization dependencies
- Deferring side-effect-free work until the result is consumed

> **Note**: `Lazy<T>` is not thread-safe. Use from a single thread (e.g. game thread).

---

## 9. Replacing Loops with Algorithms (Map / Reduce / fmap)

Loops rely on mutable iterators. FP uses `std::transform`, `std::accumulate`, and `func::fmap`.

**fmap (from `functional_core.hpp`):**

```cpp
#include "Core/functional_core.hpp"

std::vector<int> nums = {1, 2, 3, 4, 5};
auto doubled = func::fmap(nums, [](int x) { return x * 2; });
// doubled == {2, 4, 6, 8, 10}
```

**Map equivalent (`std::transform`):**

```cpp
#include <algorithm>

std::vector<Player> allPlayers = { /* ... */ };
std::vector<Player> movedPlayers;
movedPlayers.reserve(allPlayers.size());

std::transform(allPlayers.begin(), allPlayers.end(),
    std::back_inserter(movedPlayers),
    [](const Player& p) {
        return PlayerOps::move(p, 1, 1);
    });
```

**Fold/Reduce (`std::accumulate`):**

```cpp
#include <numeric>

int totalHp = std::accumulate(allPlayers.begin(), allPlayers.end(), 0,
    [](int sum, const Player& p) {
        return sum + p.hp;
    });
```

---

## 10. Function Composition & Pipeline

### compose (from `functional_core.hpp`)

`compose(f, g)` creates a new function `h` where `h(x) = f(g(x))`.

```cpp
#include "Core/functional_core.hpp"

auto double_it = [](int x) { return x * 2; };
auto add_one   = [](int x) { return x + 1; };

// Read right-to-left: first double, then add one
auto double_then_inc = func::compose(add_one, double_it);

int result = double_then_inc(5);  // add_one(double_it(5)) = 11
```

### Pipeline (from `functional_core.hpp`)

`Pipeline<T>` threads a value through a chain of transformations. Unlike `compose`, it reads **left-to-right** and supports **type-changing** steps (T -> U -> V).

```cpp
#include "Core/functional_core.hpp"

// Type changes through the pipeline: Player -> Player -> int -> bool
auto result = func::start_pipe(PlayerFactory::create("Mage", 0, 0))
    .pipe([](const Player& p) { return PlayerOps::move(p, 5, 5); })
    .pipe([](const Player& p) { return PlayerOps::takeDamage(p, 10); })
    .pipe([](const Player& p) { return p.hp; })        // Player -> int
    .pipe([](int hp) { return hp > 0; })                // int -> bool
    .unwrap();  // result is bool
```

---

## 11. Unreal Engine FP Patterns

The functional paradigm applies identically to Unreal Engine types (`FString`, `TArray`, `TMap`, `USTRUCT`). The ForbocAI SDK follows these patterns throughout.

### Data Structs with USTRUCT (no methods, no logic)

```cpp
// Pure immutable data — struct, not class
USTRUCT()
struct FPlayerState {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;

    UPROPERTY()
    int32 HP;

    UPROPERTY()
    TArray<FString> Inventory;

    // Default constructor only (required by UE reflection)
    FPlayerState() : HP(100) {}
};
```

### Factory Functions in Namespaces (not constructors)

```cpp
// Factory namespace — all creation logic here
namespace PlayerStateFactory {
    MYMODULE_API FPlayerState Create(const FString& Name) {
        FPlayerState State;
        State.Name = Name;
        State.HP = 100;
        return State;
    }

    MYMODULE_API FPlayerState FromSave(const FString& Json) {
        FPlayerState State;
        FJsonObjectConverter::JsonObjectStringToUStruct(Json, &State, 0, 0);
        return State;
    }
}
```

### Pure Operations in Namespaces (not member methods)

```cpp
// Operations namespace — stateless pure functions
namespace PlayerStateOps {
    MYMODULE_API FPlayerState TakeDamage(const FPlayerState& State, int32 Amount) {
        FPlayerState New = State;  // copy
        New.HP = FMath::Max(0, State.HP - Amount);
        return New;  // return new value
    }

    MYMODULE_API FPlayerState Heal(const FPlayerState& State, int32 Amount) {
        FPlayerState New = State;
        New.HP = FMath::Min(100, State.HP + Amount);
        return New;
    }

    MYMODULE_API FPlayerState AddItem(const FPlayerState& State, const FString& Item) {
        FPlayerState New = State;
        New.Inventory.AddUnique(Item);
        return New;
    }
}
```

### TArray Operations (functional style)

```cpp
// Map: apply a transformation to every element
TArray<FPlayerState> ApplyDamageToAll(
        const TArray<FPlayerState>& Players, int32 Amount) {
    TArray<FPlayerState> Result;
    Result.Reserve(Players.Num());
    for (const FPlayerState& P : Players) {
        Result.Add(PlayerStateOps::TakeDamage(P, Amount));
    }
    return Result;  // new array, original untouched
}

// Fold/Reduce: collapse to a single value
int32 TotalHP(const TArray<FPlayerState>& Players) {
    int32 Sum = 0;
    for (const FPlayerState& P : Players) {
        Sum += P.HP;
    }
    return Sum;
}

// Filter: return matching elements
TArray<FPlayerState> AliveOnly(const TArray<FPlayerState>& Players) {
    TArray<FPlayerState> Result;
    for (const FPlayerState& P : Players) {
        if (P.HP > 0) {
            Result.Add(P);
        }
    }
    return Result;
}
```

### Pipeline with UE Types

```cpp
#include "Core/functional_core.hpp"

auto result = func::start_pipe(PlayerStateFactory::Create(TEXT("Hero")))
    .pipe([](const FPlayerState& S) {
        return PlayerStateOps::TakeDamage(S, 20);
    })
    .pipe([](const FPlayerState& S) {
        return PlayerStateOps::AddItem(S, TEXT("Health Potion"));
    })
    .unwrap();
```

---

## Summary

1. **Use `struct`, not `class`** — Data is public, immutable, and has no logic.
2. **Use factory functions, not constructors** — All creation in namespaces (`XFactory::Create()`).
3. **Use operations namespaces, not methods** — All logic in namespaces (`XOps::Transform()`).
4. **Avoid `void`** — Functions always return a new value.
5. **Avoid mutation** — Return new data instead of modifying in place.
6. **Avoid `for` loops** — Use `std::transform`, `std::accumulate`, `func::fmap`, or range-based patterns.
7. **Avoid `new`/`delete`** — Use stack allocation or `std::shared_ptr`.
8. **Use Maybe/Either** — Replace null/exceptions with composable algebraic types.
9. **Use composition** — `func::compose()` and `func::start_pipe()` replace complex call chains.
10. **Strict C++11** — No `auto` return types (use trailing `-> decltype`), no `if constexpr`, no structured bindings.
