# Functional C++11 Guide

`Codex // Functional_Paradigm`

> A guide to functional programming in **strict C++11** for the ForbocAI SDK.
> All code targets C++11 only -- no C++14/17 features.
>
> **Library**: This guide accompanies [`functional_core.hpp`](Plugins/ForbocAI_SDK/Source/ForbocAI_SDK/Public/Core/functional_core.hpp), which provides production-ready implementations of Maybe, Either, Currying, Lazy evaluation, Pipeline, Composition, and fmap — all as **pure data structs with free functions only**.

---

## Core Principles

| OOP (avoid)                | FP (prefer)                               |
|----------------------------|-------------------------------------------|
| `class` with methods       | `struct` — pure data, **no methods**      |
| Constructors               | **Factory functions** in namespaces       |
| Member functions           | **Free functions** in namespaces          |
| `this->mutate()`           | Return a **new value**                    |
| Inheritance / virtual      | Higher-order functions / composition      |
| `void` methods             | Pure functions that **always return**     |
| `for` loops with mutation  | `std::transform` / `std::accumulate`      |
| `new` / `delete`           | Stack values or `std::shared_ptr`         |

**The iron rule: structs hold data; namespaces hold functions.**

---

## 1. Data vs. Behavior (Structs, not Classes)

In FP, data and behavior are **separated**. Structs are pure data carriers with **no member functions** (except `operator()` on callable types, which is the C++ mechanism for first-class functions). All logic lives in free functions inside namespaces.

```cpp
#include <string>
#include <vector>

// Pure Data — no logic, no methods, no constructors
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

## 2. The Factory Pattern (Replaces ALL Constructors)

**Never use constructors.** Use factory functions in namespaces for all creation. Factories separate creation logic from the data layout and are composable with other functions.

```cpp
// Factory namespace — replaces constructors entirely
namespace PlayerFactory {
    // Pure factory: inputs -> Player (aggregate initialization)
    Player create(const std::string& name, int startX, int startY) {
        return Player{name, 100, startX, startY};
    }

    Player fromJson(const std::string& json) {
        return Player{"Parsed", 100, 0, 0};
    }
}

// Usage: always construct through factories
auto hero = PlayerFactory::create("Hero", 0, 0);
```

**For UE USTRUCTs** (which require a default constructor for reflection and cannot use aggregate init due to `GENERATED_BODY()`):

```cpp
namespace TypeFactory {
    inline FMyStruct Create(FString Name, int32 Value) {
        FMyStruct S;         // UE default constructor
        S.Name = MoveTemp(Name);
        S.Value = Value;
        return S;            // Factory returns the configured instance
    }
}
```

**Why factories over constructors?**
- Logic is testable independently of data layout
- Multiple named factories for different creation paths
- No hidden `this` pointer — all inputs and outputs are explicit
- Composable with other functions (a factory IS a function)

---

## 3. Free Functions Only (No Member Functions)

All operations on data go in namespace-scoped free functions. Structs never have methods.

```cpp
// Operations namespace — stateless pure functions
namespace PlayerOps {
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

C++11's `std::function` and lambdas allow functions to accept and return other functions, replacing Strategy, Observer, and similar OOP patterns.

```cpp
#include <functional>

// HOF: applies any effect to a player
Player applyEffect(const Player& p, std::function<Player(const Player&)> effect) {
    return effect(p);
}

auto p1 = PlayerFactory::create("Hero", 0, 0);

auto poison = [](const Player& p) {
    return PlayerOps::takeDamage(p, 5);
};

auto p2 = applyEffect(p1, poison);   // p1 untouched
```

---

## 5. Maybe Monad (Optional Values)

`Maybe<T>` from `functional_core.hpp` represents a value that may or may not exist. **Pure data struct, no methods.** Use free functions for all operations.

```cpp
#include "Core/functional_core.hpp"

// --- Factory construction ---
auto health   = func::just(100);
auto noHealth = func::nothing<int>();

// --- Transform with fmap (functor) ---
auto doubled = func::fmap(health, [](int hp) { return hp * 2; });
// doubled == Just(200)

auto nope = func::fmap(noHealth, [](int hp) { return hp * 2; });
// nope == Nothing (transformation skipped safely)

// --- Chain with mbind (monadic bind / flatMap) ---
auto safeDivide = [](int x) -> func::Maybe<int> {
    if (x == 0) return func::nothing<int>();
    return func::just(100 / x);
};
auto result = func::mbind(health, safeDivide);            // Just(1)
auto fail   = func::mbind(func::just(0), safeDivide);    // Nothing

// --- Extract with default ---
int hp   = func::or_else(health, 0);     // 100
int noHp = func::or_else(noHealth, 0);   // 0

// --- Pattern match ---
auto msg = func::match(health,
    [](int hp) { return std::string("Alive"); },
    []()       { return std::string("Dead"); });
```

---

## 6. Either Monad (Error Handling)

`Either<E, T>` replaces exceptions and error codes. Convention: **Left = error, Right = success.** Pure data struct, free functions only.

```cpp
#include "Core/functional_core.hpp"

typedef func::Either<std::string, Player> PlayerResult;

// --- Factory construction ---
auto ok  = func::make_right<std::string, Player>(PlayerFactory::create("Hero", 0, 0));
auto err = func::make_left<std::string, Player>(std::string("Invalid name"));

// --- Transform the success value ---
auto moved = func::fmap(ok, [](const Player& p) {
    return PlayerOps::move(p, 5, 0);
});
// moved == Right(Player at x=5)

auto stillErr = func::fmap(err, [](const Player& p) {
    return PlayerOps::move(p, 5, 0);
});
// stillErr == Left("Invalid name") — error propagates automatically

// --- Chain operations that can fail ---
auto validateAndMove = [](const Player& p) -> PlayerResult {
    if (p.hp <= 0)
        return func::make_left<std::string, Player>(std::string("Dead"));
    return func::make_right<std::string, Player>(PlayerOps::move(p, 1, 0));
};
auto result = func::ebind(ok, validateAndMove);

// --- Pattern match ---
auto msg = func::ematch(result,
    [](const std::string& e) { return e; },
    [](const Player& p)      { return p.name; });
```

---

## 7. Currying & Partial Application

`curry<N>()` from `functional_core.hpp` converts an N-argument function into a chain of single-argument calls.

```cpp
#include "Core/functional_core.hpp"

auto damage = [](int base, float multiplier, const Player& p) {
    int amount = static_cast<int>(base * multiplier);
    return PlayerOps::takeDamage(p, amount);
};

auto curriedDamage = func::curry<3>(damage);

// Partial application: fix base and multiplier
auto fireDamage = curriedDamage(20)(1.5f);   // base=20, mult=1.5
auto iceDamage  = curriedDamage(15)(0.8f);   // base=15, mult=0.8

auto p1 = PlayerFactory::create("Mage", 0, 0);
auto p2 = fireDamage(p1);   // 30 fire damage
auto p3 = iceDamage(p1);    // 12 ice damage
```

---

## 8. Lazy Evaluation

`Lazy<T>` defers computation until needed, then caches it. Use the `lazy()` factory and `eval()` free function.

```cpp
#include "Core/functional_core.hpp"

// Nothing runs yet — the thunk is stored, not evaluated
auto expensiveState = func::lazy([]() {
    return WorldState{{}, 0};
});

// First call evaluates the thunk and caches the result
const WorldState& state1 = func::eval(expensiveState);

// Second call returns the cached result (no recomputation)
const WorldState& state2 = func::eval(expensiveState);
```

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

// fmap also works on Maybe:
auto val = func::fmap(func::just(10), [](int x) { return x * 3; });
// val == Just(30)
```

**Map equivalent (`std::transform`):**

```cpp
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

`Pipeline<T>` threads a value through transforms using `operator|`. Reads **left-to-right** and supports **type-changing** steps.

```cpp
#include "Core/functional_core.hpp"

// Type changes through the pipeline: Player -> Player -> int -> bool
auto result = func::pipe(PlayerFactory::create("Mage", 0, 0))
    | [](const Player& p) { return PlayerOps::move(p, 5, 5); }
    | [](const Player& p) { return PlayerOps::takeDamage(p, 10); }
    | [](const Player& p) { return p.hp; }        // Player -> int
    | [](int hp) { return hp > 0; };               // int -> bool

bool isAlive = result.val;  // access data directly (no unwrap method)
```

---

## 11. Unreal Engine FP Patterns

The functional paradigm applies identically to Unreal Engine types. The ForbocAI SDK follows these patterns throughout.

### Data Structs with USTRUCT (no methods, no logic)

```cpp
// Pure data — struct, not class. Default constructor only (UE-required).
USTRUCT()
struct FPlayerState {
    GENERATED_BODY()

    UPROPERTY()
    FString Name;

    UPROPERTY()
    int32 HP;

    UPROPERTY()
    TArray<FString> Inventory;

    FPlayerState() : HP(100) {}
    // NO parameterized constructors. Use TypeFactory.
};
```

### Factory Functions in Namespaces (not constructors)

```cpp
// Factory namespace — all creation logic here
namespace PlayerStateFactory {
    inline FPlayerState Create(const FString& Name) {
        FPlayerState State;
        State.Name = Name;
        State.HP = 100;
        return State;
    }

    inline FPlayerState FromSave(const FString& Json) {
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
    FPlayerState TakeDamage(const FPlayerState& State, int32 Amount) {
        FPlayerState New = State;  // copy
        New.HP = FMath::Max(0, State.HP - Amount);
        return New;  // return new value, original untouched
    }

    FPlayerState AddItem(const FPlayerState& State, const FString& Item) {
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
```

### Pipeline with UE Types

```cpp
#include "Core/functional_core.hpp"

auto result = func::pipe(PlayerStateFactory::Create(TEXT("Hero")))
    | [](const FPlayerState& S) {
        return PlayerStateOps::TakeDamage(S, 20);
    }
    | [](const FPlayerState& S) {
        return PlayerStateOps::AddItem(S, TEXT("Health Potion"));
    };

FPlayerState final = result.val;  // direct data access
```

---

## UE-Required Exceptions

The following OOP patterns are **required by Unreal Engine** and are the only acceptable deviations:

| Pattern                     | Reason                                     |
|-----------------------------|--------------------------------------------|
| `UCLASS() : public UBase`  | UE reflection + commandlet interface       |
| `GENERATED_BODY()`         | UE header tool code generation             |
| `virtual ... override`     | UE interface implementation (Commandlets)  |
| Default constructors only  | UE reflection needs default-constructible  |
| Non-const USTRUCT members  | UE reflection needs mutable access         |
| `IMPLEMENT_MODULE(...)`    | UE module registration macro               |
| `operator()` on callables  | C++ mechanism for first-class functions    |

---

## Summary

1.  **Use `struct`, not `class`** — Data is public, ideally `const`, and has **no methods**.
2.  **Use factory functions, not constructors** — All creation in namespaces (`XFactory::Create()`).
3.  **Use free functions, not member functions** — All logic in namespaces (`XOps::Transform()`).
4.  **Avoid `void`** — Functions always return a new value.
5.  **Avoid mutation** — Return new data instead of modifying in place.
6.  **Avoid `for` loops** — Use `std::transform`, `std::accumulate`, `func::fmap`, or range-based patterns.
7.  **Avoid `new`/`delete`** — Use stack allocation or `std::shared_ptr`.
8.  **Use Maybe/Either** — Replace null/exceptions with composable algebraic types via free functions.
9.  **Use composition** — `func::compose()` and `func::pipe() | f | g` replace complex call chains.
10. **Strict C++11** — No `auto` return types (use trailing `-> decltype`), no `if constexpr`, no structured bindings.
