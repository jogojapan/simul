#include "./balls.h"
#include <gtkmm/application.h>
#include <gtkmm/window.h>

int main(int argc, char **argv)
{
  auto app = Gtk::Application::create(argc, argv, "me.jogojapan.simul");

  Gtk::Window win;
  win.set_title("simul");
  win.set_default_size(800,800);

  Balls balls(23,100);
  win.add(balls);
  balls.show();

  return app->run(win);
}
