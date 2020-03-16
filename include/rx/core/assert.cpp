#include <stdlib.h> // abort
#include <stdarg.h> // va_{list,start,end,copy}
#include <stdio.h> // vsnprintf

#if defined(RX_BREAK_ON_ASSERT)
#if defined(RX_PLATFORM_WINDOWS)
#include <Windows.h>   // DebugBreak
#elif defined(RX_PLATFORM_LINUX)
#include <signal.h> // raise, SIGINT
#endif
#endif

#include "rx/core/log.h" // RX_LOG, rx::log
#include "rx/core/abort.h" // rx::abort

RX_LOG("assert", logger);

namespace rx {

[[noreturn]]
void assert_fail(const char* _expression,
  const source_location& _source_location, const char* _message, ...)
{
  va_list va;
  va_start(va, _message);

  // Determine how much memory is needed to format the assertion message.
  va_list ap;
  va_copy(ap, va);
  const int length = vsnprintf(nullptr, 0, _message, ap);
  va_end(ap);

  // Construct assertion message.
  string contents;
  contents.resize(length);
  vsnprintf(contents.data(), contents.size() + 1, _message, va);
  va_end(va);

  logger(log::level::k_error, "Assertion failed: %s (%s:%d %s) \"%s\"",
    _expression, _file, _line, _function, utility::move(contents));

#if defined(RX_BREAK_ON_ASSERT)
#if defined(RX_PLATFORM_WINDOWS)
  DebugBreak();
#elif defined(RX_PLATFORM_LINUX)
  raise(SIGINT);
#endif
#else
  abort(contents.data());
#endif
}

} // namespace rx
