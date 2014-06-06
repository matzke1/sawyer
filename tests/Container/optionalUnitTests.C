#include <sawyer/Optional.h>

#include <sawyer/Assert.h>

using namespace Sawyer;

int main() {

    Optional<int> x;
    ASSERT_always_forbid2(x, "default constructed Optional should be false");
    try {
        *x;
        ASSERT_not_reachable("should have thrown \"dereferencing nothing\"");
    } catch (const std::domain_error&) {
    }

    x = 0;
    ASSERT_always_require(x);
    ASSERT_always_require(*x==0);

    Optional<int> x2 = x;
    ASSERT_always_require(x2);
    ASSERT_always_require(*x2==0);

    x = x2;
    ASSERT_always_require(x);
    ASSERT_always_require(*x==0);

    // x == x;                                             // should be an error
    // x + x;                                              // should be an error
    // int y = x;                                          // should be an error

    std::cout <<x;

}
