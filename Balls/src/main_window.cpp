#include <main_window.h>
#include <xaml/ui/controls/canvas.h>
#include <xaml/ui/controls/grid.h>
#include <xaml/ui/window.h>

using namespace std;

struct balls_main_window_impl : xaml_implement<balls_main_window_impl, balls_main_window, xaml_object>
{
    xaml_ptr<xaml_window> m_window{};

    xaml_result XAML_CALL init() noexcept;
    xaml_result XAML_CALL show() noexcept override { return m_window->show(); }
};

xaml_result balls_main_window_impl::init() noexcept
{

    return XAML_S_OK;
}

xaml_result XAML_CALL balls_main_window_new(balls_main_window** ptr) noexcept
{
    return xaml_object_init<balls_main_window_impl>(ptr);
}
