#ifndef Sawyer_Assert_H
#define Sawyer_Assert_H

#include <string>

#define require(expr)                                                                                                          \
    ((expr) ?                                                                                                                  \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "required: "__STRING(expr),                                                   \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, ""))

#define require2(expr, note)                                                                                                   \
    ((expr) ?                                                                                                                  \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "required: "__STRING(expr),                                                   \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, (note)))

#define forbid(expr)                                                                                                           \
    (!(expr) ?                                                                                                                 \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "forbidden: "__STRING(expr),                                                  \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, ""))

#define forbid2(expr, note)                                                                                                    \
    (!(expr) ?                                                                                                                 \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed",                                                                               \
                             "forbidden: "__STRING(expr), __FILE__, __LINE__, __PRETTY_FUNCTION__, (note)))

#define not_null(expr)                                                                                                          \
    ((expr)!=0 ?                                                                                                               \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "expression: "__STRING(expr),                                                 \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, "must not be null"))

#define not_reachable(reason)                                                                                                  \
    Sawyer::Assert::fail("reached impossible state", NULL,                                                                     \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__, (reason));

#define not_implemented(reason)                                                                                                \
    Sawyer::Assert::fail("not implemented yet", NULL,                                                                          \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__, (reason))
#define TODO(reason)                                                                                                           \
    Sawyer::Assert::fail("not implemented yet", NULL,                                                                          \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__, (reason))

namespace Sawyer {
namespace Assert {

void fail(const char *mesg, const char *expr,
          const char *filename, unsigned linenum, const char *funcname,
          const std::string &note) __attribute__((noreturn));

} // namespace
} // namespace

#endif
