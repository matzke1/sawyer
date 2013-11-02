#include "Sawyer.h"
#include "Assert.h"

#include <cstdlib>

namespace Sawyer {
namespace Assert {

void
fail(const char *mesg, const char *expr, const char *filename, unsigned linenum, const char *funcname, const std::string &note)
{
    log[FATAL] <<mesg <<":\n";
    if (filename && *filename)
        log[FATAL] <<"  " <<filename <<":" <<linenum <<"\n";
    if (funcname && *funcname)
        log[FATAL] <<"  " <<funcname <<"\n";
    if (expr && *expr)
        log[FATAL] <<"  " <<expr <<"\n";
    if (!note.empty())
        log[FATAL] <<"  " <<note <<"\n";
    abort();
}

} // namespace
} // namespace
