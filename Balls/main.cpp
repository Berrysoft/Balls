#include "pch.h"

#include "mainwnd.h"

using namespace sw;

INT APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR lpCmdLine, int nCmdShow)
{
    init_app(hInstance, lpCmdLine, nCmdShow);
    mainwnd wnd;
    return current_app.run(wnd);
}
