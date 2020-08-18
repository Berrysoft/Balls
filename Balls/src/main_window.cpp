#include <balls.h>
#include <filesystem>
#include <main_window.h>
#include <nowide/filesystem.hpp>
#include <nowide/fstream.hpp>
#include <sf/sformat.hpp>
#include <xaml/ui/application.h>
#include <xaml/ui/controls/canvas.h>
#include <xaml/ui/filebox.h>
#include <xaml/ui/msgbox.h>
#include <xaml/ui/timer.h>
#include <xaml/ui/window.h>

using namespace std;

static constexpr int32_t record_version = 2;

struct balls_main_window_impl : xaml_implement<balls_main_window_impl, balls_main_window, xaml_object>
{
    xaml_ptr<xaml_window> m_window{};
    xaml_ptr<xaml_timer> m_timer{};
    xaml_ptr<xaml_canvas> m_canvas{};

    xaml_ptr<balls_map> m_map{};
    xaml_ptr<balls_map_enumerator> m_enumerator{};

    double dx, dy, dw, dh;
    xaml_point mouse_location;

    xaml_result XAML_CALL init() noexcept;
    xaml_result XAML_CALL show() noexcept override;

    xaml_result XAML_CALL init_balls(bool*) noexcept;
    xaml_result XAML_CALL show_open(bool*) noexcept;
    xaml_result XAML_CALL show_stop(bool*) noexcept;
    xaml_result XAML_CALL show_close(bool*) noexcept;
    xaml_result XAML_CALL show_save(bool*) noexcept;
    xaml_result XAML_CALL open_record(string_view, bool*) noexcept;

    xaml_result XAML_CALL on_canvas_redraw(xaml_ptr<xaml_canvas>, xaml_ptr<xaml_drawing_context>) noexcept;
    xaml_result XAML_CALL on_map_ball_score_changed(xaml_ptr<balls_map>, balls_ball_score_changed_args) noexcept;
    xaml_result XAML_CALL on_window_size_changed(xaml_ptr<xaml_window>, xaml_size) noexcept;
    xaml_result XAML_CALL on_window_closing(xaml_ptr<xaml_window>, xaml_ptr<xaml_box>) noexcept;
    xaml_result XAML_CALL on_timer_tick(xaml_ptr<xaml_timer>) noexcept;
    xaml_result XAML_CALL on_canvas_mouse_up(xaml_ptr<xaml_canvas>, xaml_mouse_button) noexcept;
    xaml_result XAML_CALL on_canvas_mouse_move(xaml_ptr<xaml_canvas>, xaml_point) noexcept;
};

xaml_result balls_main_window_impl::init() noexcept
{
    XAML_RETURN_IF_FAILED(xaml_timer_new_interval(10ms, &m_timer));
    {
        xaml_ptr<xaml_delegate> callback;
        XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_timer>>(
            xaml_mem_fn(&balls_main_window_impl::on_timer_tick, this), &callback)));
        int32_t token;
        XAML_RETURN_IF_FAILED(m_timer->add_tick(callback, &token));
    }

    XAML_RETURN_IF_FAILED(balls_map_new(&m_map));
    {
        xaml_ptr<xaml_delegate> callback;
        XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<balls_map>, balls_ball_score_changed_args>(
            xaml_mem_fn(&balls_main_window_impl::on_map_ball_score_changed, this), &callback)));
        int32_t token;
        XAML_RETURN_IF_FAILED(m_map->add_ball_score_changed(callback, &token));
    }

    XAML_RETURN_IF_FAILED(xaml_window_new(&m_window));
    XAML_RETURN_IF_FAILED(m_window->set_location({ 100, 100 }));
    {
        xaml_ptr<xaml_delegate> callback;
        XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_window>, xaml_size>(
            xaml_mem_fn(&balls_main_window_impl::on_window_size_changed, this), &callback)));
        int32_t token;
        XAML_RETURN_IF_FAILED(m_window->add_size_changed(callback, &token));
    }
    {
        xaml_ptr<xaml_delegate> callback;
        XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_window>, xaml_ptr<xaml_box>>(
            xaml_mem_fn(&balls_main_window_impl::on_window_closing, this), &callback)));
        int32_t token;
        XAML_RETURN_IF_FAILED(m_window->add_closing(callback, &token));
    }
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
            XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_canvas>, xaml_mouse_button>(
                xaml_mem_fn(&balls_main_window_impl::on_canvas_mouse_up, this), &callback)));
            int32_t token;
            XAML_RETURN_IF_FAILED(m_canvas->add_mouse_up(callback, &token));
        }
        {
            xaml_ptr<xaml_delegate> callback;
            XAML_RETURN_IF_FAILED((xaml_delegate_new_noexcept<void, xaml_ptr<xaml_canvas>, xaml_point>(
                xaml_mem_fn(&balls_main_window_impl::on_canvas_mouse_move, this), &callback)));
            int32_t token;
            XAML_RETURN_IF_FAILED(m_canvas->add_mouse_move(callback, &token));
            XAML_RETURN_IF_FAILED(m_window->set_child(m_canvas));
        }

        return XAML_S_OK;
    }
}

xaml_result balls_main_window_impl::show() noexcept
{
    xaml_ptr<xaml_application> app;
    XAML_RETURN_IF_FAILED(xaml_application_current(&app));
    xaml_ptr<xaml_vector_view> args;
    XAML_RETURN_IF_FAILED(app->get_cmd_lines(&args));
    int32_t size;
    XAML_RETURN_IF_FAILED(args->get_size(&size));
    string_view view{};
    if (size > 1)
    {
        for (int32_t i = 0; i < size; i++)
        {
            xaml_ptr<xaml_object> item;
            XAML_RETURN_IF_FAILED(args->get_at(i, &item));
            xaml_ptr<xaml_string> filename;
            XAML_RETURN_IF_FAILED(item->query(&filename));
            XAML_RETURN_IF_FAILED(to_string_view(filename, &view));
            if (!filesystem::exists(nowide::filesystem::path{ view }))
            {
                view = {};
                continue;
            }
        }
    }
    bool show;
    if (!view.empty())
    {
        XAML_RETURN_IF_FAILED(open_record(view, &show));
    }
    else
    {
        XAML_RETURN_IF_FAILED(init_balls(&show));
    }
    if (show)
    {
        XAML_RETURN_IF_FAILED(m_window->show());
        XAML_RETURN_IF_FAILED(m_window->set_size({ 450, 600 }));
    }
    return XAML_S_OK;
}

#define ADD_CUSTOM_BUTTON(buttons, result, text)                                      \
    {                                                                                 \
        xaml_msgbox_custom_button __bsimple = { (xaml_msgbox_result)(result), text }; \
        xaml_ptr<xaml_object> __box;                                                  \
        XAML_RETURN_IF_FAILED(xaml_box_value(__bsimple, &__box));                     \
        XAML_RETURN_IF_FAILED(buttons->append(__box));                                \
    }

xaml_result balls_main_window_impl::init_balls(bool* pvalue) noexcept
{
    xaml_ptr<xaml_string> message, title, instruction;
    XAML_RETURN_IF_FAILED(xaml_string_new(U("二维弹球"), &title));
    XAML_RETURN_IF_FAILED(xaml_string_new(U("请选择难度"), &instruction));
    XAML_RETURN_IF_FAILED(xaml_string_new(
        U("所有难度的区别仅为方块上数目大小的概率分布。\n"
          "加号增加球的数目；问号随机更改球的速度方向；\n"
          "减号使当前球消失；美元符号使本轮得分加倍。\n"
          "按右键暂停。请不要过于依赖示例球。"),
        &message));
    xaml_ptr<xaml_vector> buttons;
    XAML_RETURN_IF_FAILED(xaml_vector_new(&buttons));
    ADD_CUSTOM_BUTTON(buttons, 201, U("简单"));
    ADD_CUSTOM_BUTTON(buttons, 202, U("正常"));
    ADD_CUSTOM_BUTTON(buttons, 203, U("困难"));
    ADD_CUSTOM_BUTTON(buttons, 204, U("打开存档"));
#ifdef XAML_UI_COCOA
    ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_cancel, U("关闭"));
#endif // XAML_UI_COCOA
    xaml_msgbox_result result;
    XAML_RETURN_IF_FAILED(xaml_msgbox_custom(m_window, message, title, instruction, xaml_msgbox_info, buttons, &result));
    switch ((int)result)
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
    XAML_RETURN_IF_FAILED(m_map->reset_all());
    bool dummy;
    XAML_RETURN_IF_FAILED(m_map->reset(&dummy));
    *pvalue = true;
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::show_open(bool* pvalue) noexcept
{
    xaml_ptr<xaml_open_filebox> open;
    XAML_RETURN_IF_FAILED(xaml_open_filebox_new(&open));
    {
        xaml_ptr<xaml_vector> filters;
        XAML_RETURN_IF_FAILED(xaml_vector_new(&filters));
        {
            xaml_filebox_filter f{ U("存档文件"), U("*.balls") };
            xaml_ptr<xaml_object> box;
            XAML_RETURN_IF_FAILED(xaml_box_value(f, &box));
            XAML_RETURN_IF_FAILED(filters->append(box));
        }
        {
            xaml_filebox_filter f{ U("所有文件"), U("*.*") };
            xaml_ptr<xaml_object> box;
            XAML_RETURN_IF_FAILED(xaml_box_value(f, &box));
            XAML_RETURN_IF_FAILED(filters->append(box));
        }
        XAML_RETURN_IF_FAILED(open->set_filters(filters));
    }
    if (XAML_SUCCEEDED(open->show(m_window)))
    {
        xaml_ptr<xaml_string> filename;
        XAML_RETURN_IF_FAILED(open->get_result(&filename));
        string_view view;
        XAML_RETURN_IF_FAILED(to_string_view(filename, &view));
        return open_record(view, pvalue);
    }
    *pvalue = false;
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::open_record(string_view filename, bool* pvalue) noexcept
{
    nowide::ifstream stream(filename.data(), ios_base::binary);
    int32_t version;
    stream.read((char*)&version, sizeof(int32_t));
    if (version == record_version)
    {
        vector<uint8_t> buffer;
        copy(istreambuf_iterator<char>{ stream }, istreambuf_iterator<char>{}, insert_iterator{ buffer, buffer.begin() });
        xaml_ptr<xaml_buffer> buffer_ref;
        XAML_RETURN_IF_FAILED(xaml_buffer_new_reference(buffer, &buffer_ref));
        XAML_RETURN_IF_FAILED(balls_map_deserialize(buffer_ref, m_map, &m_enumerator));
        *pvalue = true;
    }
    else
    {
        xaml_ptr<xaml_string> message, title;
        XAML_RETURN_IF_FAILED(xaml_string_new(U("二维弹球"), &title));
        XAML_RETURN_IF_FAILED(xaml_string_new(U("存档是由本游戏的不同版本创建的，无法打开"), &message));
        xaml_ptr<xaml_vector> buttons;
        XAML_RETURN_IF_FAILED(xaml_vector_new(&buttons));
        ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_ok, U("关闭"));
        xaml_msgbox_result result;
        XAML_RETURN_IF_FAILED(xaml_msgbox_custom(m_window, message, title, nullptr, xaml_msgbox_error, buttons, &result));
        *pvalue = false;
    }
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::show_stop(bool* pvalue) noexcept
{
    xaml_ptr<xaml_string> message, title, instruction;
    XAML_RETURN_IF_FAILED(xaml_string_new(U("二维弹球"), &title));
    XAML_RETURN_IF_FAILED(xaml_string_new(U("游戏结束"), &instruction));
    balls_difficulty difficulty;
    XAML_RETURN_IF_FAILED(m_map->get_difficulty(&difficulty));
    int32_t ball_num;
    XAML_RETURN_IF_FAILED(m_map->get_ball_num(&ball_num));
    uint64_t score;
    XAML_RETURN_IF_FAILED(m_map->get_score(&score));
    XAML_RETURN_IF_FAILED(xaml_string_new(sf::sprint(U("难度：{}\n球数：{}\n分数：{}"), difficulty, ball_num, score), &message));
    xaml_ptr<xaml_vector> buttons;
    XAML_RETURN_IF_FAILED(xaml_vector_new(&buttons));
    ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_yes, U("重新开始"));
    ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_no, U("关闭"));
    xaml_msgbox_result result;
    XAML_RETURN_IF_FAILED(xaml_msgbox_custom(m_window, message, title, instruction, xaml_msgbox_info, buttons, &result));
    *pvalue = result == xaml_msgbox_result_yes;
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::show_close(bool* pvalue) noexcept
{
    xaml_ptr<xaml_string> message, title;
    XAML_RETURN_IF_FAILED(xaml_string_new(U("二维弹球"), &title));
    XAML_RETURN_IF_FAILED(xaml_string_new(U("游戏尚未结束，是否存档？"), &message));
    xaml_ptr<xaml_vector> buttons;
    XAML_RETURN_IF_FAILED(xaml_vector_new(&buttons));
    ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_yes, U("保存"));
    ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_no, U("不保存"));
    ADD_CUSTOM_BUTTON(buttons, xaml_msgbox_result_cancel, U("取消"));
    xaml_msgbox_result result;
    XAML_RETURN_IF_FAILED(xaml_msgbox_custom(m_window, message, title, nullptr, xaml_msgbox_info, buttons, &result));
    switch (result)
    {
    case xaml_msgbox_result_yes:
    {
        XAML_RETURN_IF_FAILED(show_save(pvalue));
        *pvalue = !*pvalue;
        return XAML_S_OK;
    }
    case xaml_msgbox_result_cancel:
        *pvalue = true;
        break;
    default:
        *pvalue = false;
        break;
    }
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::show_save(bool* pvalue) noexcept
{
    xaml_ptr<xaml_save_filebox> save;
    XAML_RETURN_IF_FAILED(xaml_save_filebox_new(&save));
    {
        xaml_ptr<xaml_vector> filters;
        XAML_RETURN_IF_FAILED(xaml_vector_new(&filters));
        {
            xaml_filebox_filter f{ U("存档文件"), U("*.balls") };
            xaml_ptr<xaml_object> box;
            XAML_RETURN_IF_FAILED(xaml_box_value(f, &box));
            XAML_RETURN_IF_FAILED(filters->append(box));
        }
        XAML_RETURN_IF_FAILED(save->set_filters(filters));
    }
    if (XAML_SUCCEEDED(save->show(m_window)))
    {
        xaml_ptr<xaml_string> filename;
        XAML_RETURN_IF_FAILED(save->get_result(&filename));
        string_view filename_view;
        XAML_RETURN_IF_FAILED(to_string_view(filename, &filename_view));
        nowide::ofstream stream(filename_view.data(), ios::binary);
        stream.write((const char*)&record_version, sizeof(int32_t));
        xaml_ptr<xaml_buffer> buffer;
        XAML_RETURN_IF_FAILED(balls_map_serialize(m_map, m_enumerator, &buffer));
        uint8_t* data;
        XAML_RETURN_IF_FAILED(buffer->get_data(&data));
        int32_t size;
        XAML_RETURN_IF_FAILED(buffer->get_size(&size));
        stream.write((const char*)data, size);
        *pvalue = true;
        return XAML_S_OK;
    }
    *pvalue = false;
    return XAML_S_OK;
}

static constexpr xaml_color get_square_color(int32_t t) noexcept
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

static constexpr xaml_rectangle get_ball_rect(xaml_point const& delta, xaml_point const& p, double extend) noexcept
{
    xaml_point real_p = p * extend + delta;
    double r = balls_radius * extend;
    return { real_p.x - r, real_p.y - r, 2 * r, 2 * r };
}

static constexpr xaml_color black_none{ 255, 0, 0, 0 };
static constexpr xaml_color black_back{ 255, 31, 31, 31 };
static constexpr xaml_color white_fore{ 255, 223, 223, 223 };
static constexpr xaml_color red_ball{ 255, 197, 15, 31 };
static constexpr xaml_color red_sample{ 255, 231, 72, 86 };
static constexpr xaml_color green_border{ 255, 22, 198, 12 };
static constexpr xaml_color blue_circle{ 255, 0, 55, 218 };
static constexpr xaml_color yellow_circle{ 255, 193, 156, 0 };
static constexpr xaml_color purple_circle{ 255, 136, 23, 152 };

static constexpr xaml_color get_circle_fill(int32_t t) noexcept
{
    switch (t)
    {
    case balls_special_new_ball:
        return blue_circle;
    case balls_special_delete_ball:
        return red_ball;
    case balls_special_random_turn:
        return purple_circle;
    case balls_special_random_turn_old:
        return red_sample;
    case balls_special_double_score:
        return yellow_circle;
    default:
        return {};
    }
}

xaml_result balls_main_window_impl::on_canvas_redraw(xaml_ptr<xaml_canvas> cv, xaml_ptr<xaml_drawing_context> dc) noexcept
{
    //画背景
    {
        xaml_ptr<xaml_solid_brush> bback;
        XAML_RETURN_IF_FAILED(xaml_solid_brush_new(black_none, &bback));
        xaml_size size;
        XAML_RETURN_IF_FAILED(cv->get_size(&size));
        XAML_RETURN_IF_FAILED(dc->fill_rect(bback, { -1, -1, size.width + 2, size.height + 2 }));
    }
    {
        xaml_ptr<xaml_solid_brush> brback;
        XAML_RETURN_IF_FAILED(xaml_solid_brush_new(black_back, &brback));
        XAML_RETURN_IF_FAILED(dc->fill_rect(brback, { dx, dy, dw, dh }));
    }
    double extend = dw / balls_client_width;
    xaml_drawing_font font{ U("Arial"), balls_num_size * extend, false, false, xaml_halignment_center, xaml_valignment_center };
    //只有没有发射球的时候才画示例球
    if (!m_enumerator)
    {
        xaml_ptr<xaml_solid_brush> sample;
        XAML_RETURN_IF_FAILED(xaml_solid_brush_new(red_sample, &sample));
        xaml_point sample_pos;
        XAML_RETURN_IF_FAILED(m_map->get_sample_position(&sample_pos));
        XAML_RETURN_IF_FAILED(dc->fill_ellipse(sample, get_ball_rect({ dx, dy }, sample_pos, extend)));
    }
    //如果加倍就画黄色球，反之画红色球
    xaml_ptr<xaml_solid_brush> bball;
    bool double_score;
    XAML_RETURN_IF_FAILED(m_map->get_is_double_score(&double_score));
    XAML_RETURN_IF_FAILED(xaml_solid_brush_new(double_score ? yellow_circle : red_ball, &bball));
    xaml_point start_pos;
    XAML_RETURN_IF_FAILED(m_map->get_start_position(&start_pos));
    //球没有发射完才画起始球
    bool end_shooting = false;
    if (m_enumerator)
    {
        XAML_RETURN_IF_FAILED(m_enumerator->get_is_end_shooting(&end_shooting));
    }
    if (!end_shooting)
    {
        XAML_RETURN_IF_FAILED(dc->fill_ellipse(bball, get_ball_rect({ dx, dy }, start_pos, extend)));
    }
    //确定了下一个起始位置才画终止球
    xaml_point end_pos;
    XAML_RETURN_IF_FAILED(m_map->get_end_position(&end_pos));
    if (start_pos != end_pos)
    {
        XAML_RETURN_IF_FAILED(dc->fill_ellipse(bball, get_ball_rect({ dx, dy }, end_pos, extend)));
    }
    //画所有运动中的球
    if (m_enumerator)
    {
        xaml_ptr<xaml_vector> balls;
        {
            xaml_ptr<xaml_object> obj;
            XAML_RETURN_IF_FAILED(m_enumerator->get_current(&obj));
            XAML_RETURN_IF_FAILED(obj->query(&balls));
        }
        XAML_FOREACH_START(ball_item, balls);
        {
            balls_ball b;
            XAML_RETURN_IF_FAILED(xaml_unbox_value(ball_item, &b));
            XAML_RETURN_IF_FAILED(dc->fill_ellipse(bball, get_ball_rect({ dx, dy }, b.pos, extend)));
        }
        XAML_FOREACH_END();
    }
    //下面的都是绿边框
    xaml_ptr<xaml_brush_pen> pborder;
    XAML_RETURN_IF_FAILED(xaml_brush_pen_new_solid(green_border, 3, &pborder));
    balls_map_t const* pmap;
    XAML_RETURN_IF_FAILED(m_map->get_map(&pmap));
    for (int32_t x = 0; x < balls_max_columns; x++)
    {
        for (int32_t y = 0; y < balls_max_rows; y++)
        {
            int32_t t = (*pmap)[y][x];
            double cx = x * balls_side_length + balls_side_length / 2;
            double cy = y * balls_side_length + balls_side_length / 2;
            xaml_point real_c = xaml_point{ cx, cy } * extend;
            xaml_point center = real_c + xaml_point{ dx, dy };
            if (t > 0)
            {
                //方块的填充色会变化
                xaml_color fillc = get_square_color(t);
                xaml_ptr<xaml_solid_brush> btext;
                XAML_RETURN_IF_FAILED(xaml_solid_brush_new(fillc.g > 127 ? black_back : white_fore, &btext));
                xaml_ptr<xaml_solid_brush> bfill;
                XAML_RETURN_IF_FAILED(xaml_solid_brush_new(fillc, &bfill));
                xaml_rectangle round_rect = xaml_rectangle{ (double)(x * balls_side_length + 5), (double)(y * balls_side_length + 5), (double)(balls_side_length - 11), (double)(balls_side_length - 11) } * extend;
                round_rect.x += dx;
                round_rect.y += dy;
                xaml_size radius = xaml_size{ 10, 10 } * extend;
                XAML_RETURN_IF_FAILED(dc->fill_round_rect(bfill, round_rect, radius));
                XAML_RETURN_IF_FAILED(dc->draw_round_rect(pborder, round_rect, radius));
                xaml_ptr<xaml_string> text;
                XAML_RETURN_IF_FAILED(xaml_string_new(to_string(t), &text));
                XAML_RETURN_IF_FAILED(dc->draw_string(btext, font, center, text));
            }
            else
            {
                xaml_ptr<xaml_solid_brush> bcircle;
                XAML_RETURN_IF_FAILED(xaml_solid_brush_new(get_circle_fill(t), &bcircle));
                double r = balls_num_size * extend / 2;
                xaml_rectangle circle_rect = { dx + real_c.x - r, dy + real_c.y - r, 2 * r, 2 * r };
                if (t < 0 && t >= balls_special_double_score)
                {
                    XAML_RETURN_IF_FAILED(dc->fill_ellipse(bcircle, circle_rect));
                    XAML_RETURN_IF_FAILED(dc->draw_ellipse(pborder, circle_rect));
                }
                xaml_ptr<xaml_solid_brush> bfore;
                XAML_RETURN_IF_FAILED(xaml_solid_brush_new(white_fore, &bfore));
                xaml_ptr<xaml_brush_pen> pfore;
                XAML_RETURN_IF_FAILED(xaml_brush_pen_new(bfore, 3, &pfore));
                switch (t)
                {
                case balls_special_new_ball:
                {
                    double length = (balls_num_size - 20) * extend / 2;
                    XAML_RETURN_IF_FAILED(dc->draw_line(pfore, { center.x, center.y - length }, { center.x, center.y + length }));
                    XAML_RETURN_IF_FAILED(dc->draw_line(pfore, { center.x - length, center.y }, { center.x + length, center.y }));
                    break;
                }
                case balls_special_delete_ball:
                {
                    double length = (balls_num_size - 20) * extend / 2;
                    XAML_RETURN_IF_FAILED(dc->draw_line(pfore, { center.x - length, center.y }, { center.x + length, center.y }));
                    break;
                }
                case balls_special_random_turn:
                case balls_special_random_turn_old:
                {
                    xaml_ptr<xaml_string> text;
                    XAML_RETURN_IF_FAILED(xaml_string_new(U("?"), &text));
                    XAML_RETURN_IF_FAILED(dc->draw_string(bfore, font, center, text));
                    break;
                }
                case balls_special_double_score:
                {
                    xaml_ptr<xaml_string> text;
                    XAML_RETURN_IF_FAILED(xaml_string_new(U("$"), &text));
                    XAML_RETURN_IF_FAILED(dc->draw_string(bfore, font, center, text));
                    break;
                }
                }
            }
        }
    }
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

xaml_result balls_main_window_impl::on_window_size_changed(xaml_ptr<xaml_window>, xaml_size) noexcept
{
    if (m_canvas)
    {
        xaml_size size;
        XAML_RETURN_IF_FAILED(m_canvas->get_size(&size));
        double sidew = (size.width + 1) / balls_max_columns;
        double sideh = (size.height + 1) / balls_max_rows;
        double sidel = (min)(sidew, sideh);
        dw = sidel * balls_max_columns - 1;
        dh = sidel * balls_max_rows - 1;
        dx = (size.width - dw) / 2;
        dy = (size.height - dh) / 2;
        return m_canvas->invalidate();
    }
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::on_window_closing(xaml_ptr<xaml_window>, xaml_ptr<xaml_box> box) noexcept
{
    bool over;
    XAML_RETURN_IF_FAILED(m_map->get_is_over(&over));
    if (!over)
    {
        bool started;
        XAML_RETURN_IF_FAILED(m_timer->get_is_enabled(&started));
        XAML_RETURN_IF_FAILED(m_timer->stop());
        bool handled;
        XAML_RETURN_IF_FAILED(show_close(&handled));
        XAML_RETURN_IF_FAILED(box->set_value(handled));
        if (started) XAML_RETURN_IF_FAILED(m_timer->start());
    }
    return XAML_S_OK;
}

xaml_result balls_main_window_impl::on_timer_tick(xaml_ptr<xaml_timer>) noexcept
{
    bool move;
    XAML_RETURN_IF_FAILED(m_enumerator->move_next(&move));
    if (!move)
    {
        m_enumerator = nullptr;
        XAML_RETURN_IF_FAILED(m_timer->stop());
        bool reset;
        XAML_RETURN_IF_FAILED(m_map->reset(&reset));
        if (!reset)
        {
            bool cont;
            XAML_RETURN_IF_FAILED(show_stop(&cont));
            if (cont)
            {
                bool show;
                XAML_RETURN_IF_FAILED(init_balls(&show));
                if (!show) return m_window->close();
            }
            else
            {
                return m_window->close();
            }
        }
    }
    return m_canvas->invalidate();
}

static constexpr xaml_point get_com_point(xaml_point const& delta, xaml_point const& rp, double extend) noexcept
{
    return (rp - delta) * extend;
}

xaml_result XAML_CALL balls_main_window_impl::on_canvas_mouse_up(xaml_ptr<xaml_canvas>, xaml_mouse_button button) noexcept
{
    switch (button)
    {
    case xaml_mouse_button_left:
        if (!m_enumerator)
        {
            double extend = balls_client_width / dw;
            xaml_point cp = get_com_point({ dx, dy }, mouse_location, extend);
            XAML_RETURN_IF_FAILED(m_map->start_by(cp, &m_enumerator));
            XAML_RETURN_IF_FAILED(m_timer->start());
        }
        break;
    case xaml_mouse_button_right:
        if (m_enumerator)
        {
            bool enabled;
            XAML_RETURN_IF_FAILED(m_timer->get_is_enabled(&enabled));
            if (enabled)
            {
                XAML_RETURN_IF_FAILED(m_timer->stop());
            }
            else
            {
                XAML_RETURN_IF_FAILED(m_timer->start());
            }
        }
        break;
    }
    return XAML_S_OK;
}

xaml_result XAML_CALL balls_main_window_impl::on_canvas_mouse_move(xaml_ptr<xaml_canvas>, xaml_point p) noexcept
{
    mouse_location = p;
    double extend = balls_client_width / dw;
    xaml_point cp = get_com_point({ dx, dy }, mouse_location, extend);
    XAML_RETURN_IF_FAILED(m_map->set_sample(cp));
    if (!m_enumerator)
    {
        XAML_RETURN_IF_FAILED(m_canvas->invalidate());
    }
    return XAML_S_OK;
}

xaml_result XAML_CALL balls_main_window_new(balls_main_window** ptr) noexcept
{
    return xaml_object_init<balls_main_window_impl>(ptr);
}
