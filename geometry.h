#ifndef _HPS_GEOMETRY_GEOMETRY_H_
#define _HPS_GEOMETRY_GEOMETRY_H_
#include <assert.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4201) // nameless struct/union
#endif

#include <math.h>

namespace hps
{
namespace geometry
{

/// <summary> A 2-dimensional vector. </summary>
template <typename NumericType_>
struct Vector2
{
  typedef NumericType_ NumericType;
  enum { Dim = 2, };
  Vector2() : x(0), y(0) {}
  Vector2(const NumericType x_, const NumericType y_) : x(x_), y(y_) {}
  inline Vector2& operator+=(const Vector2& rhs)
  {
    x += rhs.x;
    y += rhs.y;
    return *this;
  }
  inline Vector2& operator-=(const Vector2& rhs)
  {
    x -= rhs.x;
    y -= rhs.y;
    return *this;
  }
  inline Vector2& operator*=(const NumericType& rhs)
  {
    x *= rhs;
    y *= rhs;
    return *this;
  }
  inline Vector2& operator/=(const NumericType& rhs)
  {
    x /= rhs;
    y /= rhs;
    return *this;
  }
  union
  {
    struct
    {
      NumericType x;
      NumericType y;
    };
    NumericType v[2];
  };
};

/// <summary> Addition for vectors. </summary>
template <typename NumericType>
inline const Vector2<NumericType> operator+(const Vector2<NumericType>& lhs,
                                            const Vector2<NumericType>& rhs)
{
  return Vector2<NumericType>(lhs.x + rhs.x, lhs.y + rhs.y);
}

/// <summary> Subtraction for vectors. </summary>
template <typename NumericType>
inline const Vector2<NumericType> operator-(const Vector2<NumericType>& lhs,
                                            const Vector2<NumericType>& rhs)
{
  return Vector2<NumericType>(lhs.x - rhs.x, lhs.y - rhs.y);
}

/// <summary> Unary minus for vectors. </summary>
template <typename NumericType>
inline const Vector2<NumericType> operator-(const Vector2<NumericType>& v)
{
  return Vector2<NumericType>(-v.x, -v.y);
}

/// <summary> Equality test for vectors. </summary>
template <typename NumericType>
inline bool operator==(const Vector2<NumericType>& lhs,
                       const Vector2<NumericType>& rhs)
{
  return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

/// <summary> Inequality test for vectors. </summary>
template <typename NumericType>
inline bool operator!=(const Vector2<NumericType>& lhs,
                       const Vector2<NumericType>& rhs)
{
  return !(lhs == rhs);
}

/// <summary> Scalar multiplication for vectors. </summary>
template <typename NumericType, typename ScalarType>
inline const Vector2<NumericType> operator*(const Vector2<NumericType>& lhs,
                                            const ScalarType rhs)
{
  return Vector2<NumericType>(lhs.x * rhs, lhs.y * rhs);
}

/// <summary> Scalar multiplication for vectors. </summary>
template <typename NumericType, typename ScalarType>
inline const Vector2<NumericType> operator*(const ScalarType lhs,
                                            const Vector2<NumericType>& rhs)
{
  return Vector2<NumericType>(lhs * rhs.x, lhs * rhs.y);
}

/// <summary> Scalar division for vectors. </summary>
template <typename NumericType, typename ScalarType>
inline const Vector2<NumericType> operator/(const Vector2<NumericType>& lhs,
                                            const ScalarType rhs)
{
  return Vector2<NumericType>(lhs.x / rhs, lhs.y / rhs);
}

/// <summary> Compute vector length squared. </summary>
template <typename NumericType>
inline NumericType Vector2LengthSq(const Vector2<NumericType>& v)
{
  return (v.x * v.x) + (v.y * v.y);
}

/// <summary> Compute vector length. </summary>
template <typename NumericType>
inline float Vector2Length(const Vector2<NumericType>& v)
{
  return sqrt(static_cast<float>(Vector2LengthSq(v)));
}

template <typename NumericType>
inline bool Vector2NearlyEqual(const Vector2<NumericType>& lhs,
                                const Vector2<NumericType>& rhs,
                                const NumericType errSqBound)
{
  return Vector2LengthSq(rhs - lhs) < errSqBound;
}

/// <summary> A segment defined by two points. </summary>
template <typename NumericType_>
struct Segment2
{
  typedef NumericType_ NumericType;
  Segment2() : p0(), p1() {}
  Segment2(const Vector2<NumericType>& p0_, const Vector2<NumericType>& p1_)
    : p0(p0_),
      p1(p1_)
  {}
  Vector2<NumericType> p0;
  Vector2<NumericType> p1;
};

template <typename NumericType>
inline bool operator==(const Segment2<NumericType>& lhs, const Segment2<NumericType>& rhs)
{
  // Check cris-cross, too.
  return ((lhs.p0 == rhs.p0) && (lhs.p1 == rhs.p1)) ||
         ((lhs.p0 == rhs.p1) && (lhs.p1 == rhs.p0));
}

template <typename NumericType>
inline bool Segment2NearlyEqual(const Segment2<NumericType>& lhs, const Segment2<NumericType>& rhs,
                                const NumericType errSqBound)
{
  // Check cris-cross, too.
  return (Vector2NearlyEqual(lhs.p0, rhs.p0, errSqBound) &&
          Vector2NearlyEqual(lhs.p1, rhs.p1, errSqBound)) ||
         (Vector2NearlyEqual(lhs.p0, rhs.p1, errSqBound) &&
          Vector2NearlyEqual(lhs.p1, rhs.p0, errSqBound));
}

template <typename NumericType>
inline bool operator!=(const Segment2<NumericType>& lhs, const Segment2<NumericType>& rhs)
{
  return !(lhs == rhs);
}

/// <summary> A segment defined by two points with direction. </summary>
template <typename NumericType_>
struct DirectedSegment
{
  typedef NumericType_ NumericType;
  DirectedSegment() : p0(), p1() {}
  DirectedSegment(const Vector2<NumericType>& p0_, const Vector2<NumericType>& p1_)
    : p0(p0_),
      p1(p1_)
  {}
  Vector2<NumericType> p0;
  Vector2<NumericType> p1;
};

/// <summary> A line given by point and direction. </summary>
template <typename NumericType_>
struct Line
{
  typedef NumericType_ NumericType;
  Line() : p0(), dir() {}
  Line(const Vector2<NumericType>& p0_, const Vector2<NumericType>& dir_)
    : p0(p0_),
      dir(dir_)
  {}
  Vector2<NumericType> p0;
  Vector2<NumericType> dir;
};

/// <summary> An in-plane box without rotational transformation. </summary>
template <typename NumericType_>
struct AxisAlignedBox2
{
  typedef NumericType_ NumericType;
  AxisAlignedBox2() : mins(), maxs() {}
  AxisAlignedBox2(const Vector2<NumericType>& mins_, const Vector2<NumericType>& maxs_)
    : mins(mins_),
      maxs(maxs_)
  {
    assert(maxs.x >= mins.x);
    assert(maxs.y >= mins.y);
  }
  Vector2<NumericType> mins;
  Vector2<NumericType> maxs;
};

template <typename NumericType>
inline bool operator==(const AxisAlignedBox2<NumericType>& lhs,
                       const AxisAlignedBox2<NumericType>& rhs)
{
  return (lhs.mins == rhs.mins) && (lhs.maxs == rhs.maxs);
}

/// <summary> Rotate vector 90 degrees counter-clockwise. </summary>
template <typename NumericType>
inline Vector2<NumericType> Vector2Normalize(const Vector2<NumericType>& v)
{
  return (1 / Vector2Length(v)) * v;
}

/// <summary> Rotate vector 90 degrees counter-clockwise. </summary>
template <typename NumericType>
inline Vector2<NumericType> Vector2Rotate90(const Vector2<NumericType>& v)
{
  // Matrix | 0, -1 |
  //        | 1,  0 | is a 90 degree rotation in the x-y plane.
  return Vector2<NumericType>(-v.y, v.x);
}

/// <summary> Compute dot product of two vectors. </summary>
template <typename NumericType>
inline NumericType Vector2Dot(const Vector2<NumericType>& lhs,
                              const Vector2<NumericType>& rhs)
{
  return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

/// <summary> Finds a single intersection point between two lines. </summary>
template <typename NumericType>
inline bool LineIntersectLineUnique(const Line<NumericType>& a,
                                    const Line<NumericType>& b,
                                    Vector2<NumericType>* p)
{
  // Lines should not be parallel.
  const NumericType aDotb = Vector2Dot(a.dir, b.dir);
  const NumericType parallelTest = Vector2LengthSq(a.dir) * Vector2LengthSq(b.dir);
  const float diffParallelTest = static_cast<float>((aDotb * aDotb) - parallelTest);
  if (fabs(diffParallelTest) < 1.0e-4f)
  {
    return false;
  }
  // reissb -- 20111022 -- Compute line intersection using determinants.
  //   Formula published on http://en.wikipedia.org/wiki/Line-line_intersection.
  const Vector2<NumericType>& p_1 = a.p0;
  const Vector2<NumericType> p_2 = a.p0 + a.dir;
  const Vector2<NumericType>& p_3 = b.p0;
  const Vector2<NumericType> p_4 = b.p0 + b.dir;
  const NumericType det_p_1_p_2 = ((p_1.x * p_2.y) - (p_1.y * p_2.x));
  const NumericType det_p_3_p_4 = ((p_3.x * p_4.y) - (p_3.y * p_4.x));
  const NumericType denom = (((p_1.x - p_2.x) * (p_3.y - p_4.y)) -
                             ((p_1.y - p_2.y) * (p_3.x - p_4.x)));
  p->x = ((det_p_1_p_2 * (p_3.x - p_4.x)) - (det_p_3_p_4 * (p_1.x - p_2.x))) / denom;
  p->y = ((det_p_1_p_2 * (p_3.y - p_4.y)) - (det_p_3_p_4 * (p_1.y - p_2.y))) / denom;
  return true;
}

template <typename NumericType>
inline NumericType AxisAlignedBox2Area(const AxisAlignedBox2<NumericType>& box)
{
  const Vector2<NumericType> dims = box.maxs - box.mins;
  return dims.x * dims.y;
}

template <typename NumericType>
inline NumericType AxisAlignedBox2Contains(const AxisAlignedBox2<NumericType>& box,
                                           const Vector2<NumericType>& p)
{
  return (p.x >= box.mins.x) && (p.x <= box.maxs.x) &&
         (p.y >= box.mins.y) && (p.y <= box.maxs.y);
}

}
using namespace geometry;
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif //_HPS_GEOMETRY_GEOMETRY_H_
