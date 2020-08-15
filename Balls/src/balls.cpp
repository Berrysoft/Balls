#include <balls.h>
#include <ctime>
#include <random>
#include <xaml/event.h>

using namespace std;

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

    xaml_result XAML_CALL start(balls_map_enumerator**) noexcept;
    xaml_result XAML_CALL reset(bool*) noexcept;
    xaml_result XAML_CALL reset_all() noexcept;

    xaml_result XAML_CALL init() noexcept;

    bounce_side get_bounce_side(int32_t c, int32_t r) noexcept;

    int32_t m_squares[balls_max_columns][balls_max_rows];

    mt19937 m_random;
    uniform_int_distribution<int> m_index_dist;
    uniform_real_distribution<double> m_prob_dist;
};

struct balls_map_impl : xaml_implement<balls_map_impl, balls_map, xaml_object>
{
    balls_map_internal m_internal;

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

    xaml_result XAML_CALL start(balls_map_enumerator** ptr) noexcept override { return m_internal.start(ptr); }
    xaml_result XAML_CALL reset(bool* pvalue) noexcept override { return m_internal.reset(pvalue); }
    xaml_result XAML_CALL reset_all() noexcept override { return m_internal.reset_all(); }
};

xaml_result XAML_CALL balls_map_new(balls_map** ptr) noexcept
{
    return xaml_object_init<balls_map_impl>(ptr);
}

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

balls_map_internal::balls_map_internal()
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
static constexpr bounce_side get_side(const xaml_point& pos, int ls, int ts, int rs, int bs) noexcept
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

bool balls_iterator::bounce(ball& p)
{
    //格子坐标
    int c = (int)round(p.pos.x) / side_length;
    int r = (int)round(p.pos.y) / side_length;
    //四个边界坐标
    int ls = c * side_length;
    int rs = (c + 1) * side_length - 1;
    int ts = r * side_length;
    int bs = (r + 1) * side_length - 1;
    //获取当前格子边界情况
    bounce_side bside = base->get_bounce_side(c, r);

    //处理控制圆
    if (base->squares[r][c] < 0)
    {
        point center = { (double)(ls + side_length / 2), (double)(ts + side_length / 2) };
        double dis = (p.pos - center).size();
        if (dis <= num_height / 2 + radius + 10)
        {
            switch (base->squares[r][c])
            {
            case ID_NEWBALL:
                ++base->balln;
                base->squares[r][c] = 0;
                break;
            case ID_DELBALL:
                base->squares[r][c] = 0;
                return true;
            case ID_RNDTURN:
            case ID_OLDTURN:
            {
                //控制随机转向的角度分布，是一个正态分布
                //如果是平均分布，难度太大，没有游戏体验
                normal_distribution<double> thetad(atan(p.speed.y / p.speed.x) + p.speed.x < 0 ? 0 : PI, PI / 2);
                double theta = thetad(base->rnd);
                double x = abs_speed * cos(theta);
                double y = abs_speed * sin(theta);
                p.speed = { x, y };
                base->squares[r][c] = ID_OLDTURN; //触发后状态
                break;
            }
            case ID_DBSCORE:
                base->dbscore = true;
                base->squares[r][c] = 0;
                break;
            }
        }
    }

    //位置增加一个速度
    p.pos += p.speed;
    //看看现在有没有碰壁
    bounce_side nbside = get_side(p.pos, ls, ts, rs, bs);

    //是不是碰壁不仅取决于个球的努力
    //还要看有没有壁
    if ((bside & left_s) && (nbside & left_s))
    {
        change_ball(p.speed.x, p.pos.x, ls, true);
        if (c > 0)
        {
            base->squares[r][c - 1]--;
            increase_base_score();
        }
    }
    else if ((bside & right_s) && (nbside & right_s))
    {
        change_ball(p.speed.x, p.pos.x, rs, false);
        if (c < max_c - 1)
        {
            base->squares[r][c + 1]--;
            increase_base_score();
        }
    }

    if ((bside & top_s) && (nbside & top_s))
    {
        change_ball(p.speed.y, p.pos.y, ts, true);
        if (r > 0)
        {
            base->squares[r - 1][c]--;
            increase_base_score();
        }
    }
    //如果到底
    else if ((bside & bottom_s) && p.pos.y + radius >= client_height && p.speed.y > 0)
    {
        return true;
    }
    else if ((bside & bottom_s) && (nbside & bottom_s))
    {
        change_ball(p.speed.y, p.pos.y, bs, false);
        if (r < max_r - 1)
        {
            base->squares[r + 1][c]--;
            increase_base_score();
        }
    }

    if ((bside & left_top) && ((nbside & lt_s) == lt_s))
    {
        point sc = { (double)(ls + radius), (double)(ts + radius) };
        change_ball_arc(p, sc, false);
        base->squares[r - 1][c - 1]--;
        increase_base_score();
    }
    else if ((bside & right_top) && ((nbside & rt_s) == rt_s))
    {
        point sc = { (double)(rs - radius), (double)(ts + radius) };
        change_ball_arc(p, sc, true);
        base->squares[r - 1][c + 1]--;
        increase_base_score();
    }
    else if ((bside & left_bottom) && ((nbside & lb_s) == lb_s))
    {
        point sc = { (double)(ls + radius), (double)(bs - radius) };
        change_ball_arc(p, sc, true);
        base->squares[r + 1][c - 1]--;
        increase_base_score();
    }
    else if ((bside & right_bottom) && ((nbside & rb_s) == rb_s))
    {
        point sc = { (double)(rs - radius), (double)(bs - radius) };
        change_ball_arc(p, sc, false);
        base->squares[r + 1][c + 1]--;
        increase_base_score();
    }

    //Y方向速度不能太小
    if (abs(p.speed.y) < 1)
    {
        p.speed.y = p.speed.y > 0 ? 1 : -1;
    }
    return false;
}

void balls_iterator::increase_base_score()
{
    base->dbscore ? base->mscore += 2 : ++base->mscore;
}

balls_iterator& balls_iterator::operator++()
{
    ++loop;
    if (!end_shooting() && !loop)
    {
        //增加一个新的球
        bp.push_back({ base->startp, base->startv });
        --base->remain_balln;
    }
    for (auto it = bp.begin(); it != bp.end();)
    {
        auto& p = *it;
        if (bounce(p)) //碰撞
        {
            endn++;
            if (p.pos.y + radius >= client_height && base->endp == base->startp)
            {
                //计算下一个起始位置
                point tp = p.pos - p.speed;
                double h = client_height - radius - tp.y;
                double a = h / p.speed.y * p.speed.x;
                base->endp = { tp.x + a, tp.y + h };
                //球不能超过边界
                if (base->endp.x > client_width - radius)
                {
                    base->endp.x = client_width - radius;
                }
                else if (base->endp.x < radius)
                {
                    base->endp.x = radius;
                }
            }
            it = bp.erase(it); //销毁一个球
        }
        else
        {
            ++it;
        }
    }
    return *this;
}

balls_iterator balls::iterator()
{
    return balls_iterator(this); //返回一个新的迭代器
}

balls_iterator balls::iterator(int x, int y)
{
    startv = get_start(x, y);
    return iterator();
}

bool balls::over() const
{
    //如果最后一行有格子，游戏结束
    for (int c = 0; c < max_c; c++)
    {
        if (squares[max_r - 1][c] > 0)
        {
            return true;
        }
    }
    return false;
}

bool balls::reset()
{
    remain_balln = (int)balln;
    startp = endp; //起始位置设置为下一个起始位置
    dbscore = false; //分数加倍取消
    //复制每一个格子的值到下一行
    for (int r = max_r - 1; r > 0; r--)
    {
        for (int c = 0; c < max_c; c++)
        {
            int t = squares[r - 1][c];
            //如果不是触发过的？，才复制到下一行
            if (t != ID_OLDTURN)
            {
                squares[r][c] = t;
            }
            else
            {
                squares[r][c] = 0;
            }
        }
    }

    if (over())
        return false;

    //临时声明一个正态分布
    //三个难度在游戏开始有所介绍
    normal_distribution<double> distr;
    switch (dfct)
    {
    case simple:
        distr = normal_distribution<double>(balln / 2.0, balln / 6.0);
        break;
    case normal:
        distr = normal_distribution<double>(balln, balln / 3.0);
        break;
    case hard:
        distr = normal_distribution<double>(balln * 1.5, balln / 2.0);
        break;
    }
    //分布格子，只取正值
    for (int c = 0; c < max_c; c++)
    {
        if (prob(rnd) < 0.6)
        {
            int v = (int)round(distr(rnd));
            squares[0][c] = v >= 0 ? v : 0;
        }
        else
        {
            squares[0][c] = 0;
        }
    }
    //这里都是根据一些预定义的概率，
    //生成各种控制圆
    //随机转向
    if (prob(rnd) < 0.5)
    {
        int nindex = idxd(rnd);
        squares[0][nindex] = ID_RNDTURN;
    }
    //减号
    if (prob(rnd) < 0.2)
    {
        int nindex = idxd(rnd);
        squares[0][nindex] = ID_DELBALL;
    }
    //双倍分
    if (prob(rnd) < 0.2)
    {
        int nindex = idxd(rnd);
        squares[0][nindex] = ID_DBSCORE;
    }
    //加号，球数不能太多
    if (balln < INT32_MAX / 2)
    {
        if (prob(rnd) < 0.5)
        {
            int nindex = idxd(rnd);
            squares[0][nindex] = ID_NEWBALL;
        }
    }
    return true;
}

void balls::reset_all()
{
    //所有变量均回归原始
    balln = 1;
    remain_balln = (int)balln;
    startp = { client_width / 2, client_height - radius };
    endp = startp;
    for (int r = 0; r < max_r; r++)
    {
        for (int c = 0; c < max_c; c++)
        {
            squares[r][c] = 0;
        }
    }
    mscore = 0;
}

vec balls::get_start(int x, int y, double speed) const
{
    //简单的线性关系
    double xx = x - startp.x;
    double yy = y - startp.y;
    double l = sqrt(xx * xx + yy * yy);
    vec result = { speed / l * xx, speed / l * yy };
    //其实还是控制速度的y分量小于-1
    if (result.y > -speed / abs_speed)
    {
        result.y = -speed / abs_speed;
    }
    return result;
}

void balls::set_sample(int x, int y)
{
    //速度设置一个适当小的值
    //这样后期不需要调整
    vec v = get_start(x, y, 0.5);
    point tp = startp;
    int c1, c2, r;
    do
    {
        tp += v;
        c1 = (int)round(tp.x - radius) / side_length; //左上角
        c2 = (int)round(tp.x + radius) / side_length; //右上角
        r = (int)round(tp.y - radius) / side_length;
    } while (tp.x >= radius && tp.x <= client_width - radius && tp.y >= radius && //不超过边界
             (c1 >= 0 && c1 < max_c && squares[r][c1] <= 0) && //判断左上角
             (c2 >= 0 && c2 < max_c && squares[r][c2] <= 0)); //判断右上角
    sampleb = tp - v; //越过了所以要减回来
}

serialstream& operator<<(serialstream& stream, balls_iterator& it)
{
    stream << it.balln;
    stream << it.endn;
    stream << (int)it.loop;
    stream << it.bp.size();
    for (ball& b : it.bp)
    {
        stream << b;
    }
    return stream;
}

serialstream& operator>>(serialstream& stream, balls_iterator& it)
{
    stream >> it.balln;
    stream >> it.endn;
    int tloop;
    stream >> tloop;
    it.loop = tloop;
    size_t n;
    stream >> n;
    it.bp.clear();
    while (n--)
    {
        ball tb;
        stream >> tb;
        it.bp.push_back(tb);
    }
    return stream;
}
