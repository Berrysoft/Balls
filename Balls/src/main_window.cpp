#include <balls.h>
#include <main_window.h>
#include <sf/sformat.hpp>
#include <xaml/ui/controls/canvas.h>
#include <xaml/ui/controls/grid.h>
#include <xaml/ui/msgbox.h>
#include <xaml/ui/timer.h>
#include <xaml/ui/window.h>

namespace colors
{
#include <xaml/ui/colors.h>
}

using namespace std;

struct balls_main_window_impl : xaml_implement<balls_main_window_impl, balls_main_window, xaml_object>
{
    xaml_ptr<xaml_window> m_window{};
    xaml_ptr<xaml_timer> m_timer{};
    xaml_ptr<xaml_canvas> m_canvas{};

    xaml_ptr<balls_map> m_map{};
    xaml_ptr<balls_map_enumerator> m_enumerator{};

    double dx, dy, dw, dh;

    xaml_result XAML_CALL init() noexcept;
    xaml_result XAML_CALL show() noexcept override;

    xaml_result XAML_CALL init_balls(bool*) noexcept;
    xaml_result XAML_CALL show_open(bool*) noexcept;

    xaml_result XAML_CALL on_canvas_redraw(xaml_ptr<xaml_canvas>, xaml_ptr<xaml_drawing_context>) noexcept;
    xaml_result XAML_CALL on_map_ball_score_changed(xaml_ptr<balls_map>, balls_ball_score_changed_args) noexcept;
    xaml_result XAML_CALL on_canvas_size_changed(xaml_ptr<xaml_canvas>, xaml_size) noexcept;
};

xaml_result balls_main_window_impl::init() noexcept
{
    XAML_RETURN_IF_FAILED(xaml_timer_new(&m_timer));

    XAML_RETURN_IF_FAILED(balls_map_new(&m_map));
    {
        xaml_ptr<xaml_delegate> callback;
        XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<balls_map>, balls_ball_score_changed_args>(
            xaml_mem_fn(&balls_main_window_impl::on_map_ball_score_changed, this), &callback)));
        int32_t token;
        XAML_RETURN_IF_FAILED(m_map->add_ball_score_changed(callback, &token));
    }

    XAML_RETURN_IF_FAILED(xaml_window_new(&m_window));
    XAML_RETURN_IF_FAILED(m_window->set_size({ balls_client_width, balls_client_height }));
    {
        xaml_ptr<xaml_grid> grid;
        XAML_RETURN_IF_FAILED(xaml_grid_new(&grid));
        {
            XAML_RETURN_IF_FAILED(xaml_canvas_new(&m_canvas));
            XAML_RETURN_IF_FAILED(m_canvas->set_halignment(xaml_halignment_stretch));
            XAML_RETURN_IF_FAILED(m_canvas->set_valignment(xaml_valignment_stretch));
            {
                xaml_ptr<xaml_delegate> callback;
                XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_canvas>, xaml_ptr<xaml_drawing_context>>(
                    xaml_mem_fn(&balls_main_window_impl::on_canvas_redraw, this), &callback)));
                int32_t token;
                XAML_RETURN_IF_FAILED(m_canvas->add_redraw(callback, &token));
            }
            {
                xaml_ptr<xaml_delegate> callback;
                XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_canvas>, xaml_size>(
                    xaml_mem_fn(&balls_main_window_impl::on_canvas_size_changed, this), &callback)));
                int32_t token;
                XAML_RETURN_IF_FAILED(m_canvas->add_size_changed(callback, &token));
            }
            XAML_RETURN_IF_FAILED(grid->add_child(m_canvas));
        }
        XAML_RETURN_IF_FAILED(m_window->set_child(grid));
    }

    return XAML_S_OK;
}

xaml_result balls_main_window_impl::show() noexcept
{
    bool show;
    XAML_RETURN_IF_FAILED(init_balls(&show));
    if (show)
        XAML_RETURN_IF_FAILED(m_window->show());
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::init_balls(bool* pvalue) noexcept
{
    xaml_ptr<xaml_string> message, title, instruction;
    XAML_RETURN_IF_FAILED(xaml_string_new(U("二维弹球"), &title));
    XAML_RETURN_IF_FAILED(xaml_string_new(U("请选择难度"), &instruction));
    XAML_RETURN_IF_FAILED(xaml_string_new(U("所有难度的区别仅为方块上数目大小的概率分布。"), &message));
    xaml_ptr<xaml_vector> buttons;
    XAML_RETURN_IF_FAILED(xaml_vector_new(&buttons));
#define ADD_CUSTOM_BUTTON(result, text)                                               \
    {                                                                                 \
        xaml_msgbox_custom_button __bsimple = { (xaml_msgbox_result)(result), text }; \
        xaml_ptr<xaml_object> __box;                                                  \
        XAML_RETURN_IF_FAILED(xaml_box_value(__bsimple, &__box));                     \
        XAML_RETURN_IF_FAILED(buttons->append(__box));                                \
    }
    ADD_CUSTOM_BUTTON(201, U("简单"));
    ADD_CUSTOM_BUTTON(202, U("正常"));
    ADD_CUSTOM_BUTTON(203, U("困难"));
    ADD_CUSTOM_BUTTON(204, U("打开存档"));
#undef ADD_CUSTOM_BUTTON
    xaml_msgbox_result result;
    XAML_RETURN_IF_FAILED(xaml_msgbox_custom(m_window, message, title, instruction, xaml_msgbox_info, buttons, &result));
    switch (result)
    {
    case 201:
        XAML_RETURN_IF_FAILED(m_map->set_difficulty(balls_difficulty_simple));
        break;
    case 202:
        XAML_RETURN_IF_FAILED(m_map->set_difficulty(balls_difficulty_normal));
        break;
    case 203:
        XAML_RETURN_IF_FAILED(m_map->set_difficulty(balls_difficulty_hard));
        break;
    case 204:
        return show_open(pvalue);
    default:
        *pvalue = false;
        return XAML_S_OK;
    }
    bool dummy;
    XAML_RETURN_IF_FAILED(m_map->reset(&dummy));
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::show_open(bool* pvalue) noexcept
{
    // TODO
    *pvalue = false;
    return XAML_S_OK;
}

static constexpr xaml_color get_equare_color(int32_t t) noexcept
{
    constexpr int power = 4;
    constexpr int range = 256 / power;

    t %= range * 5;
    if (t < range)
    {
        return { 255, (uint8_t)(t * power), 0, 255 };
    }
    else if (t < range * 2)
    {
        return { 255, 255, 0, (uint8_t)(255 - (t - range) * power) };
    }
    else if (t < range * 3)
    {
        return { 255, 255, (uint8_t)((t - range * 2) * power), 0 };
    }
    else if (t < range * 4)
    {
        return { 255, (uint8_t)(255 - (t - range * 3) * power), 255, (uint8_t)((t - range * 3) * power) };
    }
    else
    {
        return { 255, 0, (uint8_t)(255 - (t - range * 4) * power), 255 };
    }
}

xaml_result balls_main_window_impl::on_canvas_redraw(xaml_ptr<xaml_canvas> cv, xaml_ptr<xaml_drawing_context> dc) noexcept
{
    xaml_ptr<xaml_solid_brush> bback;
    XAML_RETURN_IF_FAILED(xaml_solid_brush_new(colors::black, &bback));
    xaml_size size;
    XAML_RETURN_IF_FAILED(cv->get_size(&size));
    XAML_RETURN_IF_FAILED(dc->fill_rect(bback, { -1, -1, size.width + 2, size.height + 2 }));
    // TODO
    return XAML_S_OK;
}

static constexpr string_view simple_text = U("简单");
static constexpr string_view normal_text = U("正常");
static constexpr string_view hard_text = U("困难");

static constexpr string_view get_difficulty_text(balls_difficulty difficulty) noexcept
{
    switch (difficulty)
    {
    case balls_difficulty_simple:
        return simple_text;
    case balls_difficulty_normal:
        return normal_text;
    case balls_difficulty_hard:
        return hard_text;
    default:
        return {};
    }
}

xaml_result balls_main_window_impl::on_map_ball_score_changed(xaml_ptr<balls_map>, balls_ball_score_changed_args args) noexcept
{
    balls_difficulty difficulty;
    XAML_RETURN_IF_FAILED(m_map->get_difficulty(&difficulty));
    string title;
    if (args.ball_num != args.remain_ball_num)
    {
        title = sf::sprint(U("二维弹球 - {} 球数：{} 剩余球数：{} 分数：{}"), get_difficulty_text(difficulty), args.ball_num, args.remain_ball_num, args.score);
    }
    else
    {
        title = sf::sprint(U("二维弹球 - {} 球数：{} 分数：{}"), get_difficulty_text(difficulty), args.ball_num, args.score);
    }
    xaml_ptr<xaml_string> title_str;
    XAML_RETURN_IF_FAILED(xaml_string_new(move(title), &title_str));
    return m_window->set_title(title_str);
}

xaml_result balls_main_window_impl::on_canvas_size_changed(xaml_ptr<xaml_canvas>, xaml_size size) noexcept
{
    double sidew = (size.width + 1) / balls_max_columns;
    double sideh = (size.height + 1) / balls_max_rows;
    double sidel = (min)(sidew, sideh);
    dw = sidel * balls_max_columns - 1;
    dh = sidel * balls_max_rows - 1;
    dx = (size.width - dw) / 2;
    dy = (size.height - dh) / 2;
    return m_canvas->invalidate();
}

xaml_result XAML_CALL balls_main_window_new(balls_main_window** ptr) noexcept
{
    return xaml_object_init<balls_main_window_impl>(ptr);
}
