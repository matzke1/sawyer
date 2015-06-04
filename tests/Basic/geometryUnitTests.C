#include <Sawyer/Geometry.h>

using namespace Sawyer::Geometry;

static void
pointConstructors() {
    std::cout <<"point constructors:\n";

    Point2d x;
    std::cout <<"  x = " <<x <<"\n";

    Point2d d2 = point2(1.1, 2.2);
    std::cout <<"  d2 = " <<d2 <<"\n";

    Point2i i2 = point2(3, 4);
    std::cout <<"  i2 = " <<i2 <<"\n";

    Point3f f3 = point3(1.2f, 3.4f, 5.6f);
    std::cout <<"  f3 = " <<f3 <<"\n";
}

static void
pointConversion() {
    std::cout <<"point conversion:\n";

    Point2i i2 = point2(200, 200);
    Point2d d2 = i2.as<double>();
    std::cout <<"  i2=" <<i2 <<", d2=" <<d2 <<"\n";

    double n1 = i2.norm();
    double n2 = i2.as<double>().norm();
    std::cout <<"  n1=" <<n1 <<", n2=" <<n2 <<"\n";
}

static void
boxConstructors() {
    std::cout <<"box constructors:\n";
    Box2i empty;
    std::cout <<"  empty = " <<empty <<"\n";

    Box2d d2 = box2(point2(0.0, 1.0), point2(10.0, 11.0));
    std::cout <<"  d2 = " <<d2 <<"\n";
}

int
main() {
    Sawyer::initializeLibrary();
    pointConstructors();
    pointConversion();
    boxConstructors();
}

