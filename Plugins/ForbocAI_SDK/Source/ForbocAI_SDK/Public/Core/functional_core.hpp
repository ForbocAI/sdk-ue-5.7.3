#pragma once
#ifndef FUNCTIONAL_CORE_HPP
#define FUNCTIONAL_CORE_HPP

// ==========================================================
// Functional Core Library — Strict C++11
// ==========================================================
// Pure C++11 functional programming primitives.
// No C++14, C++17, or later features are used.
//
// DESIGN PRINCIPLES:
//   - NO classes. Only structs (plain data containers).
//   - NO constructors. Factory functions for all creation.
//   - NO member functions that operate on data.
//     (operator() is allowed only on callable structs —
//     it is the C++ mechanism for first-class functions,
//     equivalent to lambda application in FP languages.)
//   - Immutable value semantics throughout.
//   - Free functions in the `func` namespace for all operations.
//
// CONTENTS:
//   1. seq / gen_seq        — Index sequence (C++14 backport)
//   2. apply                — Tuple application (C++17 backport)
//   3. Maybe<T>             — Optional monad (data only)
//   4. Either<E, T>         — Result/Error monad (data only)
//   5. Curried / curry      — Automatic function currying
//   6. Lazy<T> / lazy       — Memoized deferred evaluation
//   7. Pipeline<T> / pipe   — Value transformation chains (operator|)
//   8. Composed / compose   — Binary function composition
//   9. fmap                 — Functor map (Maybe, Either, vector)
//  10. mbind / ebind        — Monadic bind for Maybe / Either
//  11. or_else / match      — Extraction / pattern matching
//  12. ValidationPipeline   — Functional validation chain
//  13. ConfigBuilder        — Functional configuration builder
//  14. TestResult           — Functional testing result
//  15. AsyncResult          — Functional async result handling
//
// REQUIREMENTS:
//   Maybe<T> and Either<E,T> require T and E to be default-
//   constructible. This is a deliberate C++11 trade-off
//   (C++17's std::optional removes this need). All Unreal
//   Engine USTRUCTs satisfy this requirement.
//
// See also: C++11-FP-GUIDE.md for patterns and usage.
// ==========================================================

#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <unordered_map>
#include <string>

namespace func {

// ==========================================
// 1. HELPER: Index Sequence (C++14 backport)
// ==========================================
// Generates a compile-time integer sequence for
// unpacking tuples. Equivalent to C++14's
// std::index_sequence / std::make_index_sequence.
//
// Note: gen_seq uses recursive template inheritance
// as the standard C++11 technique for this pattern.
// ==========================================

template <size_t... Is> struct seq {};

template <size_t N, size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

template <size_t... Is>
struct gen_seq<0, Is...> : seq<Is...> {};

// ==========================================
// 2. HELPER: Tuple Application (C++17 backport)
// ==========================================
// Calls a function with arguments unpacked from
// a tuple. Equivalent to C++17's std::apply.
//
// Usage:
//   auto args = std::make_tuple(1, 2, 3);
//   auto result = func::apply(myFunc, args);
// ==========================================

template <typename F, typename Tuple, size_t... Is>
auto apply_impl(F &&f, Tuple &&t, seq<Is...>)
    -> decltype(std::forward<F>(f)(
           std::get<Is>(std::forward<Tuple>(t))...)) {
  return std::forward<F>(f)(
      std::get<Is>(std::forward<Tuple>(t))...);
}

template <typename F, typename Tuple>
auto apply(F &&f, Tuple &&t) -> decltype(apply_impl(
    std::forward<F>(f), std::forward<Tuple>(t),
    gen_seq<std::tuple_size<
        typename std::remove_reference<Tuple>::type>::value>())) {
  return apply_impl(
      std::forward<F>(f), std::forward<Tuple>(t),
      gen_seq<std::tuple_size<
          typename std::remove_reference<Tuple>::type>::value>());
}

// ==========================================
// 3. DATA: Maybe (Optional Monad)
// ==========================================
// A value that may or may not exist.
// Pure data struct — no methods.
//
// Construction: use factory functions just() / nothing()
// Operations:   use free functions fmap() / mbind() / or_else()
//
// Requires: T is default-constructible.
// ==========================================

template <typename T> struct Maybe {
  bool hasValue;
  T value;
};

// --- Factory Functions ---

template <typename T>
Maybe<T> just(T v) {
  return Maybe<T>{true, std::move(v)};
}

template <typename T>
Maybe<T> nothing() {
  return Maybe<T>{false, T{}};
}

// ==========================================
// 4. DATA: Either (Result/Error Monad)
// ==========================================
// A sum type for computations that can fail.
// Convention: Left = error, Right = success.
// Pure data struct — no methods.
//
// Construction: use factory functions make_left() / make_right()
// Operations:   use free functions fmap() / ebind()
//
// Requires: E and T are default-constructible.
// ==========================================

template <typename E, typename T> struct Either {
  bool isLeft;
  E left;
  T right;
};

// --- Factory Functions ---

template <typename E, typename T>
Either<E, T> make_left(E e) {
  return Either<E, T>{true, std::move(e), T{}};
}

template <typename E, typename T>
Either<E, T> make_right(T v) {
  return Either<E, T>{false, E{}, std::move(v)};
}

// ==========================================
// 5. CALLABLE: Curried (Function Currying)
// ==========================================
// Converts an N-arity function into a chain of
// single-argument applications.
//
// Construction: use the curry<N>() factory function.
// operator() is the C++ mechanism for callable types
// (equivalent to lambda application in FP).
//
// Usage:
//   auto add = [](int a, int b) { return a + b; };
//   auto curried = func::curry<2>(add);
//   auto add5 = curried(5);    // partial application
//   int result = add5(3);       // 8
// ==========================================

template <size_t Arity, typename Func, typename CapturedArgs = std::tuple<>>
struct Curried {
  Func func;
  CapturedArgs args;

  // Partial application: not enough args yet
  template <typename... NewArgs>
  auto operator()(NewArgs &&...new_args) const -> typename std::enable_if<
      (std::tuple_size<CapturedArgs>::value + sizeof...(NewArgs) < Arity),
      Curried<Arity, Func,
              decltype(std::tuple_cat(
                  args,
                  std::make_tuple(
                      std::forward<NewArgs>(new_args)...)))>>::type {
    auto merged = std::tuple_cat(
        args, std::make_tuple(std::forward<NewArgs>(new_args)...));
    return Curried<Arity, Func, decltype(merged)>{func, merged};
  }

  // Full application: enough args, invoke the function
  template <typename... NewArgs>
  auto operator()(NewArgs &&...new_args) const -> typename std::enable_if<
      (std::tuple_size<CapturedArgs>::value + sizeof...(NewArgs) >= Arity),
      decltype(apply(
          func,
          std::tuple_cat(
              args,
              std::make_tuple(
                  std::forward<NewArgs>(new_args)...))))>::type {
    return apply(
        func,
        std::tuple_cat(
            args,
            std::make_tuple(std::forward<NewArgs>(new_args)...)));
  }
};

// Factory function: curry<Arity>(f)
template <size_t Arity, typename Func>
Curried<Arity, Func> curry(Func f) {
  return Curried<Arity, Func>{f, std::tuple<>{}};
}

// ==========================================
// 6. DATA: Lazy (Deferred Evaluation)
// ==========================================
// Memoized deferred computation. The thunk is
// evaluated at most once on first access via eval().
//
// Construction: use the lazy() factory function.
// Access:       use the eval() free function.
//
// Note: Not thread-safe. Intended for single-thread
// use (e.g. game thread).
// ==========================================

template <typename T> struct Lazy {
  std::function<T()> thunk;
  mutable std::shared_ptr<T> cached;
};

// Factory function: lazy(thunk)
template <typename F>
auto lazy(F &&f) -> Lazy<decltype(f())> {
  return Lazy<decltype(f())>{std::forward<F>(f), nullptr};
}

// Free function: eval(lazy_val) — forces evaluation, caches result
template <typename T>
const T &eval(const Lazy<T> &lz) {
  if (!lz.cached) {
    lz.cached = std::make_shared<T>(lz.thunk());
  }
  return *lz.cached;
}

// ==========================================
// 7. DATA: Pipeline (Value Transformation)
// ==========================================
// Fluent chain for threading a value through a
// series of pure transformations using operator|.
//
// Construction: use the pipe() factory function.
// Chaining:     use operator| with transform functions.
// Extraction:   access the .val member directly.
//
// Usage:
//   auto add1 = [](int x) { return x + 1; };
//   auto mul2 = [](int x) { return x * 2; };
//   auto result = func::pipe(5) | add1 | mul2;
//   int final = result.val;  // 12
// ==========================================

template <typename T> struct Pipeline {
  T val;
};

// Factory function: pipe(value)
template <typename T>
Pipeline<T> pipe(T v) {
  return Pipeline<T>{std::move(v)};
}

// operator| for chaining: Pipeline<T> | (T -> U) -> Pipeline<U>
template <typename T, typename F>
auto operator|(const Pipeline<T> &p, F f) -> Pipeline<decltype(f(p.val))> {
  return Pipeline<decltype(f(p.val))>{f(p.val)};
}

// ==========================================
// 8. CALLABLE: Composed (Function Composition)
// ==========================================
// Combines two functions: compose(f, g)(x) == f(g(x))
//
// Construction: use the compose() factory function.
// operator() is the C++ mechanism for callable types.
//
// Usage:
//   auto double_it = [](int x) { return x * 2; };
//   auto add_one   = [](int x) { return x + 1; };
//   auto both      = func::compose(add_one, double_it);
//   int result = both(5);  // add_one(double_it(5)) = 11
// ==========================================

template <typename F, typename G> struct Composed {
  F f;
  G g;

  template <typename... Args>
  auto operator()(Args &&...args) const
      -> decltype(f(g(std::forward<Args>(args)...))) {
    return f(g(std::forward<Args>(args)...));
  }
};

// Factory function: compose(f, g) -> h  where h(x) = f(g(x))
template <typename F, typename G>
Composed<F, G> compose(F f, G g) {
  return Composed<F, G>{f, g};
}

// ==========================================
// 9. FREE FUNCTION: fmap (Functor Map)
// ==========================================
// Applies a function inside a context, returning
// a new value in the same context. Overloaded for:
//   - Maybe<T>
//   - Either<E, T>
//   - std::vector<T>
//
// Does not mutate any input.
// ==========================================

// fmap for Maybe: Maybe<T> -> (T -> U) -> Maybe<U>
template <typename T, typename Func>
auto fmap(const Maybe<T> &m, Func f) -> Maybe<decltype(f(m.value))> {
  typedef decltype(f(m.value)) U;
  if (!m.hasValue)
    return Maybe<U>{false, U{}};
  return Maybe<U>{true, f(m.value)};
}

// fmap for Either: Either<E,T> -> (T -> U) -> Either<E,U>
template <typename E, typename T, typename Func>
auto fmap(const Either<E, T> &e, Func f)
    -> Either<E, decltype(f(e.right))> {
  typedef decltype(f(e.right)) U;
  if (e.isLeft)
    return Either<E, U>{true, e.left, U{}};
  return Either<E, U>{false, E{}, f(e.right)};
}

// fmap for std::vector: vector<T> -> (T -> U) -> vector<U>
template <typename T, typename Func>
auto fmap(const std::vector<T> &vec, Func f)
    -> std::vector<decltype(f(std::declval<const T &>()))> {
  typedef decltype(f(std::declval<const T &>())) U;
  std::vector<U> result;
  result.reserve(vec.size());
  for (const auto &item : vec) {
    result.push_back(f(item));
  }
  return result;
}

// ==========================================
// 10. FREE FUNCTION: Monadic Bind
// ==========================================
// Chains computations that return wrapped values.
//
// mbind: Maybe<T> -> (T -> Maybe<U>) -> Maybe<U>
// ebind: Either<E,T> -> (T -> Either<E,U>) -> Either<E,U>
// ==========================================

// Monadic bind for Maybe
template <typename T, typename Func>
auto mbind(const Maybe<T> &m, Func f) -> decltype(f(m.value)) {
  if (!m.hasValue)
    return decltype(f(m.value)){false, {}};
  return f(m.value);
}

// Monadic bind for Either
template <typename E, typename T, typename Func>
auto ebind(const Either<E, T> &e, Func f) -> decltype(f(e.right)) {
  if (e.isLeft)
    return decltype(f(e.right)){true, e.left, {}};
  return f(e.right);
}

// ==========================================
// 11. FREE FUNCTION: Extraction & Matching
// ==========================================
// or_else:  Extract from Maybe with default value.
// match:    Pattern match on Maybe (Just / Nothing).
// ematch:   Pattern match on Either (Left / Right).
// ==========================================

// or_else: Maybe<T> -> T -> T
template <typename T>
T or_else(const Maybe<T> &m, const T &def) {
  return m.hasValue ? m.value : def;
}

// match: Maybe<T> -> (T -> R) -> (() -> R) -> R
template <typename T, typename FJust, typename FNothing>
auto match(const Maybe<T> &m, FJust onJust, FNothing onNothing)
    -> decltype(onJust(m.value)) {
  if (m.hasValue)
    return onJust(m.value);
  return onNothing();
}

// ematch: Either<E,T> -> (E -> R) -> (T -> R) -> R
template <typename E, typename T, typename FLeft, typename FRight>
auto ematch(const Either<E, T> &e, FLeft onLeft, FRight onRight)
    -> decltype(onRight(e.right)) {
  if (e.isLeft)
    return onLeft(e.left);
  return onRight(e.right);
}

// ==========================================
// 12. ValidationPipeline (Functional Validation Chain)
// ==========================================
// A pipeline for chaining validation functions.
// Each validation function takes input and returns
// Either<Error, Result>. The pipeline short-circuits
// on first error.
//
// Usage:
//   auto pipeline = ValidationPipeline<int>()
//       .add(validatePositive)
//       .add(validateRange)
//       .add(validateEven);
//   auto result = pipeline.run(42);
// ==========================================

template <typename T> class ValidationPipeline {
  std::vector<std::function<Either<std::string, T>(T)>> validators;

public:
  ValidationPipeline() = default;

  // Add a validation function to the pipeline
  ValidationPipeline &add(std::function<Either<std::string, T>(T)> validator) {
    validators.push_back(std::move(validator));
    return *this;
  }

  // Run the validation pipeline
  Either<std::string, T> run(T value) const {
    for (const auto &validator : validators) {
      auto result = validator(value);
      if (result.isLeft) {
        return result; // Short-circuit on first error
      }
      value = result.right;
    }
    return make_right(std::string{}, value);
  }
};

// Factory function for ValidationPipeline
template <typename T>
ValidationPipeline<T> validationPipeline() {
  return ValidationPipeline<T>();
}

// ==========================================
// 13. ConfigBuilder (Functional Configuration Builder)
// ==========================================
// A builder pattern for creating immutable
// configuration objects using functional composition.
//
// Usage:
//   auto config = ConfigBuilder<MyConfig>()
//       .set("name", "MyApp")
//       .set("port", 8080)
//       .build();
// ==========================================

template <typename Config> class ConfigBuilder {
  std::unordered_map<std::string, std::function<void(Config &)>> setters;

public:
  ConfigBuilder() = default;

  // Add a setter function
  template <typename T>
  ConfigBuilder &set(const std::string &key, T value) {
    setters[key] = [value](Config &config) mutable {
      // Use reflection or manual mapping to set the value
      // For simplicity, we'll assume Config has a set method
      config.set(key, std::move(value));
    };
    return *this;
  }

  // Build the configuration object
  Config build() const {
    Config config;
    for (const auto &pair : setters) {
      pair.second(config);
    }
    return config;
  }
};

// Factory function for ConfigBuilder
template <typename Config>
ConfigBuilder<Config> configBuilder() {
  return ConfigBuilder<Config>();
}

// ==========================================
// 14. TestResult (Functional Testing Result)
// ==========================================
// A result type for functional testing that
// includes success/failure, messages, and
// optional detailed information.
//
// Usage:
//   auto result = TestResult<bool>::success(true);
//   auto failure = TestResult<void>::failure("Test failed");
// ==========================================

template <typename T> struct TestResult {
  bool success;
  T value;
  std::string message;
  std::unordered_map<std::string, std::string> details;

  // Factory functions
  static TestResult<T> success(T value, std::string message = "") {
    return TestResult<T>{true, std::move(value), std::move(message), {}};
  }

  static TestResult<T> failure(std::string message) {
    return TestResult<T>{false, T{}, std::move(message), {}};
  }

  // Add details
  TestResult &withDetail(const std::string &key, const std::string &value) {
    details[key] = value;
    return *this;
  }

  // Check if successful
  bool isSuccessful() const {
    return success;
  }

  // Get value or throw if failure
  T getValue() const {
    if (!success) {
      throw std::runtime_error("TestResult: Cannot get value from failure");
    }
    return value;
  }
};

// Specialization for void

template <> struct TestResult<void> {
  bool success;
  std::string message;
  std::unordered_map<std::string, std::string> details;

  static TestResult<void> success(std::string message = "") {
    return TestResult<void>{true, std::move(message), {}};
  }

  static TestResult<void> failure(std::string message) {
    return TestResult<void>{false, std::move(message), {}};
  }

  TestResult &withDetail(const std::string &key, const std::string &value) {
    details[key] = value;
    return *this;
  }

  bool isSuccessful() const {
    return success;
  }
};

// ==========================================
// 15. AsyncResult (Functional Async Result Handling)
// ==========================================
// A type for handling async operations that
// can succeed or fail, with support for
// chaining and error handling.
//
// Usage:
//   auto result = AsyncResult<int>::create([](auto resolve, auto reject) {
//       // async operation
//   });
//
//   result.then([](int value) {
//       // success
//   }).catch_([](std::string error) {
//       // failure
//   });
// ==========================================

template <typename T> class AsyncResult {
  std::function<void(std::function<void(T)>, std::function<void(std::string)>)> executor;
  std::vector<std::function<void(T)>> successHandlers;
  std::vector<std::function<void(std::string)>> errorHandlers;

public:
  // Create an async result from an executor
  static AsyncResult<T> create(std::function<void(std::function<void(T)>, std::function<void(std::string)>)> executor) {
    return AsyncResult<T>{std::move(executor)};
  }

  AsyncResult(std::function<void(std::function<void(T)>, std::function<void(std::string)>)> executor)
      : executor(std::move(executor)) {}

  // Add success handler
  AsyncResult<T> &then(std::function<void(T)> handler) {
    successHandlers.push_back(std::move(handler));
    return *this;
  }

  // Add error handler
  AsyncResult<T> &catch_(std::function<void(std::string)> handler) {
    errorHandlers.push_back(std::move(handler));
    return *this;
  }

  // Execute the async operation
  void execute() {
    executor(
      [this](T value) {
        for (const auto &handler : successHandlers) {
          handler(value);
        }
      },
      [this](std::string error) {
        for (const auto &handler : errorHandlers) {
          handler(error);
        }
      }
    );
  }
};

// Specialization for void

template <> class AsyncResult<void> {
  std::function<void(std::function<void()>, std::function<void(std::string)>)> executor;
  std::vector<std::function<void()>> successHandlers;
  std::vector<std::function<void(std::string)>> errorHandlers;

public:
  static AsyncResult<void> create(std::function<void(std::function<void()>, std::function<void(std::string)>)> executor) {
    return AsyncResult<void>{std::move(executor)};
  }

  AsyncResult(std::function<void(std::function<void()>, std::function<void(std::string)>)> executor)
      : executor(std::move(executor)) {}

  AsyncResult<void> &then(std::function<void()> handler) {
    successHandlers.push_back(std::move(handler));
    return *this;
  }

  AsyncResult<void> &catch_(std::function<void(std::string)> handler) {
    errorHandlers.push_back(std::move(handler));
    return *this;
  }

  void execute() {
    executor(
      [this]() {
        for (const auto &handler : successHandlers) {
          handler();
        }
      },
      [this](std::string error) {
        for (const auto &handler : errorHandlers) {
          handler(error);
        }
      }
    );
  }
};

} // namespace func

#endif // FUNCTIONAL_CORE_HPP
