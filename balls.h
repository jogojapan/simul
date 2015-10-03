#ifndef GTKMM_EXAMPLE_BALLS_H
#define GTKMM_EXAMPLE_BALLS_H

#include <glibmm/main.h>
#include <gtkmm/drawingarea.h>
#include <vector>
#include <utility>
#include <algorithm>
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

  Vec operator-(const Vec &v) const
  {
    return { x - v.x , y - v.y };
  }
};

inline double dot(Vec v1, Vec v2)
{ return (v1.x*v2.x + v1.y*v2.y); }

inline Vec operator*(double scalar, Vec v)
{ return { scalar * v.x , scalar * v.y }; }


class Balls : public Gtk::DrawingArea
{
public:
  const int time_lapse = 100;

  struct Ball
  {
    Vec p;
    Vec v;
    double rad;

    Ball(Vec pos,
         Vec vel,
         double radius = 0.006)
      : p(pos),
        v(vel),
        rad(radius)
    { }

    Ball()
      : Ball({0.0d,0.0d},{0.0d,0.0d},0.0d)
    { }
  };

  Balls(std::size_t n_balls = 10)
    : balls_(n_balls)
  {
    Vec p { 0.08 , 0.0 };

    using std::make_pair;
    std::generate(begin(balls_),end(balls_),
                  [&p]{ p += { 0.05 , 0.05 }; return Ball { p,{0.0001,0.0001} }; });

    Glib::signal_timeout().connect(sigc::mem_fun(*this, &Balls::on_timeout),
                                   time_lapse);

#ifndef GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED
    // Connect the signal handler if it isn't already a virtual method
    // override:

    signal_draw().connect(sigc::mem_fun(*this, &Balls::on_draw), false);
#endif //GLIBMM_DEFAULT_SIGNAL_HANDLERS_ENABLED    
  }

  virtual ~Balls()
  { }

protected:
  virtual bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr);

  void update_balls()
  {
    for (auto &ball : balls_)
      {
        ball.p += time_lapse * ball.v;
      }
    collisions();
  }

  void collisions()
  {
    for (auto &ball : balls_)
      {
        /* Check for collisions with wall. */
        if ((ball.p.x - ball.rad < 0.0) || (ball.p.x + ball.rad > 1.0))
          ball.v.x = -ball.v.x;
        if ((ball.p.y - ball.rad < 0.0) || (ball.p.y + ball.rad > 1.0))
          ball.v.y = -ball.v.y;

        for (const auto &oball : balls_)
          {
            if (&ball == &oball)
              continue;

            auto deltap = ball.p - oball.p;
            double sqr_dist = sqr(deltap.x) + sqr(deltap.y);
            double sqr_rad  = sqr(ball.rad + oball.rad);

            if (sqr_dist < sqr_rad)
              {
                /* Collision of two balls. */
                double dist = ::sqrt(sqr_dist);
                Vec min_trans_dist = ((ball.rad + oball.rad - dist) / dist) * deltap;

                double im1 = 1.0;
                double im2 = 1.0;

                ball.p -=  (im1 / (im1 + im2)) * min_trans_dist;
                oball.p -= (im2 / (im1 + im2)) * min_trans_dist;
              }
          }
      }
  }

  bool on_timeout()
  {
    /**
       Whenever we get the timeout signal, we invalidate the window to
       force a redraw of its contents.
    */

    update_balls();

    Glib::RefPtr<Gdk::Window> win = get_window();
    if (win)
      {
        Gdk::Rectangle r(0,
                         0,
                         get_allocation().get_width(),
                         get_allocation().get_height());
        win->invalidate_rect(r, false);
      }
    return true;
  }

  std::vector<Ball> balls_;
};

#endif // GTKMM_EXAMPLE_BALLS_H
