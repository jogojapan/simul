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
#include "./textbox.h"

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
        recent_collision(nullptr,0)
    { }

    Ball()
      : Ball({0.0d,0.0d},{0.0d,0.0d})
    { }
  };


  Ball random_ball()
  {
    static std::uniform_real_distribution<double> pos_dist(0,1);
    static std::uniform_real_distribution<double> speed_dist(0.000001,0.00003);
    static std::normal_distribution<double>       mass_dist(0.05,0.0);

    return {
        { pos_dist(rand_), pos_dist(rand_) },
        { speed_dist(rand_), speed_dist(rand_) },
          std::abs(mass_dist(rand_)),
        pos_dist(rand_),pos_dist(rand_),pos_dist(rand_)
    };
  }


  Balls(seed_type seed, std::size_t n_balls = 10)
    : rand_(seed),
      balls_(n_balls),
      infobox_(*this,15,2)
  {
    Vec p { 0.08 , 0.0 };

    using std::make_pair;
    std::generate(begin(balls_),end(balls_),std::bind(&Balls::random_ball,this));
    balls_.push_back(Ball { {0.5,0.5},{0.0,0.0},0.2,0.1,0.1,0.1 } );

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
      //    ball.p += ball.v;
      ball.p += time_lapse * ball.v;

    collisions();
  }

  void collisions()
  {
    using std::make_pair;
    using std::numeric_limits;
    static const double eps = numeric_limits<double>::epsilon();

    for (auto &ball : balls_)
      {
        if (ball.recent_collision.second > 0)
          --ball.recent_collision.second;
        if (ball.recent_collision.second == 0)
          ball.recent_collision.first = nullptr;

        /* Check for collisions with wall. */
        if (ball.p.x - ball.rad < 0)
          {
            ball.p.x = ball.rad + eps;
            ball.v.x = -ball.v.x;
          }
        if (ball.p.x + ball.rad > 1.0)
          {
            ball.p.x = 1.0 - ball.rad - eps;
            ball.v.x = -ball.v.x;
          }
        if (ball.p.y - ball.rad < 0)
          {
            ball.p.y = ball.rad + eps;
            ball.v.y = -ball.v.y;
          }
        if (ball.p.y + ball.rad > 1.0)
          {
            ball.p.y = 1 - ball.rad - eps;
            ball.v.y = -ball.v.y;
          }
      }

    /* Collisions. */
    foreach_two(begin(balls_),end(balls_),[](auto &ball1, auto &ball2) {
        static const double eps = numeric_limits<double>::epsilon();

        if ((ball1.recent_collision.first == &ball2)
            || (ball2.recent_collision.first == &ball1))
          return;

        auto deltap = ball1.p - ball2.p;
        double sqr_dist = sqr(deltap.x) + sqr(deltap.y);
        double sqr_rad  = sqr(ball1.rad + ball2.rad);

        if (sqr_dist < sqr_rad)
          {
            if (sqr_dist < eps)
              sqr_dist = eps;

            /* Collision of two balls. */
            double dist = ::sqrt(sqr_dist);
            if (dist < eps)
              dist = eps;
            Vec min_trans_dist = ((ball1.rad + ball2.rad - dist) / dist) * deltap;

            double sum_m = ball1.m + ball2.m;

            Vec u1 = ball1.v;
            Vec u2 = ball2.v;

            ball1.v -=
              (2*ball2.m / sum_m)
              * (dot(u1 - u2,ball1.p - ball2.p) / norm(ball1.p - ball2.p))
              * (ball1.p - ball2.p);

            ball2.v -=
              (2*ball1.m / sum_m)
              * (dot(u2 - u1,ball2.p - ball1.p) / norm(ball2.p - ball1.p))
              * (ball2.p - ball1.p);

            ball1.p += ((1/ball1.m) / (1/ball1.m+1/ball2.m)) * min_trans_dist;
            ball2.p -= ((1/ball2.m) / (1/ball1.m+1/ball2.m)) * min_trans_dist;

            ball1.recent_collision = make_pair(&ball2,3);
            ball2.recent_collision = make_pair(&ball1,3);
          }
      });

    /* Effects of gravity. */
    foreach_two(begin(balls_),end(balls_),[](auto &ball1, auto &ball2) {
        Vec deltap = ball1.p - ball2.p;
        if (deltap.len() > 0)
          {
            Vec force  = 0.00001 * ((ball1.m + ball2.m) / sqr(deltap.len())) * deltap;
            ball1.v += force;
            ball2.v -= force;
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
  Textbox                    infobox_;
};

#endif // GTKMM_EXAMPLE_BALLS_H
