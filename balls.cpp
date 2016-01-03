#include <cairomm/context.h>
#include <glibmm/main.h>
#include <sstream>

#include "balls.h"


bool Balls::on_draw(const Cairo::RefPtr<Cairo::Context>& cr)
{
  Gtk::Allocation allocation = get_allocation();
  const int width = allocation.get_width();
  const int height = allocation.get_height();

  cr->save();
  cr->scale(width, height);

  cr->set_line_width(0.001);
  for (auto ball : balls_)
    {
      cr->set_source_rgb(ball.color_r,ball.color_g,ball.color_b);
      cr->arc(ball.p.x,ball.p.y,
              ball.rad,
              0,2*M_PI);
      cr->fill();
      cr->stroke();
    }

  cr->restore();

  const Ball &ball1 = balls_[balls_.size()-1];
  std::ostringstream info;
  info << "x = " << ball1.p.x << "\ny = " << ball1.p.y;
  infobox_.show(cr,width,height,info.str());

  return true;
}
