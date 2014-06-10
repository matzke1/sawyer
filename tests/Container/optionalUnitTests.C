#include <sawyer/Optional.h>

#include <iostream>
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
    ASSERT_always_require(!x);
    ASSERT_always_require(x.getOrElse(5)==5);

    x = 0;
    ASSERT_always_require(x);
    ASSERT_always_require(*x==0);
    ASSERT_always_require(x.getOrElse(5)==0);

    Optional<int> x2 = x;
    ASSERT_always_require(x2);
    ASSERT_always_require(*x2==0);

    x = x2;
    ASSERT_always_require(x);
    ASSERT_always_require(*x==0);

    // x == x;                                             // should be an error
    // x + x;                                              // should be an error
    // int y = x;                                          // should be an error

    // This works, but because of operator<<; nothing we can do about it.
    std::cout <<x <<"\n";

}
