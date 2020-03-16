#include <stdarg.h> // va_{list,start,end,copy}
#include <stdio.h>  // vsnprintf
#include <stdlib.h> // abort

#if defined(RX_BREAK_ON_ASSERT)
#if defined(RX_PLATFORM_WINDOWS)
#include <Windows.h> // DebugBreak
#elif defined(RX_PLATFORM_LINUX)
#include <signal.h> // raise, SIGINT
#endif
#endif

#include "rx/core/abort.h" // rx::abort
#include "rx/core/log.h"   // RX_LOG, rx::log

RX_LOG("assert", logger);

namespace rx {

    [[noreturn]] void assert_fail(const char* _expression, const source_location& _source_location, const char* _message, ...) {
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

        // Write the assertion failure to the logger.
        ::logger->error("Assertion failed: %s (%s:%d %s) \"%s\"",
                      _expression,
                      _source_location.file(),
                      _source_location.line(),
                      _source_location.function(),
                      contents);

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
