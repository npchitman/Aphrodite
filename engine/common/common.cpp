#include "common.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#define BACKWARD_HAS_DW 1
#define BACKWARD_HAS_BACKTRACE_SYMBOL 1
#include <backward-cpp/backward.hpp>

namespace aph
{
backward::SignalHandling sh;

std::string TracedException::_get_trace()
{
    std::ostringstream ss;

    backward::StackTrace    stackTrace;
    backward::TraceResolver resolver;
    stackTrace.load_here();
    resolver.load_stacktrace(stackTrace);

    ss << "\n\n == backtrace == \n\n";
    for(std::size_t i = 0; i < stackTrace.size(); ++i)
    {
        const backward::ResolvedTrace trace = resolver.resolve(stackTrace[i]);

        ss << "#" << i << " at " << trace.object_function << "\n";
    }
    ss << "\n == backtrace == \n\n";

    return ss.str();
}
}  // namespace aph
