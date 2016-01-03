#ifndef GTKMM_EXAMPLE_TEXTBOX_H
#define GTKMM_EXAMPLE_TEXTBOX_H

#include <sstream>
#include <glibmm/main.h>
#include <gtkmm/drawingarea.h>

/**
   A simple text box to be displayed in a Cairo context.

   Use as follows:

   Make the text box a data member of your Gtk::DrawingArea object.

   In the constructor of that object, initialize the text box by
   passing it a reference to the DrawingArea object itself (*this), as
   well as the desired size of the text box, measured in characters
   (width and height).

   In the on_draw function of your object, call the text box's show()
   method, passing to it a reference to the Cairo::Context, the total
   width and height of the Gtk::Allocation, and the text you'd like to
   show:

   Gtk::Allocation allocation = get_allocation();
   const int width = allocation.get_width();
   const int height = allocation.get_height();

   std::ostringstream info;
   info << "x = " << ball1.p.x << "\ny = " << ball1.p.y;
   textbox_.show(cr,width,height,info.str());

   Caution: The success of this depends on what functions of the Cairo
   context you called before (esp. the scale() method). Best use
   cr->save() and cr->restore() to get a clean Cairo context before
   calling the show method of the text box.
 */
class Textbox
{
public:  
  /**
     The font type used for the text box is not configurable. Whenever
     the text box needs to set the Pango font to a Pango::Layout
     (i.e. a piece of text to be printed), it uses the function below
     to define it.
   */
  static void apply_std_font(const Glib::RefPtr<Pango::Layout> &layout)
  {
    Pango::FontDescription font;
    font.set_family("Monospace");
    font.set_weight(Pango::WEIGHT_BOLD);
    layout->set_font_description(font);
  }

  /**
     Initialize the text box by specifying the desired width and
     height in characters. We will compute the required size in
     pixels, assuming that all characters are the size of 'M'.
   */
  Textbox(Gtk::DrawingArea &parent,
          unsigned          width_chars,
          unsigned          height_chars)
    : parent_(parent),
      box_width_(0.0),
      box_height_(0.0)
  {
    std::ostringstream strm;
    for (unsigned y = 0 ; y < height_chars ; ++y)
      {
        for (unsigned x = 0 ; x < width_chars ; ++x)
          strm << 'M';
        strm << '\n';
      }

    auto txt_layout = parent_.create_pango_layout(strm.str().c_str());
    apply_std_font(txt_layout);

    int txt_width, txt_height;
    txt_layout->get_pixel_size(txt_width,txt_height);

    box_width_ = txt_width;
    box_height_ = txt_height;
  }

  void show(const Cairo::RefPtr<Cairo::Context>& cr,
            const int                            total_width,
            const int                            total_height,
            const std::string                  & txt)
    const
  {
    cr->save();

    auto txt_layout = parent_.create_pango_layout(txt.c_str());
    apply_std_font(txt_layout);
    cr->set_source_rgb(0.0,0.0,0.0);
    cr->move_to(total_width - box_width_*1.1,
                total_height - box_height_*1.1);
    txt_layout->show_in_cairo_context(cr);

    cr->restore();
  }

private:
  Gtk::DrawingArea &parent_;
  double            box_width_;
  double            box_height_;
};

#endif // GTKMM_EXAMPLE_TEXTBOX_H
