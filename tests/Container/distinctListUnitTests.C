#include <sawyer/DistinctList.h>

using namespace Sawyer::Container;

// Test that things compile
template<typename T, typename U>
static void
testCompile() {
    typedef DistinctList<T> TList;
    typedef DistinctList<U> UList;

    // Implicit convertion must exist
    {
        T t(0);
        U u(0);
        t = u;
    }

    // List constructors
    {
        TList t1;
        TList t2(t1);
        UList u1;
        TList t3(u1);
        const UList u2(u1);
        TList t4(u2);
    }

    // List operators
    {
        TList t1;
        t1 = t1;

        UList u1;
        t1 = u1;

        const UList u2;
        t1 = u2;
    }

    // List methods
    {
        T t(0);
        TList t1;
        t1.clear();
        t1.isEmpty();
        t1.size();
        t1.exists(t);
        t = t1.front();
        t = t1.back();
        t1.items();
        t1.pushFront(t);
        t1.pushBack(t);
        t = t1.popFront();
        t = t1.popBack();
        t1.erase(t);

        const TList t2;
        t2.isEmpty();
        t2.size();
        t2.exists(t);
        t = t2.front();
        t = t2.back();
        t1.items();
    }
}

int
main() {
    testCompile<int, int>();
    testCompile<double, int>();
}
