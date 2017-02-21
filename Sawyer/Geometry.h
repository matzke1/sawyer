#ifndef Sawyer_Coord_H
#define Sawyer_Coord_H

#include <Sawyer/Assert.h>
#include <cmath>
#if __cplusplus < 201103L && defined(_MSC_VER)
#include <float.h> // for _isnan
#endif

namespace Sawyer {

/** Classes to support geometry. */
namespace Geometry {

template<typename T>
struct NumberTraits {
    static T initialValue() { return 0; }
    static bool isValid(T) { return true; }
};

template<>
struct NumberTraits<double> {
    static double initialValue() { return NAN; }
#if __cplusplus >= 201103L
    static bool isValid(double n) { return !std::isnan(n); }
#elif defined(_MSC_VER)
    static bool isValid(double n) { return _isnan(n); }
#else
    static bool isValid(double n) { return !isnan(n); }
#endif
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Points
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**  Point in Euclidean space.
 *
 *   A point in an <em>n</em>-dimensional Euclidean space.  The @p Dimensionality is a positive value usually two or
 *   greater. The @p NumericType is the type used to store each coordinate of the point.  Euclidean spaces are defined on real
 *   numbers, but this class can also use integral types although not all operations will make much sense (e.g., the length of
 *   an @c int vector will be computed using @c int for the intermediate and final result even though the Euclidean length of
 *   an integer point cannot always be expressed as an integer). */
template<size_t Dimensionality, typename NumericType>
class EuclideanPoint {
    NumericType coords_[Dimensionality];

public:
    /** Point at the origin.
     *
     *  Constructs a point whose coordinates are all zero. */
    EuclideanPoint() {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] = NumberTraits<NumericType>::initialValue();
    }

    /** Coordinate of a point.
     *
     *  Returns the coordinate of the specified dimension of @p this point.  If @p this is non-const then the return value is a
     *  non-const reference so that the coordinate can be modified in place like this:
     *
     * @code
     *  EuclideanPoint<2,double> point;
     *  point(0) = 5.3;
     *  point(1) = 3.8;
     * @endcode
     *
     * @{ */
    NumericType coord(size_t dimension) const {
        ASSERT_require(dimension < Dimensionality);
        return coords_[dimension];
    }
    NumericType& coord(size_t dimension) {
        ASSERT_require(dimension < Dimensionality);
        return coords_[dimension];
    }
    NumericType operator[](size_t dimension) const {
        return coord(dimension);
    }
    NumericType& operator[](size_t dimension) {
        return coord(dimension);
    }
    /** @} */

    /** Tests whether point is valid.
     *
     *  Returns true if all coordinates are valid according to @c NumberTraits::isValid. */
    bool isValid() const {
        for (size_t i=0; i<Dimensionality; ++i) {
            if (!NumberTraits<NumericType>::isValid(coords_[i]))
                return false;
        }
        return true;
    }
    
    /** Convert a point to a new numeric type.
     *
     *  This function is often used as an intermediary when one is trying to operate on an integral point.
     *
     *  Example: Lets assume that the dimensions of an image are measured in pixels with an @c int and we want to find the
     *  diagonal distance across the image.  The diagonal will most likely be a non-integral value:
     *
     * @code
     *  EuclideanPoint<2,int> iconSize = point2(200, 200);
     *  std::cout <<iconSize.norm() <<"\n";              // prints 282
     *  std::cout <<iconSize.as<double>().norm() <<"\n"; // prints 282.843
     * @endcode */
    template<typename NewType>
    EuclideanPoint<Dimensionality, NewType> as() const {
        EuclideanPoint<Dimensionality, NewType> result;
        for (size_t i=0; i<Dimensionality; ++i)
            result.coord(i) = coords_[i];
        return result;
    }
    
    /** In-place coordinate-wise addition.
     *
     *  For each coordinate of @p this point, add the corresponding coordinate of the @p other point. */
    EuclideanPoint& operator+=(const EuclideanPoint &other) {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] += other.coords_[i];
        return *this;
    }

    /** In-place coordinate-wise addition.
     *
     *  For each coordinate of @p this point, add the specified @p number. */
    EuclideanPoint& operator+=(NumericType number) {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] += number;
        return *this;
    }
    
    /** In-place coordinate-wise subtraction.
     *
     *  For each coordinate of @p this point, subtract the corresponding coordinate of the @p other point. */
    EuclideanPoint& operator-=(const EuclideanPoint &other) {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] -= other.coords_[i];
        return *this;
    }

    /** In-place coordinate-wise subtraction.
     *
     *  For each coordinate of @p this point, subtract the specified @p number. */
    EuclideanPoint& operator-=(NumericType number) {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] -= number;
        return *this;
    }

    /** In-place coordinate-wise multiplication.
     *
     *  For each coordinate of @p this point, multiply by the specified @p number. */
    EuclideanPoint& operator*=(NumericType number) {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] *= number;
        return *this;
    }

    /** In-place coordinate-wise division.
     *
     *  For each coordinate of @p this point, divide by the specified @p number. */
    EuclideanPoint& operator/=(NumericType number) {
        for (size_t i=0; i<Dimensionality; ++i)
            coords_[i] /= number;
        return *this;
    }

    /** Coordinate-wise addition.
     *
     *  Returns a new point, each coordinate of which is the sum of the corresponding coordinate in @p this point and the @p
     *  other point. */
    EuclideanPoint operator+(const EuclideanPoint &other) const {
        EuclideanPoint result = *this;
        return result += other;
    }

    /** Coordinate-wise addition.
     *
     *  Returns a new point, each coordinate of which is the sum of the corresponding coordinate in @p this point and the
     *  specified @p number. */
    EuclideanPoint operator+(NumericType number) const {
        EuclideanPoint result = *this;
        return result += number;
    }

    /** Coordinate-wise subtraction.
     *
     *  Returns a new point, each coordinate of which is the difference between the corresponding coordinate of @p this point
     *  and the @p other point. */
    EuclideanPoint operator-(const EuclideanPoint &other) const {
        EuclideanPoint result = *this;
        return result -= other;
    }

    /** Coordinate-wise subtraction.
     *
     *  Returns a new point, each coordinate of which is the difference between the corresponding coordinate of @p this point
     *  and the specified @p number. */
    EuclideanPoint operator-(NumericType number) const {
        EuclideanPoint result = *this;
        return result -= number;
    }

    /** Coordinate-wise multiplication.
     *
     *  Returns this coordinate multiplied by a scalar. Each coordinate of the returned point is the product of the
     *  corresponding coordinate of @p this point and the specified @p number. */
    EuclideanPoint operator*(NumericType number) const {
        EuclideanPoint result = *this;
        return result *= number;
    }
    
    /** Coordinate-wise division.
     *
     *  Returns this coordinate divided by a scalar. Each coordinate of the returned point is the ratio of the corresponding
     *  coordinate of @p this point to the specified @p number. */
    EuclideanPoint operator/(NumericType number) const {
        EuclideanPoint result = *this;
        return result /= number;
    }

    /** Minimum coordinate.
     *
     *  Returns a pair whose first member is the least coordinate value and the second member is the dimension containing this
     *  smallest value. For example, the return value when applied to the Cartesian point (5.1, 3.8) is the pair (3.8, 1). */
    std::pair<NumericType, size_t> min() const {
        NumericType minval = coords_[0];
        size_t dimension = 0;
        for (size_t i=1; i<Dimensionality; ++i) {
            if (coords_[i] < minval) {
                minval = coords_[i];
                dimension = i;
            }
        }
        return std::make_pair(minval, dimension);
    }

    /** Coordinate-wise minimum.
     *
     *  Returns a new point, each element of which is the minimum of the corresponding element from @p this point and the @p
     *  other point. */
    EuclideanPoint min(const EuclideanPoint &other) const {
        EuclideanPoint result;
        for (size_t i=0; i<Dimensionality; ++i)
            result.coords_[i] = std::min(coords_[i], other.coords_[i]);
        return result;
    }
    
    /** Maximum coordinate.
     *
     *  Returns a pair whose first member is the greatest coordinate value and the second member is the dimension containing
     *  this largest value. For example, the return value when applied to the point (5.1, 3.8) is the pair
     *  (5.1, 0). */
    std::pair<NumericType, size_t> max() const {
        NumericType minval = coords_[0];
        size_t dimension = 0;
        for (size_t i=1; i<Dimensionality; ++i) {
            if (coords_[i] > minval) {
                minval = coords_[i];
                dimension = i;
            }
        }
        return std::make_pair(minval, dimension);
    }
    
    /** Coordinate-wise maximum.
     *
     *  Returns a new point, each element of which is the maximum of the corresponding element from @p this point and the @p
     *  other point. */
    EuclideanPoint max(const EuclideanPoint &other) const {
        EuclideanPoint result;
        for (size_t i=0; i<Dimensionality; ++i)
            result.coords_[i] = std::max(coords_[i], other.coords_[i]);
        return result;
    }

    /** Test for equality.
     *
     *  Returns true if each coordinate of @p this is equal to the corresponding coordinate of @p other. */
    bool operator==(const EuclideanPoint &other) const {
        for (size_t i=0; i<Dimensionality; ++i) {
            if (coords_[i] != other.coords_[i])
                return false;
        }
        return true;
    }

    /** Test for inequality.
     *
     *  Returns true if any coordinate of @p this is not equal to the corresponding coordinate of @p other. */
    bool operator!=(const EuclideanPoint &other) const {
        for (size_t i=0; i<Dimensionality; ++i) {
            if (coords_[i] != other.coords_[i])
                return true;
        }
        return false;
    }

    /** Inner, or dot, product.
     *
     *  Computes the inner product, which is the sum of the products of corresponding coordinates in @p this point and the @p
     *  other point.
     *
     * @{ */
    NumericType innerProduct(const EuclideanPoint &other) const {
        NumericType sum = 0;
        for (size_t i=0; i<Dimensionality; ++i)
            sum += coords_[i] * other.coords_[i];
        return sum;
    }
    NumericType operator*(const EuclideanPoint &other) const {
        return innerProduct(other);
    }
    /** @} */

    /** Norm, or length of a vector.
     *
     *  Computes the norm of @p this point.  The norm of a point is the length of a vector from the origin to the
     *  point.
     *
     * @{ */
    NumericType norm() const {
        return sqrt(innerProduct(*this));
    }
    NumericType length() const {
        return norm();
    }
    /** @} */

    /** Metric, or distance.
     *
     *  Computes the Euclidean metric, or the distance, from @p this point to the @p other point. */
    NumericType metric(const EuclideanPoint &other) const {
        return (other - *this).norm();
    }
    NumericType distance(const EuclideanPoint &other) const {
        return metric(other);
    }
    /** @} */

    /** Angle between two vectors.
     *
     *  Computes the non-reflex angle in radians between two vectors. The vectors are from the origin to @p this point and the
     *  origin to the @p other point. The return value is in the range 0 through <em>pi</em>, inclusive.
     *
     * @{ */
    NumericType angle(const EuclideanPoint &other) const {
        return acos(innerProduct(other) / (norm()*other.norm()));
    }
    /** @} */
};

/** Two dimensional point of type double.
 *
 *  Note that the "d" in the name stands for @c double, not "dimension". */
typedef EuclideanPoint<2, double> Point2d;

/** Three dimensional point of type double.
 *
 *  Note that the "d" in the name stands for @c double, not "dimension. */
typedef EuclideanPoint<3, double> Point3d;

/** Two dimensional point of type float. */
typedef EuclideanPoint<2, float> Point2f;

/** Three dimensional point of type float. */
typedef EuclideanPoint<3, float> Point3f;

/** Two dimensional point of type int. */
typedef EuclideanPoint<2, int> Point2i;

/** Three dimensional point of type int. */
typedef EuclideanPoint<3, int> Point3i;

/** Constructs a 2d Euclidean point. */
template<typename NumericType>
inline EuclideanPoint<2, NumericType> point2(NumericType x, NumericType y) {
    EuclideanPoint<2, NumericType> result;
    result[0] = x;
    result[1] = y;
    return result;
}

/** Constructs a 3d Euclidean point. */
template<typename NumericType>
inline EuclideanPoint<3, NumericType> point3(NumericType x, NumericType y, NumericType z) {
    EuclideanPoint<3, NumericType> result;
    result[0] = x;
    result[1] = y;
    result[2] = z;
    return result;
}

/** Print a point. */
template<size_t Dimensionality, typename NumericType>
std::ostream& operator<<(std::ostream &stream, const EuclideanPoint<Dimensionality, NumericType> &point) {
    for (size_t i=0; i<Dimensionality; ++i)
        stream <<(i?", ":"(") <<point[i];
    stream <<")";
    return stream;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                      Regions of space
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/** Euclidean box.
 *
 *  A box is represented by a minimum corner and a maximum corner. The @p NumericType should generally be some floating-point
 *  type. Although integral types are supported to some degree, some operations are not well defined for integral types. */
template<size_t Dimensionality, typename NumericType>
class EuclideanBox {
public:
    /** Points representing the box corners. */
    typedef EuclideanPoint<Dimensionality, NumericType> Point;

private:
    Point minCorner_, maxCorner_;
    bool isEmpty_;

public:
    /** Default constructor.
     *
     *  Constructs an empty box. */
    EuclideanBox(): isEmpty_(true) {}

    /** Singleton constructor.
     *
     *  Implicitly constructs a box containing a single point. */
    EuclideanBox(const Point &pt): minCorner_(pt), maxCorner_(pt), isEmpty_(false) {}

    /** Construct a box from two points.
     *
     *  The specified points need not be the minimum and maximum corners. */
    EuclideanBox(const Point &p1, const Point &p2)
        : minCorner_(p1.min(p2)), maxCorner_(p1.max(p2)), isEmpty_(false) {}

    /** Test whether this box is empty. */
    bool isEmpty() const {
        return isEmpty_;
    }

    /** Minimum corner. */
    const Point& minCorner() const {
        ASSERT_forbid(isEmpty());
        return minCorner_;
    }

    /** Maximum corner. */
    const Point& maxCorner() const {
        ASSERT_forbid(isEmpty());
        return maxCorner_;
    }

    /** Length in one dimension.
     *
     *  Length is always non-negative. */
    NumericType length(size_t dimension) const {
        if (isEmpty())
            return 0;
        ASSERT_require(dimension < Dimensionality);
        return maxCorner_[dimension] - minCorner_[dimension];
    }

    /** Length, area, volume.
     *
     *  In one dimension this is the length, in two dimensions this is the area, in three dimensions this is the volumn, etc. */
    NumericType size() const {
        if (isEmpty())
            return 0;
        NumericType product = 1;
        for (size_t i=0; i<Dimensionality; ++i)
            product *= length(i);
        return product;
    }
    
    /** Center of region.
     *
     *  The center of the box is a point where each coordinate is the average of the corresponding coordinates from the min and
     *  max corners. */
    Point center() const {
        ASSERT_forbid(isEmpty());
        Point pt;
        for (size_t i=0; i<Dimensionality; ++i)
            pt[i] = (maxCorner_[i] + minCorner_[i]) / 2;
        return pt;
    }

    /** Test equality.
     *
     *  Returns true if the min and max corners of @p this box and the @p other box are equal. An empty box is equal only to
     *  another empty box. */
    bool operator==(const EuclideanBox &other) const {
        if (isEmpty() || other.isEmpty())
            return isEmpty() && other.isEmpty();
        return minCorner_ == other.minCorner_ && maxCorner_ == other.maxCorner_;
    }

    /** Test inequality.
     *
     *  Returns true unless @p this box and the @p other box are equal. An empty box is un-equal to any non-empty box. */
    bool operator!=(const EuclideanBox &other) const {
        if (isEmpty() || other.isEmpty())
            return !isEmpty() || !other.isEmpty();
        return minCorner_ != other.minCorner_ || maxCorner_ != other.maxCorner_;
    }

    /** Test whether two boxes are disjoint.
     *
     *  Returns true if @p this box and the @p other box do not intersect. An empty box is disjoint with all other boxes
     *  whether they're empty or not. */
    bool isDisjoint(const EuclideanBox &other) const {
        if (isEmpty() || other.isEmpty())
            return true;
        for (size_t i=0; i<Dimensionality; ++i) {
            if (minCorner_[i] > other.maxCorner_[i] || maxCorner_[i] < other.minCorner_[i])
                return true;
        }
        return false;
    }
    
    /** Test whether boxes intersect.
     *
     *  An empty box never intersects with any other box, empty or not. */
    bool isIntersecting(const EuclideanBox &other) const {
        if (isEmpty() || other.isEmpty())
            return false;
        return !isDisjoint(other);
    }

    /** Test one box contains the other.
     *
     *  Returns true if @p this box contains the @p other box. All boxes contain an empty box; an empty box contains only
     *  another empty box. */
    bool isContaining(const EuclideanBox &other) const {
        if (isEmpty())
            return other.isEmpty();
        if (other.isEmpty())
            return true;
        for (size_t i=0; i<Dimensionality; ++i) {
            if (other.minCorner_[i] < minCorner_[i] || other.maxCorner_[i] > maxCorner_[i])
                return false;
        }
        return true;
    }
    
    /** Construct a new box from the intersection of boxes.
     *
     *  Returns a new box which is the intersection of @p this box and the @p other box.  If the boxes don't intersect then it
     *  returns an empty box. */
    EuclideanBox intersection(const EuclideanBox &other) const {
        if (isEmpty() || other.isEmpty())
            return EuclideanBox();
        Point pt1, pt2;
        for (size_t i=0; i<Dimensionality; ++i) {
            if (minCorner_[i] > other.maxCorner_[i] || maxCorner_[i] < other.minCorner_[i])
                return Point();                         // not intersecting
            pt1[i] = std::max(minCorner_[i], other.minCorner_[i]);
            pt2[i] = std::min(maxCorner_[i], other.maxCorner_[i]);
        }
        return EuclideanBox(pt1, pt2);
    }

    /** Translate this box in place.
     *
     *  Translates @p this box by adding the translation vector to the min and max points. Translating an empty box has no
     *  effect.
     *
     * @{ */
    EuclideanBox& operator+=(const Point &translation) {
        if (!isEmpty()) {
            minCorner_ += translation;
            maxCorner_ += translation;
        }
        return *this;
    }
    EuclideanBox& operator-=(const Point &translation) {
        if (!isEmpty()) {
            minCorner_ -= translation;
            maxCorner_ -= translation;
        }
        return *this;
    }
    /** @} */

    /** Construct a new box by translation.
     *
     *  Returns a new box having the same size as @p this box, but translated by the specified vector. Translating an empty box
     *  returns an empty box.
     *
     * @{ */
    EuclideanBox operator+(const Point &translation) const {
        EuclideanBox newBox;
        newBox += translation;
        return newBox;
    }
    EuclideanBox operator-(const Point &translation) const {
        EuclideanBox newBox;
        newBox -= translation;
        return newBox;
    }
    /** @} */

    /** Scales this box in place.
     *
     *  Scales this box in place by multipling the min and max corners by the specified scalar value. Scaling an empty box has
     *  no effect. Scaling by zero makes the box empty. */
    EuclideanBox& operator*=(const NumericType scaleFactor) {
        if (0 == scaleFactor) {
            isEmpty_ = true;
        } else if (!isEmpty()) {
            minCorner_ *= scaleFactor;
            maxCorner_ *= scaleFactor;
        }
        return *this;
    }

    /** Construct a new box by scaling.
     *
     *  Returns a new box by scaling @p this box.  This consists of multiplying the min and max corners by the scale
     *  factor. Scaling an empty box returns an empty box, as does scaling a non-empty box by zero. */
    EuclideanBox operator*(const NumericType scaleFactor) const {
        EuclideanBox newBox;
        newBox *= scaleFactor;
        return newBox;
    }
    
    /** Translate box to a new center.
     *
     *  Constructs a new box by translating @p this box so it's centered at the indicated point. Recentering an empty box
     *  returns a new empty box. */
    EuclideanBox recenter(const Point &newCenter) const {
        return *this - center() + newCenter;
    }

    /** Scale a box around its center.
     *
     *  Returns a new box created by scaling @p this box around its center.  Scaling an empty box returns an empty box, as does
     *  scaling by zero. */
    EuclideanBox centerScale(NumericType scaleFactor) const {
        return pointScale(center(), scaleFactor);
    }

    /** Scale a box around a certain point. */
    EuclideanBox pointScale(const Point &pt, NumericType scaleFactor) const {
        return (*this - pt) * scaleFactor + pt;
    }

    /** Expand a box to include a point. */
    EuclideanBox expand(const Point &pt) const {
        Point pt1 = minCorner_.min(pt);
        Point pt2 = maxCorner_.max(pt);
        return EuclideanBox(pt1, pt2);
    }
};

/** Two dimensional box of type double.
 *
 *  Note that the "d" in the name stands for @c double, not "dimension". */
typedef EuclideanBox<2, double> Box2d;

/** Three dimensional box of type double.
 *
 *  Note that the "d" in the name stands for @c double, not "dimension". */
typedef EuclideanBox<3, double> Box3d;

/** Two dimensional box of type float. */
typedef EuclideanBox<2, float> Box2f;

/** Three dimensional box of type float. */
typedef EuclideanBox<3, float> Box3f;

/** Two dimensional box of type int. */
typedef EuclideanBox<2, int> Box2i;

/** Three dimensional box of type int. */
typedef EuclideanBox<3, int> Box3i;

/** Construct a 2d box from two opposite corner points. */
template<typename NumericType>
EuclideanBox<2, NumericType> box2(const EuclideanPoint<2, NumericType> &pt1, const EuclideanPoint<2, NumericType> &pt2) {
    return EuclideanBox<2, NumericType>(pt1, pt2);
}

/** Construct a 3d box from two opposite corner points. */
template<typename NumericType>
EuclideanBox<3, NumericType> box3(const EuclideanPoint<3, NumericType> &pt1, const EuclideanPoint<3, NumericType> &pt2) {
    return EuclideanBox<3, NumericType>(pt1, pt2);
}

/** Print a box. */
template<size_t Dimensionality, typename NumericType>
std::ostream& operator<<(std::ostream &stream, const EuclideanBox<Dimensionality, NumericType> &box) {
    if (box.isEmpty()) {
        stream <<"Box(empty)";
    } else {
        stream <<"Box(" <<box.minCorner() <<", " <<box.maxCorner() <<")";
    }
    return stream;
}

} // namespace
} // namespace

#endif

