#pragma once
#include "balls.h"
#include <gdi.h>
#include <timer.h>
#include <window.h>

class mainwnd : public sw::window
{
private:
    balls balls;
    balls_iterator it;
    sw::timer main_timer;
    sw::dev_context hdcbuffer;

public:
    mainwnd();

private:
    void init();

    void main_paint(sw::window&, sw::dev_context& dc);
    void timer_tick(sw::timer&, DWORD dwTime);
    void click(sw::window&, const sw::mouse_args& arg);
    void mmove(sw::window&, const sw::mouse_args& arg);
    void kup(sw::window&, const sw::key_args& args);
    void change_title(::balls&, const balls_changed_args& args);
    void wclose(sw::window&, bool& handled);

    bool get_diff();
    std::wstring show_open_record();
    bool show_open();
    bool show_stop();
    std::wstring show_close_record();
    bool show_close();
    bool show_save();

    void reset();
};
