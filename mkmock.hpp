// Part of Measurement Kit <https://measurement-kit.github.io/>.
// Measurement Kit is free software under the BSD license. See AUTHORS
// and LICENSE for more information on the copying conditions.
#ifndef MEASUREMENT_KIT_MKMOCK_HPP
#define MEASUREMENT_KIT_MKMOCK_HPP

/// @file mkmock.hpp
/// This file contains common macro used for testing and mocking.

#include <exception>
#include <mutex>

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

#else

// This version of MKMOCK_HOOK allows us to override the value of @p Variable
// using the mock identified by @p Tag in specific cases.
#define MKMOCK_HOOK(Tag, Variable)                  \
  do {                                              \
    mkmock_##Tag *inst = mkmock_##Tag::singleton(); \
    std::unique_lock<std::mutex> _{inst->mutex};    \
    if (inst->enabled) {                            \
      Variable = inst->value;                       \
    }                                               \
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
    std::mutex mutex;                  \
  }

/// MKMOCK_WITH_ENABKED_HOOK runs @p CodeSnippet with the mock identified by
/// @p Tag enabled and with its value set to @p MockedValue. When leaving this
/// macro will disable the mock and set its value back to the old value, even
/// when @p CodeSnippet throws an exception. Exceptions will be rethrown by
/// this macro once the previous state has been reset.
#define MKMOCK_WITH_ENABLED_HOOK(Tag, MockedValue, CodeSnippet) \
  do {                                                          \
    mkmock_##Tag *inst = mkmock_##Tag::singleton();             \
    decltype(inst->value) SavedValue{};                         \
    {                                                           \
      std::unique_lock<std::mutex> _{inst->mutex};              \
      SavedValue = inst->value;                                 \
      inst->value = MockedValue;                                \
      inst->enabled = true;                                     \
    }                                                           \
    std::exception_ptr saved_exc;                               \
    try {                                                       \
      CodeSnippet                                               \
    } catch (const std::exception &) {                          \
      saved_exc = std::current_exception();                     \
    }                                                           \
    {                                                           \
      std::unique_lock<std::mutex> _{inst->mutex};              \
      inst->enabled = false;                                    \
      inst->value = SavedValue;                                 \
    }                                                           \
    if (saved_exc) {                                            \
      std::rethrow_exception(saved_exc);                        \
    }                                                           \
  } while (0)

#endif  // MEASUREMENT_KIT_MKMOCK_HPP
