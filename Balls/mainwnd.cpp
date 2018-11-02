#include "pch.h"

#include "mainwnd.h"
#include "resource.h"
#include "serialstream.h"
#include <fstream>
#include <itemdlg.h>
#include <msgbox.h>
#include <sf/sformat.hpp>

using namespace std;
using namespace sf;
using namespace sw;

#define RECORD_VERSION 2

#define ID_SIMPLE 201
#define ID_NORMAL 202
#define ID_HARD 203
#define ID_OPEN 204
#define ID_REPLAY 205
#define ID_SAVE 206
#define ID_NOSAVE 207

constexpr UINT normal_speed = 30;
constexpr UINT fast_speed = 10;

constexpr COLORREF black_back = RGB(0, 0, 0);
constexpr pen black_back_pen = { PS_SOLID, 1, black_back };
constexpr solid_brush black_back_brush = { black_back };

constexpr COLORREF white_fore = RGB(255, 255, 255);
constexpr pen white_fore_pen = { PS_SOLID, 3, white_fore };

constexpr COLORREF red_ball = RGB(197, 15, 31);
constexpr pen red_ball_pen = { PS_SOLID, 1, red_ball };
constexpr solid_brush red_ball_brush = { red_ball };

constexpr COLORREF red_sample = RGB(231, 72, 86);
constexpr pen red_sample_pen = { PS_SOLID, 1, red_sample };
constexpr solid_brush red_sample_brush = { red_sample };

constexpr COLORREF green_border = RGB(22, 198, 12);
constexpr pen green_border_pen = { PS_SOLID, 3, green_border };

constexpr COLORREF blue_circle = RGB(0, 55, 218);
constexpr solid_brush blue_circle_brush = { blue_circle };

constexpr COLORREF yellow_circle = RGB(193, 156, 0);
constexpr pen yellow_circle_pen = { PS_SOLID, 1, yellow_circle };
constexpr solid_brush yellow_circle_brush = { yellow_circle };

constexpr COLORREF purple_circle = RGB(136, 23, 152);
constexpr solid_brush purple_circle_brush = { purple_circle };

mainwnd::mainwnd()
    : window(L"Balls", WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION | WS_MINIMIZEBOX),
      main_timer(30), fasten(false)
{
    init();
}

void mainwnd::init()
{
    icon(LoadIcon(current_app.instance(), MAKEINTRESOURCE(IDI_ICONAPP)));
    //关联事件
    paint(&mainwnd::main_paint, this);
    main_timer.elapsed(&mainwnd::timer_tick, this);
    mouse_down(&mainwnd::click, this);
    mouse_move(&mainwnd::mmove, this);
    key_down(&mainwnd::kdown, this);
    key_up(&mainwnd::kup, this);
    balls.ball_score_changed(&mainwnd::change_title, this);
    closing(&mainwnd::wclose, this);
    //调整大小
    client_rect({ 0, 0, client_width, client_height });
    move_center(false);
    //获取难度
    if (!get_diff())
    {
        //初始化游戏
        balls.sample_ball(balls.start_pos());
        balls.reset();
    }
    //强制刷新标题
    change_title(balls, { balls.ball_num(), balls.score() });
    //生成缓存
    hdcbuffer = get_buffered_dc(client_width, client_height);
    //字符相对于y居中放置（方便计算坐标）
    hdcbuffer.text_align(TA_CENTER);
    hdcbuffer.back_mode(TRANSPARENT);
    hdcbuffer.text_color(white_fore);
    hdcbuffer.set_font(font{ TEXT("Segoe UI"), num_height }.create());
}

void mainwnd::main_paint(window&, dev_context& dc)
{
    //画背景
    hdcbuffer.set_pen(black_back_pen.create());
    hdcbuffer.set_brush(black_back_brush.create());
    hdcbuffer.draw_rect({ 0, 0, client_width, client_height });
    //只有没有发射球的时候才画示例球
    if (!it)
    {
        hdcbuffer.set_pen(red_sample_pen.create());
        hdcbuffer.set_brush(red_sample_brush.create());
        hdcbuffer.draw_ellipse(balls.sample_ball(), radius);
    }
    //如果加倍就画黄色球，反之画红色球
    if (balls.double_score())
    {
        hdcbuffer.set_pen(yellow_circle_pen.create());
        hdcbuffer.set_brush(yellow_circle_brush.create());
    }
    else
    {
        hdcbuffer.set_pen(red_ball_pen.create());
        hdcbuffer.set_brush(red_ball_brush.create());
    }
    //球没有发射完才画起始球
    if (!it || !it.end_shooting())
    {
        hdcbuffer.draw_ellipse(balls.start_pos(), radius);
    }
    //确定了下一个起始位置才画终止球
    if (balls.end_pos() != balls.start_pos())
    {
        hdcbuffer.draw_ellipse(balls.end_pos(), radius);
    }
    //画所有运动中的球
    for (const ball& p : *it)
    {
        hdcbuffer.draw_ellipse(p.pos, radius);
    }
    //下面的都是绿边框
    hdcbuffer.set_pen(green_border_pen.create());
    for (int x = 0; x < max_c; x++)
    {
        for (int y = 0; y < max_r; y++)
        {
            int t = balls.get_squares()[y][x];
            int cx = x * side_length + side_length / 2;
            int cy = y * side_length + side_length / 2;
            if (t > 0)
            {
                //方块的填充色会变化
                hdcbuffer.set_brush(solid_brush{ RGB(t > 63 ? 255 : t * 4, 0, t > 63 ? t >= 127 ? 0 : 255 - (t - 63) * 4 : 255) }.create());
                hdcbuffer.draw_rect({ x * side_length + 5, y * side_length + 5, (x + 1) * side_length - 1 - 5, (y + 1) * side_length - 1 - 5 });
                hdcbuffer.draw_string({ cx, cy - num_height / 2 }, to_wstring(t));
            }
            else
            {
                switch (t)
                {
                case ID_NEWBALL:
                {
                    hdcbuffer.set_brush(blue_circle_brush.create());
                    hdcbuffer.draw_ellipse({ cx, cy }, num_height / 2);
                    pen_ptr ori = hdcbuffer.set_pen(white_fore_pen.create());
                    hdcbuffer.draw_cross({ cx, cy }, num_height - 10 * 2);
                    hdcbuffer.set_pen(move(ori));
                    break;
                }
                case ID_DELBALL:
                {
                    hdcbuffer.set_brush(red_ball_brush.create());
                    hdcbuffer.draw_ellipse({ cx, cy }, num_height / 2);
                    pen_ptr ori = hdcbuffer.set_pen(white_fore_pen.create());
                    hdcbuffer.draw_line({ cx - num_height / 2 + 10, cy }, { cx + num_height / 2 - 10, cy });
                    hdcbuffer.set_pen(move(ori));
                    break;
                }
                case ID_RNDTURN:
                case ID_OLDTURN:
                {
                    hdcbuffer.set_brush((t == ID_RNDTURN ? purple_circle_brush : red_sample_brush).create());
                    hdcbuffer.draw_ellipse({ cx, cy }, num_height / 2);
                    hdcbuffer.draw_string({ cx, cy - num_height / 2 }, TEXT("?"));
                    break;
                }
                case ID_DBSCORE:
                {
                    hdcbuffer.set_brush(yellow_circle_brush.create());
                    hdcbuffer.draw_ellipse({ cx, cy }, num_height / 2);
                    hdcbuffer.draw_string({ cx, cy - num_height / 2 }, TEXT("￥"));
                    break;
                }
                }
            }
        }
    }
    //最后把位图复制到窗口上
    dc.copy_dc_bit({ 0, 0, client_width, client_height }, hdcbuffer);
}

void mainwnd::timer_tick(timer&, DWORD)
{
    ++it;
    if (!it)
    {
        main_timer.stop();
        if (!balls.reset())
        {
            if (show_stop())
            {
                if (!get_diff())
                    reset();
            }
            else
            {
                close();
                return;
            }
        }
        //如果游戏没有结束而且在上一次发射之后鼠标没有移动
        //示例球的位置仍然保持原状
        //因此需要重新计算
        POINT p;
        if (GetCursorPos(&p) && ScreenToClient(hWnd, &p))
        {
            balls.set_sample(p.x, p.y);
        }
    }
    //不需要清屏，位图是覆盖上去的
    refresh(false);
}

void mainwnd::click(window&, const mouse_args& arg)
{
    //只有在本轮未开始的时候响应单击
    if (!it)
    {
        it = balls.iterator(arg.pos.x, arg.pos.y);
        main_timer.start();
    }
}

void mainwnd::mmove(window&, const mouse_args& arg)
{
    balls.set_sample(arg.pos.x, arg.pos.y);
    refresh(false);
}

void mainwnd::kdown(window&, const key_args& args)
{
    //按下空格加快速度
    if (args.key == VK_SPACE && !fasten)
    {
        fasten = true;
        main_timer.time_span(fast_speed);
    }
}

void mainwnd::kup(window&, const key_args& args)
{
    //抬起空格恢复正常
    //按F7暂停
    switch (args.key)
    {
    case VK_SPACE:
        if (fasten)
        {
            fasten = false;
            main_timer.time_span(normal_speed);
        }
        break;
    case VK_F7:
        if (it)
        {
            if (main_timer)
            {
                main_timer.stop();
            }
            else
            {
                main_timer.start();
            }
        }
        break;
    }
}

wstring get_string_dfct(difficulty dfct)
{
    switch (dfct)
    {
    case simple:
        return TEXT("简单");
    case normal:
        return TEXT("正常");
    case hard:
        return TEXT("困难");
    default:
        return {};
    }
}

void mainwnd::change_title(::balls&, const balls_changed_args& args)
{
    text(sprint(TEXT("二维弹球 - {} 球数：{} 分数：{}"), get_string_dfct(balls.game_dfct()), args.ball_num, args.score));
}

void mainwnd::wclose(window&, bool& handled)
{
    if (!balls.over())
    {
        bool started = main_timer;
        main_timer.stop();
        handled = show_close();
        if (started)
            main_timer.start();
    }
}

TASKDIALOG_BUTTON get_difficulty_buttons[] =
    {
        { ID_SIMPLE, TEXT("简单\r\n正态分布，平均值为球数的一半") },
        { ID_NORMAL, TEXT("正常\r\n正态分布，平均值为球数") },
        { ID_HARD, TEXT("困难\r\n正态分布，平均值为球数的1.5倍") },
        { ID_OPEN, TEXT("打开存档\r\n打开已保存的游戏存档") }
    };
bool mainwnd::get_diff()
{
    auto result = show_dialog(taskdlg{
        TEXT("二维弹球"),
        TEXT("请选择难度"),
        TEXT("所有难度的区别仅为方块上数目大小的概率分布。"),
        { taskdlg_custom, LoadIcon(current_app.instance(), MAKEINTRESOURCE(IDI_ICONAPP)) },
        { taskdlg_close_button, get_difficulty_buttons, true },
        {},
        {},
        { TEXT("加号增加球的数目；问号随机更改球的速度方向；\r\n"
               "减号使当前球消失；人民币符号使本轮得分加倍。\r\n"
               "按空格加速，按F7暂停。请不要过于依赖示例球。"),
          TEXT("帮助") },
        { TEXT("版权所有 © 2018 Berrysoft"), { taskdlg_information } } });
    switch (result.button_index)
    {
    case ID_SIMPLE:
        balls.game_dfct(simple);
        break;
    case ID_NORMAL:
        balls.game_dfct(normal);
        break;
    case ID_HARD:
        balls.game_dfct(hard);
        break;
    case ID_OPEN:
        if (show_open())
            return true;
    default:
        exit(0);
    }
    return false;
}

COMDLG_FILTERSPEC show_open_filter[] =
    {
        { TEXT("存档文件"), TEXT("*.balls") },
        { TEXT("所有文件"), TEXT("*.*") }
    };
wstring mainwnd::show_open_record()
{
    open_item_dlg fdlg;
    fdlg.filter(show_open_filter);
    if (show_dialog(fdlg) == S_OK)
        return fdlg.result_filename();
    return {};
}

bool mainwnd::show_open()
{
    wstring filename = show_open_record();
    if (filename.empty())
        return false;
    serialstream stream(filename, ios::in);
    int version;
    stream >> version;
    if (version != RECORD_VERSION)
    {
        show_dialog(msgbox{
            TEXT("存档是由本游戏的不同版本创建的，无法打开"),
            TEXT("二维弹球"),
            ok_button, error_icon });
        return false;
    }
    stream >> balls;
    it = balls_iterator(&balls);
    stream >> it;
    return true;
}

TASKDIALOG_BUTTON show_stop_buttons[] =
    {
        { ID_REPLAY, TEXT("重新开始") }
    };
bool mainwnd::show_stop()
{
    auto result = show_dialog(taskdlg{
        TEXT("二维弹球"),
        TEXT("游戏结束"),
        sprint(TEXT("难度：{}\r\n球数：{}\r\n分数：{}"),
               get_string_dfct(balls.game_dfct()),
               balls.ball_num(),
               balls.score()),
        { taskdlg_information },
        { taskdlg_close_button, show_stop_buttons } });
    if (result.button_index == ID_REPLAY)
        return true;
    return false;
}

TASKDIALOG_BUTTON show_close_buttons[] =
    {
        { ID_SAVE, TEXT("存档") },
        { ID_NOSAVE, TEXT("不存档") }
    };
bool mainwnd::show_close()
{
    auto result = show_dialog(taskdlg{
        TEXT("二维弹球"),
        TEXT("游戏尚未结束，是否存档？"),
        {},
        { taskdlg_information },
        { taskdlg_cancel_button, show_close_buttons } });
    switch (result.button_index)
    {
    case ID_SAVE:
        return !show_save();
    case IDCANCEL:
        return true;
    }
    return false;
}

COMDLG_FILTERSPEC show_close_filter[] =
    {
        { TEXT("存档文件"), TEXT("*.balls") }
    };
wstring mainwnd::show_close_record()
{
    save_item_dlg fdlg;
    fdlg.filter(show_close_filter);
    if (show_dialog(fdlg) == S_OK)
        return fdlg.result_filename();
    return {};
}

//如果保存成功返回true
bool mainwnd::show_save()
{
    wstring filename = show_close_record();
    if (filename.empty())
        return false;
    serialstream stream(filename, ios::out);
    stream << RECORD_VERSION;
    stream << balls;
    stream << it;
    return true;
}

void mainwnd::reset()
{
    balls.reset_all();
    balls.reset();
    fasten = false;
    main_timer.time_span(normal_speed);
}
