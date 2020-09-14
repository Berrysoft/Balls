#include <balls/main_window.h>
#include <xaml/ui/application.h>

using namespace std;

static xaml_ptr<balls_main_window> wnd;

xaml_result XAML_CALL xaml_main(xaml_application*) noexcept
{
    XAML_RETURN_IF_FAILED(balls_main_window_new(&wnd));
    return wnd->show();
}
