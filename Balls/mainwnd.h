﻿#pragma once
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
    bool fasten;
    sw::dev_context hdcbuffer;

public:
    mainwnd();

private:
    void init();

    void main_paint(sw::window&, sw::dev_context& dc);
    void timer_tick(sw::timer&, DWORD dwTime);
    void click(sw::window&, const sw::mouse_args& arg);
    void mmove(sw::window&, const sw::mouse_args& arg);
    void kdown(sw::window&, const sw::key_args& args);
    void kup(sw::window&, const sw::key_args& args);
    void change_title(::balls&, const balls_changed_args& args);

    difficulty get_diff();
    bool show_stop();

    void reset();
};
