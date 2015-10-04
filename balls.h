#ifndef GTKMM_EXAMPLE_BALLS_H
#define GTKMM_EXAMPLE_BALLS_H

#include <glibmm/main.h>
#include <gtkmm/drawingarea.h>
#include <vector>
#include <utility>
#include <algorithm>
#include <cmath>
#include <random>
#include <functional>
#include <limits>

#include "./vec2d.h"

template <class It, class Func>
void foreach_two(It beg, It end, Func &&func)
{
  using std::for_each;
  while (beg != end)
    {
      auto &elem = *beg;
      ++beg;
      for_each(beg,end,[&elem,&func](auto &elem2)
               {
                 func(elem,elem2);
               });
    }
}

class Balls : public Gtk::DrawingArea
{
public:
  using seed_type = std::random_device::result_type;

  const int time_lapse = 10;

  struct Ball
  {
    Vec p;
    Vec v;
    double m;
    double rad;
    double color_r;
    double color_g;
    double color_b;

    std::pair<Ball*,unsigned> recent_collision;

    Ball(Vec pos,
         Vec vel,
         double mass = 0.1,
         double r    = 0.2,
         double g    = 0.2,
         double b    = 0.2)
      : p(pos),
        v(vel),
        m(mass),
        rad(0.12 * mass),
        color_r(r),
        color_g(g),
        color_b(b),
        recent_collision { nullptr , 0 }
    { }

    Ball()
      : Ball({0.0d,0.0d},{0.0d,0.0d})
    { }
  };


  Ball random_ball()
  {
    static std::uniform_real_distribution<double> pos_dist(0,1);
    static std::uniform_real_distribution<double> speed_dist(0.00001,0.0003);
    static std::uniform_real_distribution<double> mass_dist(0.01,0.5);

    return {
        { pos_dist(rand_), pos_dist(rand_) },
        { speed_dist(rand_), speed_dist(rand_) },
        mass_dist(rand_),
        pos_dist(rand_),pos_dist(rand_),pos_dist(rand_)
    };
  }


  Balls(seed_type seed, std::size_t n_balls = 10)
    : rand_(seed),
      balls_(n_balls)
  {
    Vec p { 0.08 , 0.0 };

    using std::make_pair;
    std::generate(begin(balls_),end(balls_),std::bind(&Balls::random_ball,this));

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
        ball.p += time_lapse * ball.v;

    collisions();

    for (auto &ball : balls_)
      if (ball.recent_collision.second > 0)
        ball.recent_collision.second -= 1;
  }

  void collisions()
  {
    using std::make_pair;

    for (auto &ball : balls_)
      {
        /* Check for collisions with wall. */
        if ((ball.p.x - ball.rad < std::numeric_limits<double>::epsilon())
            || (ball.p.x + ball.rad > 1.0 - std::numeric_limits<double>::epsilon()))
          ball.v.x = -ball.v.x;
        if ((ball.p.y - ball.rad < std::numeric_limits<double>::epsilon())
            || (ball.p.y + ball.rad > 1.0 - std::numeric_limits<double>::epsilon()))
          ball.v.y = -ball.v.y;
      }

    foreach_two(begin(balls_),end(balls_),[](auto &ball, auto &oball) {
        if (ball.recent_collision.second > 0 && ball.recent_collision.first == &oball)
          return;

        auto deltap = ball.p - oball.p;
        double sqr_dist = sqr(deltap.x) + sqr(deltap.y);
        double sqr_rad  = sqr(ball.rad + oball.rad);

        if (sqr_dist < sqr_rad)
          {
            /* Collision of two balls. */
            double dist = ::sqrt(sqr_dist);
            Vec min_trans_dist = ((ball.rad + oball.rad - dist) / dist) * deltap;

            double im1 = 1 / ball.m;
            double im2 = 1 / oball.m;

            ball.p  += (im1 / (im1 + im2)) * min_trans_dist;
            oball.p -= (im2 / (im1 + im2)) * min_trans_dist;

            Vec deltav = ball.v - oball.v;
            double vn = dot(deltav,min_trans_dist.normal());

            if (vn < 0)
              {
                const double restitution = 0.9;
                double i = (-(1.0d + restitution) * vn) / (im1 + im2);
                Vec impulse = i * min_trans_dist;
                ball.v  += im1 * impulse;
                oball.v -= im2 * impulse;
              }

            ball.recent_collision  = make_pair(&oball,10);
            oball.recent_collision = make_pair(&ball,10);
          }
      });
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

  std::default_random_engine rand_;
  std::vector<Ball>          balls_;
};

#endif // GTKMM_EXAMPLE_BALLS_H
