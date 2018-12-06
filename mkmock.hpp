// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_MKMOCK_HPP
#define MEASUREMENT_KIT_MKMOCK_HPP

/// @file mkmock.hpp
/// This file contains common macro used for testing and mocking.

#include <exception>
#include <mutex>
#include <utility>

#ifndef MKMOCK_HOOK_ENABLE

/// MKMOCK_HOOK provides you a hook to override the value of @p Variable
/// and identified by the @p Tag unique tag. Use this macro like:
///
/// ```C
/// int rv = call_some_api();
/// MKMOCK_HOOK(call_some_api, rv);
/// if (rv != 0) {
///   // Handle error
///   return;
/// }
/// ````
///
/// When MKMOCK_HOOK_ENABLE is not defined, this macro expands to
/// nothing. Otherwise, it generates code that you could use in the
/// unit tests to inject failures in specific code places.
#define MKMOCK_HOOK(Tag, Variable)  // Nothing

/// MKMOCK_HOOK_ALLOC is like MKMOCK_HOOK except that it also defines
/// a deleter to be called to free allocated memory when we want to
/// make a successful memory allocation look like a failure. Without
/// using this macro, `asan` will complain about a memory leak.
#define MKMOCK_HOOK_ALLOC(Tag, Variable, Deleter)  // Nothing

#else

// This version of MKMOCK_HOOK allows us to override the value of @p Variable
// using the mock identified by @p Tag in specific cases.
#define MKMOCK_HOOK(Tag, Variable)                         \
  do {                                                     \
    mkmock_##Tag *inst = mkmock_##Tag::singleton();        \
    std::unique_lock<std::recursive_mutex> _{inst->mutex}; \
    if (inst->enabled) {                                   \
      Variable = inst->value;                              \
    }                                                      \
  } while (0)

// This version of MKMOCK_HOOK allows us to override the value of @p Variable
// using the mock identified by @p Tag in specific cases. If the memory that
// has been allocated is non NULL, Deleter will be invoked to clean it before
// overriding the returned value so that there are no memory leaks.
#define MKMOCK_HOOK_ALLOC(Tag, Variable, Deleter)          \
  do {                                                     \
    mkmock_##Tag *inst = mkmock_##Tag::singleton();        \
    std::unique_lock<std::recursive_mutex> _{inst->mutex}; \
    if (inst->enabled) {                                   \
      if (Variable != nullptr) {                           \
        Deleter(Variable);                                 \
      }                                                    \
      Variable = inst->value;                              \
    }                                                      \
  } while (0)

#endif  // !MKMOCK_HOOK_ENABLE

/// MKMOCK_DEFINE_HOOK defines a hook with tag @p Tag and type @p Type. This
/// macro should be used in the unit tests source file only.
#define MKMOCK_DEFINE_HOOK(Tag, Type)  \
  class mkmock_##Tag {                 \
   public:                             \
    static mkmock_##Tag *singleton() { \
      static mkmock_##Tag instance;    \
      return &instance;                \
    }                                  \
                                       \
    bool enabled = false;              \
    Type value = {};                   \
    Type saved_value = {};             \
    std::exception_ptr saved_exc;      \
    std::recursive_mutex mutex;        \
  }

/// MKMOCK_WITH_ENABKED_HOOK runs @p CodeSnippet with the mock identified by
/// @p Tag enabled and with its value set to @p MockedValue. When leaving this
/// macro will disable the mock and set its value back to the old value, even
/// when @p CodeSnippet throws an exception. Exceptions will be rethrown by
/// this macro once the previous state has been reset.
#define MKMOCK_WITH_ENABLED_HOOK(Tag, MockedValue, CodeSnippet)   \
  /* Implementation note: this macro is written such that it   */ \
  /* can call itself without triggering compiler warning about */ \
  /* reusing the same names in a inner scope.                  */ \
  do {                                                            \
    {                                                             \
      mkmock_##Tag *inst = mkmock_##Tag::singleton();             \
      inst->mutex.lock(); /* Barrier for other threads */         \
      inst->saved_exc = {};                                       \
      inst->saved_value = inst->value;                            \
      inst->value = MockedValue;                                  \
      inst->enabled = true;                                       \
    }                                                             \
    try {                                                         \
      CodeSnippet                                                 \
    } catch (const std::exception &) {                            \
      mkmock_##Tag *inst = mkmock_##Tag::singleton();             \
      inst->saved_exc = std::current_exception();                 \
    }                                                             \
    {                                                             \
      mkmock_##Tag *inst = mkmock_##Tag::singleton();             \
      inst->enabled = false;                                      \
      inst->value = inst->saved_value;                            \
      inst->saved_value = {};                                     \
      std::exception_ptr saved_exc;                               \
      std::swap(saved_exc, inst->saved_exc);                      \
      inst->mutex.unlock(); /* Allow another thread. */           \
      if (saved_exc) {                                            \
        std::rethrow_exception(saved_exc);                        \
      }                                                           \
    }                                                             \
  } while (0)

#endif  // MEASUREMENT_KIT_MKMOCK_HPP
