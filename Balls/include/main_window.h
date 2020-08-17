#ifndef BALLS_MAIN_WINDOW_H
#define BALLS_MAIN_WINDOW_H

#include <xaml/object.h>

XAML_CLASS(balls_main_window, { 0x30b1280b, 0xf891, 0x45f1, { 0xb3, 0xd1, 0xaa, 0xf5, 0x93, 0x4f, 0xd7, 0x7c } })

#define BALLS_MAIN_WINDOW_VTBL(type)           \
    XAML_VTBL_INHERIT(XAML_OBJECT_VTBL(type)); \
    XAML_METHOD(show, type)

XAML_DECL_INTERFACE_(balls_main_window, xaml_object)
{
    XAML_DECL_VTBL(balls_main_window, BALLS_MAIN_WINDOW_VTBL);
};

EXTERN_C xaml_result XAML_CALL balls_main_window_new(balls_main_window**) XAML_NOEXCEPT;

#endif // !BALLS_MAIN_WINDOW_H
