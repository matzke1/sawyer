#ifndef Sawyer_Assert_H
#define Sawyer_Assert_H

#include <string>

// If SAWYER_NDEBUG is defined then some of the macros defined in this header become no-ops.  For interoperability with the
// more standard NDEBUG symbol, we define SAWYER_NDEBUG if NDEBUG is defined.
#ifdef NDEBUG
#undef SAWYER_NDEBUG
#define SAWYER_NDEBUG
#endif



#ifdef SAWYER_NDEBUG

#define require(expr) /*void*/
#define require2(expr, note) /*void*/
#define forbid(expr) /*void*/
#define forbid2(expr, note) /*void*/
#define not_null(expr) expr
#define not_null2(expr, note) expr

#else

/** Requires that @p expr be true. If @p expr is false then fail by calling Sawyer::Assert::fail(). */
#define require(expr)                                                                                                          \
    ((expr) ?                                                                                                                  \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "required: "__STRING(expr),                                                   \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, ""))

/** Requires that @p expr be true.  If @p expr is false then fail by calling Sawyer::Assert::fail() with @p note as part
 *  of the description. */
#define require2(expr, note)                                                                                                   \
    ((expr) ?                                                                                                                  \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "required: "__STRING(expr),                                                   \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, (note)))

/** Requires that @p expr be false.  If @p expr is true then fail by calling Sawyer::Assert::faile(). */
#define forbid(expr)                                                                                                           \
    (!(expr) ?                                                                                                                 \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed", "forbidden: "__STRING(expr),                                                  \
                             __FILE__, __LINE__, __PRETTY_FUNCTION__, ""))

/** Requires that @p expr be false.  If @p expr is true then fail by calling Sawyer::Assert::fail() with @p note as part
 *  of the description. */
#define forbid2(expr, note)                                                                                                    \
    (!(expr) ?                                                                                                                 \
        static_cast<void>(0) :                                                                                                 \
        Sawyer::Assert::fail("assertion failed",                                                                               \
                             "forbidden: "__STRING(expr), __FILE__, __LINE__, __PRETTY_FUNCTION__, (note)))

/** Requires that @p expr be a non-null pointer.  If @p expr is null then fail by calling Sawyer::Assert::fail().  This
 *  function returns @p expr (when it returns). */
#define not_null(expr)                                                                                                         \
    Sawyer::Assert::not_null_impl(expr, __STRING(expr), __FILE__, __LINE__, __PRETTY_FUNCTION__, "must not be null")

/** Requires that @p expr be a non-null pointer.  If @p expr is null then fail by calling Sawyer::Assert::fail() with
 *  @p note as part of the description.  This function returns @p expr (when it returns). */
#define not_null2(expr, note)                                                                                                  \
    Sawyer::Assert::not_null_impl(expr, __STRING(expr), __FILE__, __LINE__, __PRETTY_FUNCTION__, (note))

#endif

/** Always fails by calling Sawyer::Assert::fail().  Does not return to caller.  Use this macro to mark code that shouldn't
 *  be reached for some reason.  This macro is not disabled by SAWYER_NDEBUG. */
#define not_reachable(reason)                                                                                                  \
    Sawyer::Assert::fail("reached impossible state", NULL,                                                                     \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__, (reason));

/** Always fails by calling Sawyer::Assert::fail().  Does not return to caller.  Use this macro to indicate where
 *  functionality has not been implemented yet. This macro is not disabled by SAWYER_NDEBUG. */
#define not_implemented(reason)                                                                                                \
    Sawyer::Assert::fail("not implemented yet", NULL,                                                                          \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__, (reason))

/** Always fails by calling Sawyer::Assert::fail().  Does not return to caller. Use this macro to indicate where
 *  functionality has not been implemented yet.  This is an alias for not_implemented() but is useful in IDEs that
 *  match the "TODO" string.  This macro is not disabled by SAWYER_NDEBUG. */
#define TODO(reason)                                                                                                           \
    Sawyer::Assert::fail("not implemented yet", NULL,                                                                          \
                         __FILE__, __LINE__, __PRETTY_FUNCTION__, (reason))

namespace Sawyer {
namespace Assert {

/** Cause immediate failure.  This function is the low-level function called by most of the other Sawyer::Assert macros
 *  when an assertion fails. Calls to this function do not return. */
void fail(const char *mesg, const char *expr,
          const char *filename, unsigned linenum, const char *funcname,
          const std::string &note) __attribute__((noreturn));

/** Implementation for the not_null() macro when that macro is not disabled by SAWYER_NDEBUG. */
template<typename Pointer>
inline Pointer not_null_impl(Pointer pointer, const char *expr,
                             const char *filename, unsigned linenum, const char *funcname,
                             const std::string &note) {
    if (!pointer)
        fail("null pointer", expr, filename, linenum, funcname, note);
    return pointer;
}

} // namespace
} // namespace

#endif
