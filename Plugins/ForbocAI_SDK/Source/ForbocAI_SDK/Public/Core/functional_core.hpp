#pragma once
#ifndef FUNCTIONAL_CORE_HPP
#define FUNCTIONAL_CORE_HPP

/**
 * Functional Core Library — Strict C++11
 * Pure C++11 functional programming primitives.
 * No C++14, C++17, or later features are used.
 * This header is the canonical source of truth for the
 * functional substrate. If surrounding docs disagree, this file wins.
 * DESIGN PRINCIPLES:
 *   - Prefer structs and plain data for domain state.
 *   - Prefer factory functions for construction of public values.
 *   - Keep domain behavior in free functions under the `func` namespace.
 *   - A small number of helper wrappers intentionally expose fluent
 *     member APIs where C++11 ergonomics would otherwise become
 *     unreadable (`MemoizedLast`, `ValidationPipeline`,
 *     `ConfigBuilder`, `AsyncResult`).
 *   - Value semantics throughout.
 * CONTENTS:
 *   1. seq / gen_seq        — Index sequence (C++14 backport)
 *   2. apply                — Tuple application (C++17 backport)
 *   3. Maybe<T>             — Optional monad (data only)
 *   4. Either<E, T>         — Result/Error monad (data only)
 *   5. Curried / curry      — Automatic function currying
 *   6. Lazy<T> / lazy       — Memoized deferred evaluation
 *   7. MemoizedLast         — Last-input memoization for derived values
 *   8. Pipeline<T> / pipe   — Value transformation chains (operator|)
 *   9. Composed / compose   — Binary function composition
 *  10. fmap                 — Functor map (Maybe, Either, vector)
 *  11. mbind / ebind        — Monadic bind for Maybe / Either
 *  12. or_else / match      — Extraction / pattern matching
 *  13. ValidationPipeline   — Functional validation chain
 *  14. ConfigBuilder        — Functional configuration builder
 *  15. TestResult           — Functional testing result
 *  16. AsyncResult          — Functional async result handling
 *  17. HttpResult           — Functional HTTP result wrapper
 *  18. AsyncChain           — AsyncResult chaining helpers
 *  19. Dispatcher            — Dictionary-based typed dispatch
 *  20. multi_match           — Multi-case value-based pattern matching
 *  21. from_nullable         — Lift nullable values into Maybe
 * REQUIREMENTS:
 *   Several helpers default-construct inactive payloads or
 *   error branches as a deliberate C++11 trade-off:
 *   `Maybe<T>`, `Either<E, T>`, `ValidationPipeline<T, E>`,
 *   `TestResult<T>`, and `HttpResult<T>`.
 *   All host types used with these primitives are expected
 *   to satisfy that requirement.
 * See also: C++11-FP-GUIDE.md for patterns and usage.
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

#include <cstdint>
#include <cstdlib>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace func {

/**
 * 1. HELPER: Index Sequence (C++14 backport)
 * Generates a compile-time integer sequence for
 * unpacking tuples. Equivalent to C++14's
 * std::index_sequence / std::make_index_sequence.
 * Note: gen_seq uses recursive template inheritance
 * as the standard C++11 technique for this pattern.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

template <size_t... Is> struct seq {};

template <size_t N, size_t... Is>
struct gen_seq : gen_seq<N - 1, N - 1, Is...> {};

template <size_t... Is> struct gen_seq<0, Is...> : seq<Is...> {};

/**
 * Invokes a callable with tuple elements expanded by index sequence.
 * User Story: As C++11 functional helpers, I need tuple expansion so stored
 * argument lists can be replayed through generic callables cleanly.
 */
template <typename F, typename Tuple, size_t... Is>
auto apply_impl(F &&f, Tuple &&t, seq<Is...>)
    -> decltype(std::forward<F>(f)(std::get<Is>(std::forward<Tuple>(t))...)) {
  return std::forward<F>(f)(std::get<Is>(std::forward<Tuple>(t))...);
}

/**
 * Applies a callable to the contents of a tuple.
 * User Story: As higher-order helpers, I need tuple application so currying
 * and deferred calls can execute stored arguments consistently.
 */
template <typename F, typename Tuple>
auto apply(F &&f, Tuple &&t) -> decltype(apply_impl(
    std::forward<F>(f), std::forward<Tuple>(t),
    gen_seq<std::tuple_size<
        typename std::remove_reference<Tuple>::type>::value>())) {
  return apply_impl(std::forward<F>(f), std::forward<Tuple>(t),
                    gen_seq<std::tuple_size<
                        typename std::remove_reference<Tuple>::type>::value>());
}

/**
 * 3. DATA: Maybe (Optional Monad)
 * A value that may or may not exist.
 * Pure data struct — no methods.
 * Construction: use factory functions just() / nothing()
 * Operations:   use free functions fmap() / mbind() / or_else()
 * Requires: T is default-constructible.
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename T> struct Maybe {
  bool hasValue;
  T value;
};

/**
 * Wraps a concrete value in a populated Maybe.
 * User Story: As optional flows, I need a simple way to lift a value into
 * Maybe so absence and presence stay explicit in pipelines.
 */
template <typename T> Maybe<T> just(T v) {
  return Maybe<T>{true, std::move(v)};
}

/**
 * Builds an empty Maybe with a default-constructed payload.
 * User Story: As optional flows, I need a canonical empty Maybe so code can
 * represent missing values without custom sentinels.
 */
template <typename T> Maybe<T> nothing() { return Maybe<T>{false, T{}}; }

/**
 * 4. DATA: Either (Result/Error Monad)
 * A sum type for computations that can fail.
 * Convention: Left = error, Right = success.
 * Pure data struct — no methods.
 * Construction: use factory functions make_left() / make_right()
 * Operations:   use free functions fmap() / ebind()
 * Requires: E and T are default-constructible.
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename E, typename T> struct Either {
  bool isLeft;
  E left;
  T right;
};

/**
 * Constructs the error branch of an Either value.
 * User Story: As result-returning code, I need a clear error constructor so
 * failure paths remain explicit in functional chains.
 */
template <typename E, typename T> Either<E, T> make_left(E e) {
  return Either<E, T>{true, std::move(e), T{}};
}

/**
 * Constructs the error branch while preserving an explicit fallback payload.
 * User Story: As result-returning code, I need an error constructor that also
 * satisfies payload shape requirements in C++11.
 */
template <typename E, typename T> Either<E, T> make_left(E e, T dummy) {
  return Either<E, T>{true, std::move(e), std::move(dummy)};
}

/**
 * Constructs the success branch of an Either value.
 * User Story: As result-returning code, I need a clear success constructor so
 * successful values move through pipelines predictably.
 */
template <typename E, typename T> Either<E, T> make_right(T v) {
  return Either<E, T>{false, E{}, std::move(v)};
}

/**
 * Constructs the success branch while preserving an explicit fallback error value.
 * User Story: As result-returning code, I need a success constructor that also
 * preserves error shape requirements in C++11.
 */
template <typename E, typename T> Either<E, T> make_right(E dummy, T v) {
  return Either<E, T>{false, std::move(dummy), std::move(v)};
}

/**
 * 5. CALLABLE: Curried (Function Currying)
 * Converts an N-arity function into a chain of
 * single-argument applications.
 * Construction: use the curry<N>() factory function.
 * operator() is the C++ mechanism for callable types
 * (equivalent to lambda application in FP).
 * Usage:
 *   auto add = [](int a, int b) { return a + b; };
 *   auto curried = func::curry<2>(add);
 *   auto add5 = curried(5);    // partial application
 *   int result = add5(3);       // 8
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <size_t Arity, typename Func, typename CapturedArgs = std::tuple<>>
struct Curried {
  Func func;
  CapturedArgs args;

  /**
   * Partial application: not enough args yet
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  template <typename... NewArgs>
  auto operator()(NewArgs &&...new_args) const -> typename std::enable_if<
      (std::tuple_size<CapturedArgs>::value + sizeof...(NewArgs) < Arity),
      Curried<Arity, Func,
              decltype(std::tuple_cat(args,
                                      std::make_tuple(std::forward<NewArgs>(
                                          new_args)...)))>>::type {
    auto merged = std::tuple_cat(
        args, std::make_tuple(std::forward<NewArgs>(new_args)...));
    return Curried<Arity, Func, decltype(merged)>{func, merged};
  }

  /**
   * Full application: enough args, invoke the function
   * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
   */
  template <typename... NewArgs>
  auto operator()(NewArgs &&...new_args) const -> typename std::enable_if<
      (std::tuple_size<CapturedArgs>::value + sizeof...(NewArgs) >= Arity),
      decltype(func::apply(func,
                     std::tuple_cat(args, std::make_tuple(std::forward<NewArgs>(
                                              new_args)...))))>::type {
    return func::apply(
        func, std::tuple_cat(
                  args, std::make_tuple(std::forward<NewArgs>(new_args)...)));
  }
};

/**
 * Converts a callable into a curried wrapper with the requested arity.
 * User Story: As functional composition code, I need currying so larger
 * runtime helpers can be partially applied in readable C++11.
 */
template <size_t Arity, typename Func> Curried<Arity, Func> curry(Func f) {
  return Curried<Arity, Func>{f, std::tuple<>{}};
}

/**
 * 6. DATA: Lazy (Deferred Evaluation)
 * Memoized deferred computation. The thunk is
 * evaluated at most once on first access via eval().
 * Construction: use the lazy() factory function.
 * Access:       use the eval() free function.
 * Note: Not thread-safe. Intended for single-thread
 * use (e.g. game thread).
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename T> struct Lazy {
  std::function<T()> thunk;
  mutable std::shared_ptr<T> cached;
};

/**
 * Wraps a thunk so it is evaluated once on first access.
 * User Story: As deferred setup code, I need lazy values so expensive work only
 * runs when a caller actually needs the result.
 */
template <typename F> auto lazy(F &&f) -> Lazy<decltype(f())> {
  return Lazy<decltype(f())>{std::forward<F>(f), nullptr};
}

/**
 * Forces a lazy value and memoizes the computed result.
 * User Story: As deferred setup code, I need an explicit force helper so lazy
 * values can be materialized through one consistent API.
 */
template <typename T> const T &eval(const Lazy<T> &lz) {
  if (!lz.cached) {
    lz.cached = std::make_shared<T>(lz.thunk());
  }
  return *lz.cached;
}

/**
 * 7. CALLABLE: MemoizedLast (Last-Input Memoization)
 * Memoizes the most recent invocation of a pure function.
 * This is the canonical primitive for selector-style
 * derived-data memoization.
 * Construction: use the memoizeLast<Signature>() factory
 * function, optionally with a custom comparator.
 * Access: call the wrapper like a normal function.
 * Note: the default comparator uses tuple equality, so
 * callers with non-comparable or overly-large inputs
 * should supply a custom comparator over a smaller key.
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename Signature> class MemoizedLast;

template <typename Result, typename... Args>
class MemoizedLast<Result(Args...)> {
public:
  typedef std::tuple<typename std::decay<Args>::type...> ArgsTuple;
  typedef std::function<bool(const ArgsTuple &, const ArgsTuple &)> Comparator;

private:
  std::function<Result(Args...)> func;
  Comparator equals;
  mutable bool hasCached;
  mutable ArgsTuple lastArgs;
  mutable std::shared_ptr<Result> lastResult;

  /**
   * Returns the default tuple-equality comparator for memoized calls.
   * User Story: As memoized selectors, I need a default comparator so repeated
   * calls can skip recomputation when arguments have not changed.
   */
  static Comparator defaultComparator() {
    return Comparator(
        [](const ArgsTuple &lhs, const ArgsTuple &rhs) { return lhs == rhs; });
  }

public:
  /**
   * Constructs an empty memoizer with the default tuple comparator.
   * User Story: As memoized selector setup, I need a default-constructed cache
   * so wrappers can be created before binding a callable.
   */
  MemoizedLast()
      : func(), equals(defaultComparator()), hasCached(false), lastArgs(),
        lastResult() {}

  /**
   * Constructs a memoizer around a callable and comparator.
   * User Story: As memoized selector setup, I need constructor injection so a
   * callable and equality policy can be bound into one cache wrapper.
   */
  MemoizedLast(std::function<Result(Args...)> inFunc,
               Comparator comparator = defaultComparator())
      : func(std::move(inFunc)), equals(std::move(comparator)),
        hasCached(false), lastArgs(), lastResult() {}

  const Result &operator()(Args... args) const {
    ArgsTuple currentArgs(std::forward<Args>(args)...);

    if (hasCached && lastResult && equals(lastArgs, currentArgs)) {
      return *lastResult;
    }

    Result computed = func::apply(func, currentArgs);
    lastArgs = currentArgs;

    if (!lastResult) {
      lastResult = std::make_shared<Result>(std::move(computed));
    } else {
      *lastResult = std::move(computed);
    }

    hasCached = true;
    return *lastResult;
  }
};

/**
 * Memoizes the last invocation of a std::function with default comparison.
 * User Story: As derived-data helpers, I need last-call memoization so cached
 * computations can be reused when inputs repeat.
 */
template <typename Signature>
MemoizedLast<Signature> memoizeLast(std::function<Signature> function) {
  return MemoizedLast<Signature>(std::move(function));
}

/**
 * Memoizes the last invocation of a generic callable with default comparison.
 * User Story: As derived-data helpers, I need memoization for generic
 * callables so caching is not limited to std::function inputs.
 */
template <typename Signature, typename F>
MemoizedLast<Signature> memoizeLast(F f) {
  return MemoizedLast<Signature>(std::function<Signature>(f));
}

/**
 * Memoizes the last invocation using a custom argument comparator.
 * User Story: As derived-data helpers, I need custom comparison so caching can
 * respect caller-defined notions of argument equality.
 */
template <typename Signature>
MemoizedLast<Signature>
memoizeLast(std::function<Signature> function,
            typename MemoizedLast<Signature>::Comparator comparator) {
  return MemoizedLast<Signature>(std::move(function), std::move(comparator));
}

/**
 * Memoizes a generic callable using a custom argument comparator.
 * User Story: As derived-data helpers, I need generic custom-comparator
 * memoization so reusable callables can control cache invalidation.
 */
template <typename Signature, typename F>
MemoizedLast<Signature>
memoizeLast(F f, typename MemoizedLast<Signature>::Comparator comparator) {
  return MemoizedLast<Signature>(std::function<Signature>(f),
                                 std::move(comparator));
}

/**
 * 8. DATA: Pipeline (Value Transformation)
 * Fluent chain for threading a value through a
 * series of pure transformations using operator|.
 * Construction: use the pipe() factory function.
 * Chaining:     use operator| with transform functions.
 * Extraction:   access the .val member directly.
 * Usage:
 *   auto add1 = [](int x) { return x + 1; };
 *   auto mul2 = [](int x) { return x * 2; };
 *   auto result = func::pipe(5) | add1 | mul2;
 *   int final = result.val;  // 12
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename T> struct Pipeline {
  T val;
};

/**
 * Starts a pipeline with an initial value.
 * User Story: As functional composition code, I need a pipeline entry point so
 * value-threading reads clearly in C++11 call sites.
 */
template <typename T> Pipeline<T> pipe(T v) {
  return Pipeline<T>{std::move(v)};
}

/**
 * operator| for chaining lvalue-backed pipelines.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename T, typename F>
auto operator|(const Pipeline<T> &p, F f) -> Pipeline<decltype(f(p.val))> {
  return Pipeline<decltype(f(p.val))>{f(p.val)};
}

/**
 * operator| for chaining move-only or ownership-transferring values.
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */
template <typename T, typename F>
auto operator|(Pipeline<T> &&p, F f)
    -> Pipeline<decltype(f(std::move(p.val)))> {
  return Pipeline<decltype(f(std::move(p.val)))>{f(std::move(p.val))};
}

/**
 * 9. CALLABLE: Composed (Function Composition)
 * Combines two functions: compose(f, g)(x) == f(g(x))
 * Construction: use the compose() factory function.
 * operator() is the C++ mechanism for callable types.
 * Usage:
 *   auto double_it = [](int x) { return x * 2; };
 *   auto add_one   = [](int x) { return x + 1; };
 *   auto both      = func::compose(add_one, double_it);
 *   int result = both(5);  // add_one(double_it(5)) = 11
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename F, typename G> struct Composed {
  F f;
  G g;

  template <typename... Args>
  auto operator()(Args &&...args) const
      -> decltype(f(g(std::forward<Args>(args)...))) {
    return f(g(std::forward<Args>(args)...));
  }
};

/**
 * Composes two functions so the result of `g` feeds into `f`.
 * User Story: As functional composition code, I need reusable composition so
 * runtime transforms can be assembled declaratively.
 */
template <typename F, typename G> Composed<F, G> compose(F f, G g) {
  return Composed<F, G>{f, g};
}

/**
 * Maps a function across the populated branch of a Maybe.
 * User Story: As optional transformations, I need fmap on Maybe so values can
 * be transformed without unwrapping and rewrapping by hand.
 */
template <typename T, typename Func>
auto fmap(const Maybe<T> &m, Func f) -> Maybe<decltype(f(m.value))> {
  typedef decltype(f(m.value)) U;
  if (!m.hasValue)
    return Maybe<U>{false, U{}};
  return Maybe<U>{true, f(m.value)};
}

/**
 * Maps a function across the success branch of an Either.
 * User Story: As result transformations, I need fmap on Either so success
 * values can be transformed while preserving failures unchanged.
 */
template <typename E, typename T, typename Func>
auto fmap(const Either<E, T> &e, Func f) -> Either<E, decltype(f(e.right))> {
  typedef decltype(f(e.right)) U;
  if (e.isLeft)
    return Either<E, U>{true, e.left, U{}};
  return Either<E, U>{false, E{}, f(e.right)};
}

/**
 * Maps a function across every element in a vector.
 * User Story: As collection transformations, I need fmap on vectors so
 * element-wise mapping follows the same functional style as Maybe and Either.
 */
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

/**
 * Chains a Maybe-producing function onto a Maybe value.
 * User Story: As optional workflows, I need bind semantics so dependent Maybe
 * operations can short-circuit naturally on missing values.
 */
template <typename T, typename Func>
auto mbind(const Maybe<T> &m, Func f) -> decltype(f(m.value)) {
  if (!m.hasValue)
    return decltype(f(m.value)){false, {}};
  return f(m.value);
}

/**
 * Chains an Either-producing function onto an Either success value.
 * User Story: As result workflows, I need bind semantics so failure branches
 * stop the pipeline while successes continue.
 */
template <typename E, typename T, typename Func>
auto ebind(const Either<E, T> &e, Func f) -> decltype(f(e.right)) {
  if (e.isLeft)
    return decltype(f(e.right)){true, e.left, {}};
  return f(e.right);
}

/**
 * Extracts a Maybe value or returns the provided default.
 * User Story: As boundary code, I need a defaulting helper so Maybe values can
 * be converted into concrete values at integration points.
 */
template <typename T> T or_else(const Maybe<T> &m, const T &def) {
  return m.hasValue ? m.value : def;
}

/**
 * Pattern matches on a Maybe with Just and Nothing callbacks.
 * User Story: As boundary code, I need pattern matching on Maybe so success and
 * empty branches can be handled declaratively.
 */
template <typename T, typename FJust, typename FNothing>
auto match(const Maybe<T> &m, FJust onJust, FNothing onNothing)
    -> decltype(onJust(m.value)) {
  if (m.hasValue)
    return onJust(m.value);
  return onNothing();
}

/**
 * Pattern matches on an Either with error and success callbacks.
 * User Story: As boundary code, I need pattern matching on Either so success
 * and failure handling stay explicit and type-safe.
 */
template <typename E, typename T, typename FLeft, typename FRight>
auto ematch(const Either<E, T> &e, FLeft onLeft, FRight onRight)
    -> decltype(onRight(e.right)) {
  if (e.isLeft)
    return onLeft(e.left);
  return onRight(e.right);
}

/**
 * 13. ValidationPipeline (Functional Validation Chain)
 * A pipeline for chaining validation functions.
 * Each validation function takes input and returns
 * Either<Error, Result>. The pipeline short-circuits
 * on first error.
 * Usage:
 *   auto pipeline = ValidationPipeline<int>()
 *       .add(validatePositive)
 *       .add(validateRange)
 *       .add(validateEven);
 *   auto result = pipeline.run(42);
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename T, typename E = std::string> class ValidationPipeline {
  std::vector<std::function<Either<E, T>(T)>> validators;

public:
  /**
   * Constructs an empty validation pipeline.
   * User Story: As validation flows, I need a blank pipeline so validators can
   * be appended incrementally before execution.
   */
  ValidationPipeline() = default;

  /**
   * Appends a validator to the pipeline.
   * User Story: As validation flows, I need validators chained fluently so
   * input rules can be assembled without mutable plumbing.
   */
  ValidationPipeline &add(std::function<Either<E, T>(T)> validator) {
    validators.push_back(std::move(validator));
    return *this;
  }

  /**
   * Runs validators in order and stops on the first error.
   * User Story: As validation flows, I need short-circuit execution so failing
   * input stops at the first invalid step.
   */
  Either<E, T> run(T val) const {
    T current = std::move(val);
    for (const auto &validator : validators) {
      auto result = validator(current);
      if (result.isLeft) {
        return result; // Short-circuit on first error
      }
      current = result.right;
    }
    return make_right(E{}, current);
  }
};

/**
 * Creates an empty validation pipeline.
 * User Story: As validation flows, I need a pipeline entry point so validators
 * can be declared and composed incrementally.
 */
template <typename T, typename E = std::string>
ValidationPipeline<T, E> validationPipeline() {
  return ValidationPipeline<T, E>();
}

/**
 * 14. ConfigBuilder (Functional Configuration Builder)
 * A builder pattern for creating immutable
 * configuration objects using functional composition.
 * Usage:
 *   auto config = configBuilder<MyConfig>()
 *       .setMember(&MyConfig::name, std::string("MyApp"))
 *       .setMember(&MyConfig::port, 8080)
 *       .build();
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

template <typename Config> class ConfigBuilder {
  std::vector<std::function<void(Config &)>> setters;

public:
  /**
   * Constructs an empty configuration builder.
   * User Story: As config assembly flows, I need a blank builder so setter
   * transforms can be accumulated before materializing a config value.
   */
  ConfigBuilder() = default;

  /**
   * Adds an explicit mutating transform to the eventual config value.
   * User Story: As config assembly flows, I need queued setters so immutable
   * config objects can be built through composable transforms.
   */
  ConfigBuilder &with(std::function<void(Config &)> setter) {
    setters.push_back(std::move(setter));
    return *this;
  }

  /**
   * Assigns a concrete member through a pointer-to-member.
   * User Story: As config assembly flows, I need member assignment helpers so
   * config values can be declared without repetitive boilerplate.
   */
  template <typename T> ConfigBuilder &setMember(T Config::*member, T value) {
    return with([member, value](Config &config) mutable {
      config.*member = std::move(value);
    });
  }

  /**
   * Delegates string-keyed assignment to config types that expose `set`.
   * User Story: As config assembly flows, I need key-based setters so dynamic
   * config types can participate in the same builder pattern.
   */
  template <typename T> ConfigBuilder &set(const std::string &key, T value) {
    return with([key, value](Config &config) mutable {
      config.set(key, std::move(value));
    });
  }

  /**
   * Materializes the configured value by replaying all queued setters.
   * User Story: As config assembly flows, I need a final build step so queued
   * transforms can produce one immutable config value.
   */
  Config build() const {
    Config config;
    for (const auto &setter : setters) {
      setter(config);
    }
    return config;
  }
};

/**
 * Creates an empty functional configuration builder.
 * User Story: As config assembly flows, I need a builder entry point so
 * immutable config values can be constructed declaratively.
 */
template <typename Config> ConfigBuilder<Config> configBuilder() {
  return ConfigBuilder<Config>();
}

/**
 * 15. TestResult (Functional Testing Result)
 * A result type for functional testing that
 * includes success/failure, messages, and
 * optional detailed information.
 * Usage:
 *   auto result = TestResult<bool>::Success(true);
 *   auto failure = TestResult<void>::Failure("Test failed");
 * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
 */

template <typename T> struct TestResult {
  bool bSuccess;
  T value;
  std::string message;
  std::unordered_map<std::string, std::string> details;

  /**
   * Builds a successful test result with an attached value.
   * User Story: As functional tests, I need a success factory so assertions can
   * return values and metadata through one result type.
   */
  static TestResult<T> Success(T value, std::string message = "") {
    return TestResult<T>{true, std::move(value), std::move(message), {}};
  }

  /**
   * Builds a failed test result with a message.
   * User Story: As functional tests, I need a failure factory so assertion
   * failures can be reported without exceptions or ad hoc flags.
   */
  static TestResult<T> Failure(std::string message) {
    return TestResult<T>{false, T{}, std::move(message), {}};
  }

  /**
   * Attaches a string detail pair to the result.
   * User Story: As functional tests, I need structured detail fields so
   * failures and successes can carry extra diagnostic context.
   */
  TestResult &withDetail(const std::string &key, const std::string &val) {
    details[key] = val;
    return *this;
  }

  /**
   * Reports whether the result represents success.
   * User Story: As functional tests, I need a direct success check so calling
   * code can branch without inspecting raw fields.
   */
  bool isSuccessful() const { return bSuccess; }

  /**
   * Returns the value as a Maybe when the test succeeded.
   * User Story: As functional tests, I need a non-throwing accessor so
   * no-exception builds can read successful values safely.
   */
  Maybe<T> TryGetValue() const { return bSuccess ? just(value) : nothing<T>(); }

  /**
   * Returns the value or fails fast when the result is unsuccessful.
   * User Story: As functional tests, I need a strict accessor so code can
   * demand a successful value when failure is unrecoverable.
   */
  T getValue() const {
    if (!bSuccess) {
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
      throw std::runtime_error("TestResult: Cannot get value from failure");
#else
      std::abort();
#endif
    }
    return value;
  }
};

/**
 * Specialization for void
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

template <> struct TestResult<void> {
  bool bSuccess;
  std::string message;
  std::unordered_map<std::string, std::string> details;

  /**
   * Builds a successful void test result.
   * User Story: As functional tests, I need a void success factory so
   * side-effect-only assertions can still return structured outcomes.
   */
  static TestResult<void> Success(std::string message = "") {
    return TestResult<void>{true, std::move(message), {}};
  }

  /**
   * Builds a failed void test result with a message.
   * User Story: As functional tests, I need a void failure factory so
   * assertion failures can be reported even when no value is returned.
   */
  static TestResult<void> Failure(std::string message) {
    return TestResult<void>{false, std::move(message), {}};
  }

  /**
   * Attaches a string detail pair to the void result.
   * User Story: As functional tests, I need structured detail fields so
   * void assertions can still surface diagnostic metadata.
   */
  TestResult &withDetail(const std::string &key, const std::string &val) {
    details[key] = val;
    return *this;
  }

  /**
   * Reports whether the void result represents success.
   * User Story: As functional tests, I need a direct success check so callers
   * can branch on pass or fail without inspecting raw fields.
   */
  bool isSuccessful() const { return bSuccess; }
};

/**
 * 16. AsyncResult (Functional Async Result Handling)
 * A type for handling async operations that
 * can succeed or fail, with support for
 * chaining and error handling.
 * Safe for async callbacks via shared state.
 * Usage:
 *   auto result = AsyncResult<int>::create([](
 *       std::function<void(int)> resolve,
 *       std::function<void(std::string)> reject) {
 *       // async operation
 *   });
 *   result.then([](int value) {
 *       // success
 *   }).catch_([](std::string error) {
 *       // failure
 *   }).execute();
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename T> class AsyncResult {
  struct State {
    std::function<void(std::function<void(T)>,
                       std::function<void(std::string)>)>
        executor;
    std::vector<std::function<void(T)>> successHandlers;
    std::vector<std::function<void(std::string)>> errorHandlers;
  };
  std::shared_ptr<State> state;

public:
  /**
   * Builds an async result from an executor callback.
   * User Story: As async composition code, I need a factory that captures an
   * executor so asynchronous work can be chained through one result type.
   */
  static AsyncResult<T>
  create(std::function<void(std::function<void(T)>,
                            std::function<void(std::string)>)>
             executor) {
    auto res = AsyncResult<T>();
    res.state->executor = std::move(executor);
    return res;
  }

  /**
   * Constructs an empty async result shell with shared state.
   * User Story: As async composition code, I need a default async container so
   * executors and handlers can be attached after construction.
   */
  AsyncResult() : state(std::make_shared<State>()) {}

  /**
   * Registers a success handler on the async result.
   * User Story: As async composition code, I need success callbacks so resolved
   * values can trigger follow-up behavior without blocking.
   */
  const AsyncResult<T> &then(std::function<void(T)> handler) const {
    state->successHandlers.push_back(std::move(handler));
    return *this;
  }

  /**
   * Registers an error handler on the async result.
   * User Story: As async composition code, I need error callbacks so rejected
   * work can surface failures through the same fluent API.
   */
  const AsyncResult<T> &catch_(std::function<void(std::string)> handler) const {
    state->errorHandlers.push_back(std::move(handler));
    return *this;
  }

  /**
   * Executes the stored async operation.
   * User Story: As async composition code, I need an explicit execute step so
   * async pipelines run only when the caller is ready to trigger them.
   */
  void execute() const {
    if (!state || !state->executor)
      return;

    /**
     * Capture state in lambda to keep it alive
     * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
     */
    auto capturedState = state;
    state->executor(
        [capturedState](T value) {
          for (const auto &handler : capturedState->successHandlers) {
            handler(value);
          }
        },
        [capturedState](std::string error) {
          for (const auto &handler : capturedState->errorHandlers) {
            handler(error);
          }
        });
  }
};

/**
 * Specialization for void
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

template <> class AsyncResult<void> {
  struct State {
    std::function<void(std::function<void()>, std::function<void(std::string)>)>
        executor;
    std::vector<std::function<void()>> successHandlers;
    std::vector<std::function<void(std::string)>> errorHandlers;
  };
  std::shared_ptr<State> state;

public:
  /**
   * Builds a void async result from an executor callback.
   * User Story: As async composition code, I need a void factory so fire-and-
   * signal tasks can share the same chaining surface as valued tasks.
   */
  static AsyncResult<void>
  create(std::function<void(std::function<void()>,
                            std::function<void(std::string)>)>
             executor) {
    auto res = AsyncResult<void>();
    res.state->executor = std::move(executor);
    return res;
  }

  /**
   * Constructs an empty void async result shell with shared state.
   * User Story: As async composition code, I need a default void container so
   * executors and callbacks can be wired together incrementally.
   */
  AsyncResult() : state(std::make_shared<State>()) {}

  /**
   * Registers a success handler on the void async result.
   * User Story: As async composition code, I need success callbacks so
   * completion-only tasks can notify later stages without return values.
   */
  const AsyncResult<void> &then(std::function<void()> handler) const {
    state->successHandlers.push_back(std::move(handler));
    return *this;
  }

  /**
   * Registers an error handler on the void async result.
   * User Story: As async composition code, I need error callbacks so void
   * tasks can surface failures through the same fluent interface.
   */
  const AsyncResult<void> &
  catch_(std::function<void(std::string)> handler) const {
    state->errorHandlers.push_back(std::move(handler));
    return *this;
  }

  /**
   * Executes the stored void async operation.
   * User Story: As async composition code, I need an explicit execute step so
   * completion-only async pipelines run on demand.
   */
  void execute() const {
    if (!state || !state->executor)
      return;

    auto capturedState = state;
    state->executor(
        [capturedState]() {
          for (const auto &handler : capturedState->successHandlers) {
            handler();
          }
        },
        [capturedState](std::string error) {
          for (const auto &handler : capturedState->errorHandlers) {
            handler(error);
          }
        });
  }
};

/**
 * 17. HttpResult (Functional Http Request Wrapper)
 * User Story: As a maintainer, I need this note so the surrounding code intent stays clear during maintenance and debugging.
 */

typedef std::int32_t HttpStatusCode;

template <typename T> struct HttpResult {
  bool bSuccess;
  HttpStatusCode ResponseCode;
  T data;
  std::string error;

  /**
   * Builds a successful HTTP result wrapper.
   * User Story: As HTTP adapter code, I need a success factory so decoded
   * payloads carry both data and transport status through one value.
   */
  static HttpResult<T> Success(T d, HttpStatusCode code = 200) {
    return HttpResult<T>{true, code, std::move(d), ""};
  }

  /**
   * Builds a failed HTTP result wrapper.
   * User Story: As HTTP adapter code, I need a failure factory so transport or
   * decoding errors can move through the same result channel as successes.
   */
  static HttpResult<T> Failure(std::string e, HttpStatusCode code = 0) {
    return HttpResult<T>{false, code, T{}, std::move(e)};
  }
};

/**
 * 18. AsyncChain (Helpers for chaining AsyncResults)
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

namespace AsyncChain {
/**
 * Chains one AsyncResult into another async-producing transformation.
 * User Story: As async thunk composition, I need async chaining so one async
 * result can feed into the next without nested callback plumbing.
 */
template <typename T, typename U, typename F>
auto then(const AsyncResult<T> &res, F f) -> AsyncResult<U> {
  return AsyncResult<U>::create(
      [res, f](std::function<void(U)> resolve,
               std::function<void(std::string)> reject) {
        res.then([f, resolve, reject](T val) {
             f(val).then(resolve).catch_(reject).execute();
           })
            .catch_(reject)
            .execute();
      });
}
} // namespace AsyncChain

/**
 * 19. Dispatcher (Dictionary-Based Typed Dispatch)
 * A lookup table mapping keys to handler functions.
 * Returns Maybe<Result> from dispatch — just(handler())
 * if the key exists, nothing<Result>() if not.
 * Construction: use createDispatcher<Key, Result>() with
 *               a vector of {key, handler} pairs.
 * Dispatch:     use the dispatch() free function.
 * Usage:
 *   auto d = func::createDispatcher<FString, int>({
 *       {TEXT("a"), []() { return 1; }},
 *       {TEXT("b"), []() { return 2; }},
 *   });
 *   auto result = func::dispatch(d, TEXT("a")); // just(1)
 *   auto miss   = func::dispatch(d, TEXT("z")); // nothing
 * User Story: As a maintainer, I need this step note so I can follow the scenario progression and reason about the expected state changes.
 */

template <typename Key, typename Result> struct Dispatcher {
  std::unordered_map<Key, std::function<Result()>> table;
};

/**
 * Builds a dispatcher table from key-to-handler entries.
 * User Story: As keyed dispatch flows, I need a typed dispatcher table so
 * string or enum keys can resolve handlers declaratively.
 */
template <typename Key, typename Result>
Dispatcher<Key, Result> createDispatcher(
    std::vector<std::pair<Key, std::function<Result()>>> entries) {
  Dispatcher<Key, Result> d;
  for (size_t i = 0; i < entries.size(); ++i) {
    d.table[entries[i].first] = entries[i].second;
  }
  return d;
}

/**
 * Looks up and invokes a handler by key when one exists.
 * User Story: As keyed dispatch flows, I need dispatch to return Maybe so
 * missing handlers do not require exceptions or sentinels.
 */
template <typename Key, typename Result>
Maybe<Result> dispatch(const Dispatcher<Key, Result> &d, const Key &key) {
  typename std::unordered_map<Key, std::function<Result()>>::const_iterator it =
      d.table.find(key);
  if (it != d.table.end()) {
    return just(it->second());
  }
  return nothing<Result>();
}

/**
 * Reports whether a dispatcher has a handler for the given key.
 * User Story: As keyed dispatch flows, I need a presence check so callers can
 * branch before invoking optional handlers.
 */
template <typename Key, typename Result>
bool has(const Dispatcher<Key, Result> &d, const Key &key) {
  return d.table.find(key) != d.table.end();
}

/**
 * Returns every key currently registered in the dispatcher.
 * User Story: As keyed dispatch flows, I need access to registered keys so
 * tools and tests can inspect available handlers.
 */
template <typename Key, typename Result>
std::vector<Key> keys(const Dispatcher<Key, Result> &d) {
  std::vector<Key> result;
  for (typename std::unordered_map<Key, std::function<Result()>>::const_iterator
           it = d.table.begin();
       it != d.table.end(); ++it) {
    result.push_back(it->first);
  }
  return result;
}

/**
 * 20. multi_match (Multi-Case Value-Based Pattern Matching)
 * Tries each predicate/handler pair in order.
 * Returns just(handler(value)) from the first predicate
 * that returns true. Returns nothing<R>() if no match.
 * Helper factories:
 *   wildcard<T>()   — always-true predicate (default arm)
 *   equals<T>(val)  — value-equality predicate
 *   when<T,R>(pred, handler) — construct a MatchCase
 * Usage:
 *   auto result = func::multi_match<FString, int>(
 *       input,
 *       {
 *           func::when<FString, int>(
 *               func::equals<FString>(TEXT("a")),
 *               [](const FString&) { return 1; }),
 *           func::when<FString, int>(
 *               func::wildcard<FString>(),
 *               [](const FString&) { return 0; }),
 *       });
 * User Story: As a maintainer, I need this section note so related declarations and logic stay easy to locate.
 */

template <typename T, typename R> struct MatchCase {
  std::function<bool(const T &)> predicate;
  std::function<R(const T &)> handler;
};

/**
 * Builds a match case from a predicate and a handler.
 * User Story: As pattern-matching helpers, I need reusable cases so matching
 * logic can be declared independently from evaluation.
 */
template <typename T, typename R>
MatchCase<T, R> when(std::function<bool(const T &)> pred,
                     std::function<R(const T &)> handler) {
  MatchCase<T, R> c;
  c.predicate = std::move(pred);
  c.handler = std::move(handler);
  return c;
}

/**
 * Returns a predicate that matches every input.
 * User Story: As pattern-matching helpers, I need a wildcard predicate so
 * match lists can declare explicit default branches.
 */
template <typename T> std::function<bool(const T &)> wildcard() {
  return [](const T &) { return true; };
}

/**
 * Returns a predicate that matches a specific expected value.
 * User Story: As pattern-matching helpers, I need equality predicates so case
 * lists can express direct value matches declaratively.
 */
template <typename T> std::function<bool(const T &)> equals(T expected) {
  return [expected](const T &value) { return value == expected; };
}

/**
 * Evaluates match cases in order and returns the first successful result.
 * User Story: As pattern-matching helpers, I need ordered case evaluation so
 * callers can express prioritized matching without manual branching.
 */
template <typename T, typename R>
Maybe<R> multi_match(const T &value, const std::vector<MatchCase<T, R>> &cases) {
  for (size_t i = 0; i < cases.size(); ++i) {
    if (cases[i].predicate(value)) {
      return just(cases[i].handler(value));
    }
  }
  return nothing<R>();
}

/**
 * Lifts a nullable pointer into a Maybe.
 * User Story: As boundary helpers, I need nullable pointers lifted into Maybe
 * so pointer-based APIs can join functional pipelines safely.
 */
template <typename T> Maybe<T> from_nullable(const T *ptr) {
  if (ptr) {
    return just(*ptr);
  }
  return nothing<T>();
}

/**
 * Lifts a value into a Maybe when the caller marks it as valid.
 * User Story: As boundary helpers, I need validity-flag lifting so non-pointer
 * APIs can still participate in Maybe-based flows.
 */
template <typename T> Maybe<T> from_nullable_value(T value, bool valid) {
  if (valid) {
    return just(std::move(value));
  }
  return nothing<T>();
}

/**
 * Extracts a Maybe value or aborts the current boundary with an error message.
 * User Story: As boundary code, I need a fail-fast extractor so required Maybe
 * values can be enforced at integration boundaries.
 */
template <typename T>
T require_just(const Maybe<T> &m, const std::string &errorMsg) {
  if (m.hasValue) {
    return m.value;
  }
#if defined(__cpp_exceptions) || defined(__EXCEPTIONS) || defined(_CPPUNWIND)
  throw std::runtime_error(errorMsg);
#else
  std::abort();
#endif
}

} // namespace func

#endif // FUNCTIONAL_CORE_HPP
