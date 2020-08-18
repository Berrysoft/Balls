#include <balls/main_window.h>
#include <xaml/ui/application.h>

using namespace std;

xaml_result XAML_CALL xaml_main(xaml_application* app, int* pcode) noexcept
{
    xaml_ptr<balls_main_window> wnd;
    XAML_RETURN_IF_FAILED(balls_main_window_new(&wnd));
    XAML_RETURN_IF_FAILED(wnd->show());
    return app->run(pcode);
}
