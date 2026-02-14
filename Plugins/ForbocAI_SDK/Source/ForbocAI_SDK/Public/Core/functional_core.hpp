#pragma once
#ifndef FUNCTIONAL_CORE_HPP
#define FUNCTIONAL_CORE_HPP

// ==========================================================
// Functional Core Library — Strict C++11
// ==========================================================
//
// Pure C++11 functional programming primitives.
// No C++14, C++17, or later features are used.
//
// DESIGN PRINCIPLES:
//   - Factory functions instead of class constructors
//   - Immutable value types (data separated from behavior)
//   - No OOP inheritance, no virtual methods
//   - Functions as first-class citizens
//
// CONTENTS:
//   1. seq / gen_seq      — Index sequence (C++14 backport)
//   2. apply              — Tuple application (C++17 backport)
//   3. Maybe<T>           — Optional monad
//   4. Either<E, T>       — Result/Error monad
//   5. Curried<> / curry  — Automatic function currying
//   6. Lazy<T> / lazy     — Memoized deferred evaluation
//   7. Pipeline<T>        — Fluent transformation chains
//   8. Composed / compose — Binary function composition
//   9. fmap               — Functor map over std::vector
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

namespace func {

// ==========================================
// 1. HELPER: Index Sequence (C++14 backport)
// ==========================================
// Generates a compile-time integer sequence for
// unpacking tuples. Equivalent to C++14's
// std::index_sequence / std::make_index_sequence.
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
// 3. MONAD: Maybe (Optional)
// ==========================================
// A value that may or may not exist.
// Construct via factory functions only:
//   Maybe<T>::Just(value)   — present
//   Maybe<T>::Nothing()     — absent
//
// Usage:
//   auto val  = Maybe<int>::Just(42);
//   auto none = Maybe<int>::Nothing();
//   auto doubled = val.map([](int x) { return x * 2; });
//   int result = doubled.orElse(0);
//
// Requires: T is default-constructible.
// ==========================================

template <typename T> struct Maybe {
  bool hasValue;
  T value;

  // --- Factory Functions (use these, not brace init) ---
  static Maybe<T> Just(T v) { return {true, std::move(v)}; }
  static Maybe<T> Nothing() { return {false, T()}; }

  // --- Monadic bind (flatMap) ---
  // Maybe<T> -> (T -> Maybe<U>) -> Maybe<U>
  template <typename Func>
  auto bind(Func f) const -> decltype(f(value)) {
    if (!hasValue)
      return decltype(f(value))::Nothing();
    return f(value);
  }

  // --- Functor map ---
  // Maybe<T> -> (T -> U) -> Maybe<U>
  template <typename Func>
  auto map(Func f) const -> Maybe<decltype(f(value))> {
    if (!hasValue)
      return Maybe<decltype(f(value))>::Nothing();
    return Maybe<decltype(f(value))>::Just(f(value));
  }

  // --- Extract with default ---
  T orElse(const T &def) const { return hasValue ? value : def; }
};

// ==========================================
// 4. MONAD: Either (Result / Error)
// ==========================================
// A sum type for computations that can fail.
// Convention: Left = error, Right = success.
// Construct via factory functions only:
//   Either<E,T>::Right(value)  — success
//   Either<E,T>::Left(error)   — failure
//
// Usage:
//   typedef Either<std::string, int> IntResult;
//   auto ok  = IntResult::Right(42);
//   auto err = IntResult::Left("division by zero");
//   auto doubled = ok.map([](int x) { return x * 2; });
//
// Requires: E and T are default-constructible.
// ==========================================

template <typename E, typename T> struct Either {
  bool isLeft;
  E left;
  T right;

  // --- Factory Functions ---
  static Either<E, T> Left(E e) { return {true, std::move(e), T()}; }
  static Either<E, T> Right(T v) { return {false, E(), std::move(v)}; }

  // --- Monadic bind (flatMap) ---
  // Either<E,T> -> (T -> Either<E,U>) -> Either<E,U>
  template <typename Func>
  auto bind(Func f) const -> decltype(f(right)) {
    if (isLeft)
      return decltype(f(right))::Left(left);
    return f(right);
  }

  // --- Functor map ---
  // Either<E,T> -> (T -> U) -> Either<E,U>
  template <typename Func>
  auto map(Func f) const -> Either<E, decltype(f(right))> {
    if (isLeft)
      return Either<E, decltype(f(right))>::Left(left);
    return Either<E, decltype(f(right))>::Right(f(right));
  }
};

// ==========================================
// 5. FUNCTION: Currying
// ==========================================
// Converts an N-arity function into a chain of
// single-argument applications.
//
// Use the curry<N>() factory function:
//   auto add = [](int a, int b) { return a + b; };
//   auto curried = func::curry<2>(add);
//   auto add5 = curried(5);    // partial application
//   int result = add5(3);       // 8
// ==========================================

template <size_t Arity, typename Func, typename CapturedArgs = std::tuple<>>
class Curried {
  Func func;
  CapturedArgs args;

public:
  Curried(Func f, CapturedArgs a) : func(f), args(a) {}

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
    return Curried<Arity, Func, decltype(merged)>(func, merged);
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
  return Curried<Arity, Func>(f, std::tuple<>());
}

// ==========================================
// 6. LAZY EVALUATION
// ==========================================
// Deferred, memoized computation. The thunk is
// evaluated at most once on first access.
//
// Use the lazy() factory function:
//   auto val = func::lazy([]() { return expensive(); });
//   const auto& r1 = val.get();  // evaluates thunk
//   const auto& r2 = val.get();  // returns cached
//
// Note: Not thread-safe. Intended for single-thread
// use (e.g. game thread). For thread safety, guard
// access externally with std::call_once or a mutex.
// ==========================================

template <typename T> class Lazy {
  std::function<T()> thunk;
  mutable std::shared_ptr<T> value;

public:
  explicit Lazy(std::function<T()> f)
      : thunk(std::move(f)), value(nullptr) {}

  const T &get() const {
    if (!value) {
      value = std::make_shared<T>(thunk());
    }
    return *value;
  }
};

// Factory function: lazy(thunk)
template <typename F>
auto lazy(F &&f) -> Lazy<decltype(f())> {
  return Lazy<decltype(f())>(std::forward<F>(f));
}

// ==========================================
// 7. COMPOSITION & PIPELINE
// ==========================================
// Fluent chain for threading a value through a
// series of pure transformations.
//
// Supports type-changing steps: T -> U -> V ...
//
// Use the start_pipe() factory function:
//   auto result = func::start_pipe(player)
//       .pipe([](const Player& p) { return move(p, 1, 0); })
//       .pipe([](const Player& p) { return p.hp; })
//       .unwrap();  // returns int
// ==========================================

template <typename T> struct Pipeline {
  T value;

  // Type-changing pipe: Pipeline<T> -> (T -> U) -> Pipeline<U>
  template <typename Func>
  auto pipe(Func f) const -> Pipeline<decltype(f(value))> {
    return Pipeline<decltype(f(value))>{f(value)};
  }

  T unwrap() const { return value; }
};

// Factory function: start_pipe(value)
template <typename T>
Pipeline<T> start_pipe(T v) { return Pipeline<T>{v}; }

// ==========================================
// 8. FUNCTION COMPOSITION
// ==========================================
// Combines two functions: compose(f, g)(x) == f(g(x))
//
// Use the compose() factory function:
//   auto double_it = [](int x) { return x * 2; };
//   auto add_one   = [](int x) { return x + 1; };
//   auto pipeline  = func::compose(add_one, double_it);
//   int result = pipeline(5);  // add_one(double_it(5)) = 11
// ==========================================

template <typename F, typename G>
class Composed {
  F f;
  G g;

public:
  Composed(F f_, G g_) : f(f_), g(g_) {}

  template <typename... Args>
  auto operator()(Args &&...args) const
      -> decltype(f(g(std::forward<Args>(args)...))) {
    return f(g(std::forward<Args>(args)...));
  }
};

// Factory function: compose(f, g) -> h  where h(x) = f(g(x))
template <typename F, typename G>
Composed<F, G> compose(F f, G g) {
  return Composed<F, G>(f, g);
}

// ==========================================
// 9. FUNCTOR MAP (fmap for std::vector)
// ==========================================
// Applies a function to every element, returning
// a new vector. Does not mutate the input.
//
// Usage:
//   std::vector<int> nums = {1, 2, 3};
//   auto doubled = func::fmap(nums, [](int x) { return x * 2; });
//   // doubled == {2, 4, 6}
//
// For Unreal Engine TArray, use range-based patterns
// (see C++11-FP-GUIDE.md, UE-Specific Patterns section).
// ==========================================

template <typename T, typename Func>
auto fmap(const std::vector<T> &vec, Func f)
    -> std::vector<decltype(f(std::declval<const T &>()))> {
  typedef decltype(f(std::declval<const T &>())) ResultT;
  std::vector<ResultT> result;
  result.reserve(vec.size());
  for (const auto &item : vec) {
    result.push_back(f(item));
  }
  return result;
}

} // namespace func

#endif // FUNCTIONAL_CORE_HPP
