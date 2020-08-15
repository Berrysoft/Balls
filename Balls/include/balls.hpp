#ifndef BALLS_HPP
#define BALLS_HPP

#include <loopvar.h>
#include <random>
#include <serialstream.h>
#include <xaml/ui/drawing.h>

struct ball
{
    xaml_point pos;
    xaml_point speed;
};

constexpr int radius = 20; //球的半径
constexpr int max_c = 6; //方块行数
constexpr int max_r = 8; //方块列数
constexpr int side_length = 200; //方块边长
constexpr int client_width = side_length * max_c - 1; //用户区宽度
constexpr int client_height = side_length * max_r - 1; //用户区高度
constexpr double abs_speed = 50.0; //速度（像素/帧）
constexpr int num_height = 100; //数字字号

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

enum difficulty
{
    simple,
    normal,
    hard
};

constexpr int id_new_ball = -1; //+
constexpr int id_delete_ball = -2; //-
constexpr int id_random_turn = -3; //? 没有被触发
constexpr int id_random_turn_old = -4; //? 触发后
constexpr int id_double_score = -5; //￥

class balls_iterator;

struct balls_changed_args
{
    int ball_num;
    int remain_ball_num;
    std::size_t score;
};

class balls
{
private:
    observable<int> balln; //下一轮发射球的个数
    observable<int> remain_balln;
    point startp; //起始位置
    point endp; //下一次的起始位置
    vec startv; //起始速度
    point sampleb; //示例球
    bool dbscore; //是否分数加倍
    observable<std::size_t> mscore; //分数
    difficulty dfct; //难度
    std::vector<std::vector<int>> squares; //方块/控制圆/啥也没有
    std::mt19937 rnd; //随机数发生器
    std::uniform_int_distribution<int> idxd; //控制特殊控制圆的位置的随机分布（0-5）
    std::uniform_real_distribution<double> prob; //控制特殊控制圆出现的概率（0-1）

    bounce_side get_bounce_side(int c, int r); //计算那12种可能

public:
    balls();
    virtual ~balls() {}

    constexpr int ball_num() { return balln; }
    void ball_num(int n) { balln = n; }
    constexpr int remain_ball_num() { return remain_balln; }
    constexpr std::size_t score() { return mscore; }
    void score(std::size_t s) { mscore = s; }
    constexpr const point& start_pos() { return startp; }
    constexpr const point& end_pos() { return endp; }
    constexpr const vec& start_veh() { return startv; }
    void start_veh(const vec& v) { startv = v; }
    constexpr const point& sample_ball() { return sampleb; }
    void sample_ball(const point& p) { sampleb = p; }
    constexpr bool double_score() { return dbscore; }
    constexpr const std::vector<std::vector<int>>& get_squares() { return squares; }
    constexpr const difficulty game_dfct() { return dfct; }
    void game_dfct(difficulty d) { dfct = d; }

    friend class balls_iterator;

    balls_iterator iterator();
    balls_iterator iterator(int x, int y);

    bool over() const;
    bool reset();
    void reset_all();

    vec get_start(int x, int y, double speed = abs_speed) const;
    void set_sample(int x, int y);

private:
    void balln_changed(observable<int>&, const int& n)
    {
        on_ball_score_changed(*this, { n, remain_balln, mscore });
    }
    void remain_balln_changed(observable<int>&, const int& n)
    {
        on_ball_score_changed(*this, { balln, n, mscore });
    }
    void score_changed(observable<std::size_t>&, const std::size_t& s)
    {
        on_ball_score_changed(*this, { balln, remain_balln, s });
    }

    EVENT_SENDER_E(ball_score_changed, balls&, const balls_changed_args&)

public:
    friend serialstream& operator<<(serialstream& stream, balls& balls);
    friend serialstream& operator>>(serialstream& stream, balls& balls);
};

class balls_iterator
{
private:
    int balln; //本轮总球数
    int endn; //因为各种原因下场的球数
    loopvar<int> loop; //循环变量，控制是否发球
    std::vector<ball> bp; //在场的球
    balls* base; //创建这个迭代器的实例指针

public:
    balls_iterator() = default;
    balls_iterator(balls* base) : balln(base->balln), endn(0), loop(3, 0, 3), base(base) {}

    constexpr const std::vector<ball>& operator*() { return bp; }
    constexpr const std::vector<ball>* operator->() { return &bp; }

    constexpr operator bool() const { return balln > endn; }

    bool end_shooting() const { return (int)bp.size() + endn >= balln; }

private:
    bool bounce(ball& p);

    void increase_base_score();

public:
    balls_iterator& operator++();
    balls_iterator operator++(int)
    {
        balls_iterator result = *this;
        operator++();
        return result;
    }

public:
    friend serialstream& operator<<(serialstream& stream, balls_iterator& it);
    friend serialstream& operator>>(serialstream& stream, balls_iterator& it);
};

#endif // !BALLS_HPP
