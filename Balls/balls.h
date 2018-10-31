#pragma once
#include "loopvar.h"
#include "observable.h"
#include <event.h>
#include <random>

typedef struct point vec;

struct point
{
    double x;
    double y;

    friend constexpr bool operator==(const point& p1, const point& p2) { return p1.x == p2.x && p1.y == p2.y; }
    friend constexpr bool operator!=(const point& p1, const point& p2) { return !operator==(p1, p2); }

    constexpr point operator+() { return *this; }
    constexpr point operator-() { return { -x, -y }; }

    friend constexpr point operator+(const point& p, const vec& v) { return { p.x + v.x, p.y + v.y }; }
    friend constexpr point operator-(const point& p, const vec& v) { return { p.x - v.x, p.y - v.y }; }

    point& operator+=(const vec& v)
    {
        x += v.x;
        y += v.y;
        return *this;
    }
    point& operator-=(const vec& v)
    {
        x -= v.x;
        y -= v.y;
        return *this;
    }

    operator POINT() const;
    double size() const;
};

std::ostream& operator<<(std::ostream& stream, point& p);
std::istream& operator>>(std::istream& stream, point& p);

struct ball
{
    point pos;
    vec speed;
};

constexpr int radius = 10; //球的半径
constexpr int max_c = 6; //方块行数
constexpr int max_r = 8; //方块列数
constexpr int side_length = 100; //方块边长
constexpr int client_width = side_length * max_c - 1; //用户区宽度
constexpr int client_height = side_length * max_r - 1; //用户区高度
constexpr double abs_speed = 25.0; //速度（像素/帧）

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

#define ID_NEWBALL (-1) //+
#define ID_DELBALL (-2) //-
#define ID_RNDTURN (-3) //? 没有被触发
#define ID_OLDTURN (-4) //? 触发后
#define ID_DBSCORE (-5) //￥

class balls_iterator;

struct balls_changed_args
{
    int ball_num;
    std::size_t score;
};

class balls
{
private:
    observable<int> balln; //下一轮发射球的个数
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
        balls_changed_args args = { n, mscore };
        on_ball_score_changed(*this, args);
    }
    void score_changed(observable<std::size_t>&, const std::size_t& s)
    {
        balls_changed_args args = { balln, s };
        on_ball_score_changed(*this, args);
    }

    EVENT_SENDER_E(ball_score_changed, balls&, const balls_changed_args&)

public:
    friend std::ostream& operator<<(std::ostream& stream, balls& balls);
    friend std::istream& operator>>(std::istream& stream, balls& balls);
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

    bool end_shooting() const { return bp.size() + endn >= (std::size_t)balln; }

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
    friend std::ostream& operator<<(std::ostream& stream, balls_iterator& it);
    friend std::istream& operator>>(std::istream& stream, balls_iterator& it);
};
