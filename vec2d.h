#include <cmath>

constexpr double sqr(const double v)
{ return v * v; }


struct Vec
{
  double x;
  double y;

  Vec(double nx, double ny)
    : x(nx) , y(ny)
  { }

  Vec()
    : Vec(0.0,0.0)
  { }

  Vec &operator+=(Vec v)
  {
    x += v.x;
    y += v.y;
    return *this;
  }

  Vec &operator-=(Vec v)
  {
    x -= v.x;
    y -= v.y;
    return *this;
  }

  Vec operator-(const Vec &v) const
  {
    return { x - v.x , y - v.y };
  }

  double len() const
  {
    return ::sqrt(sqr(x)+sqr(y));
  }

  Vec normal() const
  {
    const double L = len();
    return { x / L , y / L };
  }
};

inline double dot(Vec v1, Vec v2)
{ return (v1.x*v2.x + v1.y*v2.y); }

inline double norm(Vec v)
{ return (sqr(v.x) + sqr(v.y)); }

inline Vec operator*(double scalar, Vec v)
{ return { scalar * v.x , scalar * v.y }; }
