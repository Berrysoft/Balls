#include <balls.h>
#include <ctime>
#include <loopvar.hpp>
#include <numbers>
#include <random>
#include <xaml/event.h>

using namespace std;
using std::numbers::pi;

enum bounce_side
{
    left_s = 0x1, //左边
    top_s = 0x2, //上边
    right_s = 0x4, //右边
    bottom_s = 0x8, //下边
    lt_s = left_s | top_s, //左上角
    rt_s = right_s | top_s, //右上角
    lb_s = left_s | bottom_s, //左下角
    rb_s = right_s | bottom_s, //右下角
    left_top = 0x10, //在左上角遇到凸直角
    right_top = 0x20, //在右上角遇到凸直角
    left_bottom = 0x40, //在左下角遇到凸直角
    right_bottom = 0x80 //在右下角遇到凸直角
};

struct balls_map_internal
{
    xaml_object* m_outer_this{ nullptr };

    balls_map_internal() noexcept;

    virtual ~balls_map_internal() {}

    XAML_EVENT_IMPL(ball_score_changed)

    XAML_PROP_IMPL_BASE(ball_num, int32_t, int32_t*)
    XAML_PROP_IMPL_BASE(remain_ball_num, int32_t, int32_t*)
    XAML_PROP_IMPL_BASE(score, int32_t, int32_t*)

    xaml_result XAML_CALL set_ball_num(int32_t value) noexcept
    {
        if (m_ball_num != value)
        {
            m_ball_num = value;
            return on_ball_score_changed(m_outer_this, balls_ball_score_changed_args{ m_ball_num, m_remain_ball_num, m_score });
        }
        return XAML_S_OK;
    }

    xaml_result XAML_CALL set_remain_ball_num(int32_t value) noexcept
    {
        if (m_remain_ball_num != value)
        {
            m_remain_ball_num = value;
            return on_ball_score_changed(m_outer_this, balls_ball_score_changed_args{ m_ball_num, m_remain_ball_num, m_score });
        }
        return XAML_S_OK;
    }

    xaml_result XAML_CALL set_score(int32_t value) noexcept
    {
        if (m_score != value)
        {
            m_score = value;
            return on_ball_score_changed(m_outer_this, balls_ball_score_changed_args{ m_ball_num, m_remain_ball_num, m_score });
        }
        return XAML_S_OK;
    }

    XAML_PROP_IMPL(difficulty, balls_difficulty, balls_difficulty*, balls_difficulty)

    XAML_PROP_IMPL(start_position, xaml_point, xaml_point*, xaml_point const&)
    XAML_PROP_IMPL(end_position, xaml_point, xaml_point*, xaml_point const&)
    XAML_PROP_IMPL(start_velocity, xaml_point, xaml_point*, xaml_point const&)
    XAML_PROP_IMPL(sample_position, xaml_point, xaml_point*, xaml_point const&)

    XAML_PROP_IMPL_BASE(is_double_score, bool, bool*)
    XAML_PROP_PTR_IMPL_BASE(map, xaml_vector)

    xaml_result XAML_CALL get_is_over(bool*) noexcept;

    xaml_result XAML_CALL start(balls_map_enumerator**) noexcept;
    xaml_result XAML_CALL start_by(xaml_point const&, balls_map_enumerator**) noexcept;
    xaml_result XAML_CALL reset(bool*) noexcept;
    xaml_result XAML_CALL reset_all() noexcept;

    xaml_point get_start(xaml_point const& p, double speed) const noexcept;

    xaml_result set_sample(xaml_point const& p) noexcept;

    xaml_result XAML_CALL init() noexcept;

    bounce_side get_bounce_side(int32_t c, int32_t r) noexcept;

    balls_map_t m_squares{};

    mt19937 m_random;
    uniform_int_distribution<int32_t> m_index_dist;
    uniform_real_distribution<double> m_prob_dist;

    xaml_result XAML_CALL get_map(balls_map_t const** ptr) noexcept
    {
        *ptr = &m_squares;
        return XAML_S_OK;
    }
};

struct balls_map_impl : xaml_implement<balls_map_impl, balls_map, xaml_object>
{
    balls_map_internal m_internal{};

    balls_map_impl() noexcept { m_internal.m_outer_this = this; }

    XAML_PROP_INTERNAL_IMPL(ball_num, int32_t*, int32_t)
    XAML_PROP_INTERNAL_IMPL(remain_ball_num, int32_t*, int32_t)
    XAML_PROP_INTERNAL_IMPL(score, int32_t*, int32_t)
    XAML_PROP_INTERNAL_IMPL(difficulty, balls_difficulty*, balls_difficulty)

    XAML_EVENT_INTERNAL_IMPL(ball_score_changed)

    XAML_PROP_INTERNAL_IMPL(start_position, xaml_point*, xaml_point const&)
    XAML_PROP_INTERNAL_IMPL(end_position, xaml_point*, xaml_point const&)
    XAML_PROP_INTERNAL_IMPL(start_velocity, xaml_point*, xaml_point const&)
    XAML_PROP_INTERNAL_IMPL(sample_position, xaml_point*, xaml_point const&)

    XAML_PROP_INTERNAL_IMPL_BASE(is_double_score, bool*)
    XAML_PROP_INTERNAL_IMPL_BASE(map, xaml_vector**)

    XAML_PROP_INTERNAL_IMPL_BASE(is_over, bool*)
    XAML_PROP_INTERNAL_IMPL_BASE(map, balls_map_t const**)

    xaml_result XAML_CALL start(balls_map_enumerator** ptr) noexcept override { return m_internal.start(ptr); }
    xaml_result XAML_CALL start_by(xaml_point const& p, balls_map_enumerator** ptr) noexcept override { return m_internal.start_by(p, ptr); }
    xaml_result XAML_CALL reset(bool* pvalue) noexcept override { return m_internal.reset(pvalue); }
    xaml_result XAML_CALL reset_all() noexcept override { return m_internal.reset_all(); }

    xaml_result XAML_CALL set_sample(xaml_point const& p) noexcept override { return m_internal.set_sample(p); }

    xaml_result XAML_CALL init() noexcept { return m_internal.init(); }
};

xaml_result XAML_CALL balls_map_new(balls_map** ptr) noexcept
{
    return xaml_object_init<balls_map_impl>(ptr);
}

struct balls_map_enumerator_impl : xaml_implement<balls_map_enumerator_impl, balls_map_enumerator, xaml_enumerator, xaml_object>
{
    balls_map_internal* m_base;

    int32_t m_ball_num;
    int32_t m_stopped_num;
    loopvar<int32_t> m_loop;
    xaml_ptr<xaml_vector> m_current_balls;

    balls_map_enumerator_impl() noexcept : m_stopped_num(0), m_loop(3, 0, 3) {}

    xaml_result XAML_CALL move_next(bool* pvalue) noexcept override;
    xaml_result XAML_CALL get_current(xaml_object** ptr) noexcept override { return m_current_balls.query(ptr); }

    xaml_result XAML_CALL bounce(balls_ball& p, bool* pvalue) noexcept;
    xaml_result XAML_CALL increase_base_score() noexcept;

    xaml_result XAML_CALL get_is_end_shooting(bool* pvalue) noexcept override
    {
        int32_t size;
        XAML_RETURN_IF_FAILED(m_current_balls->get_size(&size));
        *pvalue = size + m_stopped_num >= m_ball_num;
        return XAML_S_OK;
    }

    xaml_result XAML_CALL init(balls_map_internal* base) noexcept
    {
        m_base = base;
        m_ball_num = m_base->m_ball_num;
        XAML_RETURN_IF_FAILED(xaml_vector_new(&m_current_balls));
        return XAML_S_OK;
    }
};

bounce_side balls_map_internal::get_bounce_side(int32_t c, int32_t r) noexcept
{
    int result = 0;
    //实际上只需考虑8种可能
    //前四种有可能同时有两个成立，这就是一种新的可能
    if (c == 0 || m_squares[r][c - 1] > 0)
    {
        result |= left_s;
    }
    if (r == 0 || m_squares[r - 1][c] > 0)
    {
        result |= top_s;
    }
    if (c == balls_max_columns - 1 || m_squares[r][c + 1] > 0)
    {
        result |= right_s;
    }
    if (r == balls_max_rows - 1 || m_squares[r + 1][c] > 0)
    {
        result |= bottom_s;
    }
    //需要保证是凸直角
    if (c > 0 && r > 0 && m_squares[r - 1][c - 1] > 0 && !(result & lt_s))
    {
        result |= left_top;
    }
    if (c < balls_max_columns - 1 && r > 0 && m_squares[r - 1][c + 1] > 0 && !(result & rt_s))
    {
        result |= right_top;
    }
    if (c > 0 && r < balls_max_rows - 1 && m_squares[r + 1][c - 1] > 0 && !(result & lb_s))
    {
        result |= left_bottom;
    }
    if (c < balls_max_columns - 1 && r < balls_max_rows - 1 && m_squares[r + 1][c + 1] > 0 && !(result & rb_s))
    {
        result |= right_bottom;
    }
    return (bounce_side)result;
}

balls_map_internal::balls_map_internal() noexcept
    : m_ball_num(1), m_start_position({ balls_client_width / 2, balls_client_height - balls_radius }), m_end_position(m_start_position),
      m_difficulty(balls_difficulty_normal), m_random((unsigned int)time(nullptr)), m_index_dist(0, balls_max_columns - 1), m_prob_dist(0, 1)
{
}

xaml_result balls_map_internal::init() noexcept
{
    XAML_RETURN_IF_FAILED(xaml_event_new(&m_ball_score_changed));
    return XAML_S_OK;
}

static constexpr void change_ball(double& speed, double& pos, int side, bool minus) noexcept
{
    speed = -speed; //速度反向
    double off = side - (pos + (minus ? -balls_radius : balls_radius)); //计算偏移量
    pos += 2 * off; //反向增加二倍偏移量
}

// TODO: move to XamlCpp repo
constexpr xaml_point operator-(const xaml_point& p) noexcept
{
    return { -p.x, -p.y };
}

static constexpr void change_ball_arc(balls_ball& p, const xaml_point& sc, bool minus) noexcept
{
    xaml_point off = sc - p.pos; //两点差距
    xaml_point noff = { off.y, off.x }; //反弹后差距
    p.pos = sc + (minus ? -noff : noff); //反弹
    xaml_point ns = { -p.speed.y, -p.speed.x }; //速度反弹
    p.speed = minus ? -ns : ns; //修正
}

//获取一个球球心所在的格子周围的格子情况
static bounce_side get_side(const xaml_point& pos, int ls, int ts, int rs, int bs) noexcept
{
    int result = 0;
    //坐标整数化
    int x = (int)round(pos.x);
    int y = (int)round(pos.y);
    if (x - balls_radius <= ls)
    {
        result |= left_s;
    }
    if (x + balls_radius >= rs)
    {
        result |= right_s;
    }
    if (y - balls_radius <= ts)
    {
        result |= top_s;
    }
    if (bs < balls_client_height && y + balls_radius >= bs)
    {
        result |= bottom_s;
    }
    return (bounce_side)result;
}

static double distance(const xaml_point& p) noexcept
{
    return sqrt(p.x * p.x + p.y * p.y);
}

xaml_result balls_map_enumerator_impl::bounce(balls_ball& p, bool* pvalue) noexcept
{
    //格子坐标
    int c = (int)round(p.pos.x) / balls_side_length;
    int r = (int)round(p.pos.y) / balls_side_length;
    //四个边界坐标
    int ls = c * balls_side_length;
    int rs = (c + 1) * balls_side_length - 1;
    int ts = r * balls_side_length;
    int bs = (r + 1) * balls_side_length - 1;
    //获取当前格子边界情况
    bounce_side bside = m_base->get_bounce_side(c, r);

    //处理控制圆
    if (m_base->m_squares[r][c] < 0)
    {
        xaml_point center = { (double)(ls + balls_side_length / 2), (double)(ts + balls_side_length / 2) };
        double dis = distance(p.pos - center);
        if (dis <= balls_num_size / 2 + balls_radius + 10)
        {
            switch (m_base->m_squares[r][c])
            {
            case balls_special_new_ball:
                ++m_base->m_ball_num;
                m_base->m_squares[r][c] = 0;
                break;
            case balls_special_delete_ball:
                m_base->m_squares[r][c] = 0;
                *pvalue = true;
                return XAML_S_OK;
            case balls_special_random_turn:
            case balls_special_random_turn_old:
            {
                //控制随机转向的角度分布，是一个正态分布
                //如果是平均分布，难度太大，没有游戏体验
                normal_distribution<double> thetad(atan(p.speed.y / p.speed.x) + p.speed.x < 0 ? 0 : pi, pi / 2);
                double theta = thetad(m_base->m_random);
                double x = balls_abs_speed * cos(theta);
                double y = balls_abs_speed * sin(theta);
                p.speed = { x, y };
                m_base->m_squares[r][c] = balls_special_random_turn_old; //触发后状态
                break;
            }
            case balls_special_double_score:
                m_base->m_is_double_score = true;
                m_base->m_squares[r][c] = 0;
                break;
            }
        }
    }

    //位置增加一个速度
    p.pos = p.pos + p.speed;
    //看看现在有没有碰壁
    bounce_side nbside = get_side(p.pos, ls, ts, rs, bs);

    //是不是碰壁不仅取决于个球的努力
    //还要看有没有壁
    if ((bside & left_s) && (nbside & left_s))
    {
        change_ball(p.speed.x, p.pos.x, ls, true);
        if (c > 0)
        {
            m_base->m_squares[r][c - 1]--;
            XAML_RETURN_IF_FAILED(increase_base_score());
        }
    }
    else if ((bside & right_s) && (nbside & right_s))
    {
        change_ball(p.speed.x, p.pos.x, rs, false);
        if (c < balls_max_columns - 1)
        {
            m_base->m_squares[r][c + 1]--;
            XAML_RETURN_IF_FAILED(increase_base_score());
        }
    }

    if ((bside & top_s) && (nbside & top_s))
    {
        change_ball(p.speed.y, p.pos.y, ts, true);
        if (r > 0)
        {
            m_base->m_squares[r - 1][c]--;
            XAML_RETURN_IF_FAILED(increase_base_score());
        }
    }
    //如果到底
    else if ((bside & bottom_s) && p.pos.y + balls_radius >= balls_client_height && p.speed.y > 0)
    {
        *pvalue = true;
        return XAML_S_OK;
    }
    else if ((bside & bottom_s) && (nbside & bottom_s))
    {
        change_ball(p.speed.y, p.pos.y, bs, false);
        if (r < balls_max_rows - 1)
        {
            m_base->m_squares[r + 1][c]--;
            XAML_RETURN_IF_FAILED(increase_base_score());
        }
    }

    if ((bside & left_top) && ((nbside & lt_s) == lt_s))
    {
        xaml_point sc = { (double)(ls + balls_radius), (double)(ts + balls_radius) };
        change_ball_arc(p, sc, false);
        m_base->m_squares[r - 1][c - 1]--;
        XAML_RETURN_IF_FAILED(increase_base_score());
    }
    else if ((bside & right_top) && ((nbside & rt_s) == rt_s))
    {
        xaml_point sc = { (double)(rs - balls_radius), (double)(ts + balls_radius) };
        change_ball_arc(p, sc, true);
        m_base->m_squares[r - 1][c + 1]--;
        XAML_RETURN_IF_FAILED(increase_base_score());
    }
    else if ((bside & left_bottom) && ((nbside & lb_s) == lb_s))
    {
        xaml_point sc = { (double)(ls + balls_radius), (double)(bs - balls_radius) };
        change_ball_arc(p, sc, true);
        m_base->m_squares[r + 1][c - 1]--;
        XAML_RETURN_IF_FAILED(increase_base_score());
    }
    else if ((bside & right_bottom) && ((nbside & rb_s) == rb_s))
    {
        xaml_point sc = { (double)(rs - balls_radius), (double)(bs - balls_radius) };
        change_ball_arc(p, sc, false);
        m_base->m_squares[r + 1][c + 1]--;
        XAML_RETURN_IF_FAILED(increase_base_score());
    }

    //Y方向速度不能太小
    if (abs(p.speed.y) < 1)
    {
        p.speed.y = p.speed.y > 0 ? 1 : -1;
    }
    *pvalue = false;
    return XAML_S_OK;
}

xaml_result balls_map_enumerator_impl::increase_base_score() noexcept
{
    return m_base->set_score(m_base->m_score + (m_base->m_is_double_score ? 2 : 1));
}

xaml_result balls_map_enumerator_impl::move_next(bool* pvalue) noexcept
{
    ++m_loop;
    bool is_ends;
    XAML_RETURN_IF_FAILED(get_is_end_shooting(&is_ends));
    if (!is_ends && !m_loop)
    {
        //增加一个新的球
        balls_ball new_ball{ m_base->m_start_position, m_base->m_start_velocity };
        xaml_ptr<xaml_object> box;
        XAML_RETURN_IF_FAILED(xaml_box_value(new_ball, &box));
        XAML_RETURN_IF_FAILED(m_current_balls->append(box));
        XAML_RETURN_IF_FAILED(m_base->set_remain_ball_num(m_base->m_remain_ball_num - 1));
    }
    int32_t size;
    XAML_RETURN_IF_FAILED(m_current_balls->get_size(&size));
    for (int32_t i = 0; i < size;)
    {
        xaml_ptr<xaml_object> box_ball;
        XAML_RETURN_IF_FAILED(m_current_balls->get_at(i, &box_ball));
        xaml_ptr<xaml_box> box;
        XAML_RETURN_IF_FAILED(box_ball->query(&box));
        balls_ball ball;
        XAML_RETURN_IF_FAILED(box->get_value(&ball));
        bool b;
        XAML_RETURN_IF_FAILED(bounce(ball, &b));
        if (b)
        {
            m_stopped_num++;
            if (ball.pos.y + balls_radius >= balls_client_height && m_base->m_end_position == m_base->m_start_position)
            {
                //计算下一个起始位置
                xaml_point tp = ball.pos - ball.speed;
                double h = balls_client_height - balls_radius - tp.y;
                double a = h / ball.speed.y * ball.speed.x;
                XAML_RETURN_IF_FAILED(m_base->set_end_position({ tp.x + a, tp.y + h }));
                //球不能超过边界
                if (m_base->m_end_position.x > balls_client_width - balls_radius)
                {
                    m_base->m_end_position.x = balls_client_width - balls_radius;
                }
                else if (m_base->m_end_position.x < balls_radius)
                {
                    m_base->m_end_position.x = balls_radius;
                }
            }
            XAML_RETURN_IF_FAILED(m_current_balls->remove_at(i)); //销毁一个球
        }
        else
        {
            i++;
        }
        box->set_value(ball);
    }
    *pvalue = m_ball_num > m_stopped_num;
    return XAML_S_OK;
}

xaml_result balls_map_internal::start(balls_map_enumerator** ptr) noexcept
{
    return xaml_object_init<balls_map_enumerator_impl>(ptr, this);
}

xaml_result balls_map_internal::start_by(xaml_point const& p, balls_map_enumerator** ptr) noexcept
{
    m_start_velocity = get_start(p, balls_abs_speed);
    return start(ptr);
}

xaml_result balls_map_internal::get_is_over(bool* pvalue) noexcept
{
    //如果最后一行有格子，游戏结束
    for (int c = 0; c < balls_max_columns; c++)
    {
        if (m_squares[balls_max_rows - 1][c] > 0)
        {
            *pvalue = true;
            return XAML_S_OK;
        }
    }
    *pvalue = false;
    return XAML_S_OK;
}

xaml_result balls_map_internal::reset(bool* pvalue) noexcept
{
    XAML_RETURN_IF_FAILED(set_remain_ball_num(m_ball_num));
    m_start_position = m_end_position; //起始位置设置为下一个起始位置
    m_sample_position = m_start_position;
    m_is_double_score = false; //分数加倍取消
    //复制每一个格子的值到下一行
    for (int r = balls_max_rows - 1; r > 0; r--)
    {
        for (int c = 0; c < balls_max_columns; c++)
        {
            int t = m_squares[r - 1][c];
            //如果不是触发过的？，才复制到下一行
            if (t != balls_special_random_turn_old)
            {
                m_squares[r][c] = t;
            }
            else
            {
                m_squares[r][c] = 0;
            }
        }
    }

    bool is_over;
    XAML_RETURN_IF_FAILED(get_is_over(&is_over));
    if (is_over)
    {
        *pvalue = false;
        return XAML_S_OK;
    }

    //临时声明一个正态分布
    //三个难度在游戏开始有所介绍
    normal_distribution<double> distr;
    switch (m_difficulty)
    {
    case balls_difficulty_simple:
        distr = normal_distribution<double>(m_ball_num / 2.0, m_ball_num / 6.0);
        break;
    case balls_difficulty_normal:
        distr = normal_distribution<double>(m_ball_num, m_ball_num / 3.0);
        break;
    case balls_difficulty_hard:
        distr = normal_distribution<double>(m_ball_num * 1.5, m_ball_num / 2.0);
        break;
    }
    //分布格子，只取正值
    for (int32_t c = 0; c < balls_max_columns; c++)
    {
        if (m_prob_dist(m_random) < 0.6)
        {
            int32_t v = (int32_t)round(distr(m_random));
            m_squares[0][c] = v >= 0 ? v : 0;
        }
        else
        {
            m_squares[0][c] = 0;
        }
    }
    //这里都是根据一些预定义的概率，
    //生成各种控制圆
    //随机转向
    if (m_prob_dist(m_random) < 0.5)
    {
        int32_t nindex = m_index_dist(m_random);
        m_squares[0][nindex] = balls_special_random_turn;
    }
    //减号
    if (m_prob_dist(m_random) < 0.2)
    {
        int32_t nindex = m_index_dist(m_random);
        m_squares[0][nindex] = balls_special_delete_ball;
    }
    //双倍分
    if (m_prob_dist(m_random) < 0.2)
    {
        int32_t nindex = m_index_dist(m_random);
        m_squares[0][nindex] = balls_special_double_score;
    }
    //加号，球数不能太多
    if (m_ball_num < numeric_limits<int32_t>::max() / 2)
    {
        if (m_prob_dist(m_random) < 0.5)
        {
            int32_t nindex = m_index_dist(m_random);
            m_squares[0][nindex] = balls_special_new_ball;
        }
    }
    *pvalue = true;
    return XAML_S_OK;
}

xaml_result balls_map_internal::reset_all() noexcept
{
    //所有变量均回归原始
    m_ball_num = 1;
    XAML_RETURN_IF_FAILED(set_remain_ball_num(m_ball_num));
    m_start_position = { balls_client_width / 2, balls_client_height - balls_radius };
    m_end_position = m_start_position;
    memset(m_squares, 0, sizeof(m_squares));
    m_score = 0;
    return XAML_S_OK;
}

xaml_point balls_map_internal::get_start(const xaml_point& p, double speed) const noexcept
{
    //简单的线性关系
    xaml_point pp = p - m_start_position;
    double l = distance(pp);
    xaml_point result = speed / l * pp;
    //其实还是控制速度的y分量小于-1
    if (result.y > -speed / balls_abs_speed)
    {
        result.y = -speed / balls_abs_speed;
    }
    return result;
}

xaml_result balls_map_internal::set_sample(const xaml_point& p) noexcept
{
    //速度设置一个适当小的值
    //这样后期不需要调整
    xaml_point v = get_start(p, 0.5);
    xaml_point tp = m_start_position;
    int32_t c1, c2, r;
    do
    {
        tp = tp + v;
        c1 = (int32_t)round(tp.x - balls_radius) / balls_side_length; //左上角
        c2 = (int32_t)round(tp.x + balls_radius) / balls_side_length; //右上角
        r = (int32_t)round(tp.y - balls_radius) / balls_side_length;
    } while (tp.x >= balls_radius && tp.x <= balls_client_width - balls_radius && tp.y >= balls_radius && //不超过边界
             (c1 >= 0 && c1 < balls_max_columns && m_squares[r][c1] <= 0) && //判断左上角
             (c2 >= 0 && c2 < balls_max_columns && m_squares[r][c2] <= 0)); //判断右上角
    m_sample_position = tp - v; //越过了所以要减回来
    return XAML_S_OK;
}
