#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
#![feature(let_chains)]
#![feature(path_add_extension)]

use std::{cell::Cell, ffi::OsString, path::Path, rc::Rc, time::Duration};

use balls::{
    BallType, CLIENT_HEIGHT, CLIENT_WIDTH, Difficulty, Map, MapTicker, NUM_SIZE, RADIUS, SIDE,
    Special,
};
use compio::{
    fs::File,
    io::{AsyncReadAtExt, AsyncWriteAtExt},
    runtime::spawn,
    time::interval,
};
use winio::{
    App, BrushPen, Canvas, CanvasEvent, Child, Color, ColorTheme, Component, ComponentSender,
    CustomButton, DrawingFontBuilder, FileBox, HAlign, Layoutable, MessageBox, MessageBoxButton,
    MessageBoxResponse, MessageBoxStyle, MouseButton, Point, Rect, Size, SolidColorBrush, VAlign,
    Visible, Window, WindowEvent,
};

fn main() {
    App::new().run::<MainModel>(std::env::args_os().nth(1), &());
}

#[derive(Debug)]
struct State {
    map: Map,
    ticker: Option<MapTicker>,
    drect: Rect,
    mouse: Point,
    timer_running: Rc<Cell<bool>>,
}

impl State {
    pub fn new() -> Self {
        Self {
            map: Map::default(),
            ticker: None,
            drect: Rect::zero(),
            mouse: Point::zero(),
            timer_running: Rc::new(Cell::new(false)),
        }
    }
}

impl State {
    pub fn set_drect(&mut self, canvas: &Canvas) {
        let size = canvas.size();
        let columns = self.map.column_len() as f64;
        let rows = self.map.row_len() as f64;
        let sidew = size.width / columns;
        let sideh = size.height / rows;
        let sidel = sidew.min(sideh);
        let dw = sidel * columns - 1.0;
        let dh = sidel * rows - 1.0;
        let dx = (size.width - dw) / 2.0;
        let dy = (size.height - dh) / 2.0;
        self.drect = Rect::new(Point::new(dx, dy), Size::new(dw, dh));
    }

    pub fn com_point(&self, rp: Point) -> Point {
        let extend = CLIENT_WIDTH / self.drect.width();
        (rp - self.drect.origin).to_point() * extend
    }

    pub fn ball_rect(&self, p: Point) -> Rect {
        let extend = self.drect.width() / CLIENT_WIDTH;
        let real_p = p * extend + self.drect.origin.to_vector();
        let r = RADIUS * extend;
        Rect::new(
            Point::new(real_p.x - r, real_p.y - r),
            Size::new(2.0 * r, 2.0 * r),
        )
    }
}

struct MainModel {
    window: Child<Window>,
    canvas: Child<Canvas>,
    state: State,
}

enum MainMessage {
    Noop,
    Startup(Option<OsString>),
    Tick,
    Close,
    Redraw,
    MouseMove(Point),
    MouseUp(MouseButton),
}

impl Component for MainModel {
    type Event = ();
    type Init = Option<OsString>;
    type Message = MainMessage;
    type Root = ();

    fn init(counter: Self::Init, _root: &Self::Root, sender: &ComponentSender<Self>) -> Self {
        let mut window = Child::<Window>::init((), &());
        window.set_size(Size::new(450.0, 600.0));

        #[cfg(windows)]
        window.set_icon_by_id(1);

        let canvas = Child::<Canvas>::init((), &window);

        let state = State::new();

        sender.post(MainMessage::Startup(counter));

        let sender = sender.clone();
        let timer_running = state.timer_running.clone();
        spawn(async move {
            let mut interval = interval(Duration::from_millis(10));
            loop {
                interval.tick().await;
                if timer_running.get() && !sender.post(MainMessage::Tick) {
                    break;
                }
            }
        })
        .detach();

        Self {
            window,
            canvas,
            state,
        }
    }

    async fn start(&mut self, sender: &ComponentSender<Self>) {
        let fut_window = self.window.start(
            sender,
            |e| match e {
                WindowEvent::Close => Some(MainMessage::Close),
                WindowEvent::Resize => Some(MainMessage::Redraw),
                _ => None,
            },
            || MainMessage::Noop,
        );
        let fut_canvas = self.canvas.start(
            sender,
            |e| match e {
                CanvasEvent::Redraw => Some(MainMessage::Redraw),
                CanvasEvent::MouseMove(p) => Some(MainMessage::MouseMove(p)),
                CanvasEvent::MouseUp(b) => Some(MainMessage::MouseUp(b)),
                _ => None,
            },
            || MainMessage::Noop,
        );
        futures_util::future::join(fut_window, fut_canvas).await;
    }

    async fn update(&mut self, message: Self::Message, sender: &ComponentSender<Self>) -> bool {
        match message {
            MainMessage::Noop => false,
            MainMessage::Redraw => true,
            MainMessage::Startup(p) => {
                if let Some(path) = p
                    && compio::fs::metadata(&path).await.is_ok()
                {
                    if !open_record(&self.window, &path, &mut self.state).await {
                        sender.output(());
                    }
                } else if !init_balls(&self.window, &mut self.state).await {
                    sender.output(());
                }
                self.window.show();
                true
            }
            MainMessage::MouseMove(p) => {
                self.state.mouse = p;
                if self.state.ticker.is_none() {
                    self.state.map.update_sample(self.state.com_point(p));
                    true
                } else {
                    false
                }
            }
            MainMessage::MouseUp(b) => match b {
                MouseButton::Left => {
                    if self.state.ticker.is_none() {
                        self.state.ticker =
                            Some(self.state.map.start(self.state.com_point(self.state.mouse)));
                        self.state.timer_running.set(true);
                    }
                    true
                }
                MouseButton::Right => {
                    if self.state.ticker.is_some() {
                        self.state
                            .timer_running
                            .set(!self.state.timer_running.get());
                    }
                    true
                }
                _ => false,
            },
            MainMessage::Tick => {
                if let Some(mut ticker) = self.state.ticker.take() {
                    if ticker.tick(&mut self.state.map) {
                        self.state.ticker = Some(ticker);
                    } else {
                        ticker.consume(&mut self.state.map);
                        if self.state.map.reset() {
                            self.state
                                .map
                                .update_sample(self.state.com_point(self.state.mouse));
                        } else {
                            let difficulty = self.state.map.difficulty();
                            let balls_num = self.state.map.balls_num();
                            let score = self.state.map.score();
                            if !show_stop(&self.window, difficulty, balls_num, score).await
                                || !init_balls(&self.window, &mut self.state).await
                            {
                                sender.output(());
                            }
                        }
                    }
                    return true;
                }
                false
            }
            MainMessage::Close => {
                let running = self.state.timer_running.replace(false);

                match show_close(&self.window, &mut self.state).await {
                    ShowCloseResult::Close => {
                        sender.output(());
                        false
                    }
                    ShowCloseResult::Cancel => {
                        self.state.timer_running.set(running);
                        false
                    }
                    ShowCloseResult::Retry => {
                        if init_balls(&self.window, &mut self.state).await {
                            true
                        } else {
                            self.state.timer_running.set(running);
                            false
                        }
                    }
                }
            }
        }
    }

    fn render(&mut self, _sender: &ComponentSender<Self>) {
        {
            let difficulty = self.state.map.difficulty();
            let title = if let Some(ticker) = self.state.ticker.as_ref() {
                format!(
                    "二维弹球 - {} 球数：{} 剩余球数：{} 分数：{}",
                    difficulty_str(difficulty),
                    self.state.map.balls_num(),
                    ticker.remain(),
                    self.state.map.score()
                )
            } else {
                format!(
                    "二维弹球 - {} 球数：{} 分数：{}",
                    difficulty_str(difficulty),
                    self.state.map.balls_num(),
                    self.state.map.score()
                )
            };
            if self.window.text() != title {
                self.window.set_text(title);
            }
        }
        let csize = self.window.client_size();
        self.canvas.set_size(csize);
        self.canvas.set_loc(Point::zero());
        self.state.set_drect(&self.canvas);

        let size = self.canvas.size();
        let palette = Palette::current();
        let mut ctx = self.canvas.context();
        let brush = SolidColorBrush::new(palette.erase);
        ctx.fill_rect(brush, Rect::from_size(size));

        let drect = self.state.drect;
        let brush = SolidColorBrush::new(palette.back);
        ctx.fill_rect(brush, drect);

        let extend = drect.width() / CLIENT_WIDTH;
        let font = DrawingFontBuilder::new()
            .family("Arial")
            .size(NUM_SIZE * extend)
            .halign(HAlign::Center)
            .valign(VAlign::Center)
            .build();
        if self.state.ticker.is_none() {
            let sample_pos = self.state.map.sample();
            if sample_pos.x >= RADIUS
                && sample_pos.x <= CLIENT_WIDTH - RADIUS
                && sample_pos.y >= RADIUS
                && sample_pos.y <= CLIENT_HEIGHT - RADIUS
            {
                let brush = SolidColorBrush::new(palette.red_sample);
                ctx.fill_ellipse(brush, self.state.ball_rect(sample_pos));
            }
        }
        let bball = if self.state.map.doubled_score() {
            SolidColorBrush::new(palette.yellow_circle)
        } else {
            SolidColorBrush::new(palette.red_ball)
        };
        let start_pos = self.state.map.startp();
        let end_shooting = self
            .state
            .ticker
            .as_ref()
            .map(|ticker| ticker.is_end())
            .unwrap_or_default();
        if !end_shooting {
            ctx.fill_ellipse(&bball, self.state.ball_rect(start_pos));
        }
        if let Some(ticker) = self.state.ticker.as_ref() {
            if let Some(new_start) = ticker.new_startp() {
                ctx.fill_ellipse(&bball, self.state.ball_rect(new_start));
            }
            for b in ticker.balls() {
                ctx.fill_ellipse(&bball, self.state.ball_rect(b.pos));
            }
        }
        let pborder = BrushPen::new(SolidColorBrush::new(palette.green_border), 3.0);
        for (y, row) in self.state.map.balls().iter().enumerate() {
            for (x, t) in row.iter().enumerate() {
                let cx = x as f64 * SIDE + SIDE / 2.0;
                let cy = y as f64 * SIDE + SIDE / 2.0;
                let real_c = Point::new(cx, cy) * extend;
                let center = real_c + self.state.drect.origin.to_vector();
                match t {
                    BallType::None => {}
                    BallType::Normal(score) => {
                        let fillc = square_color(*score);
                        let btext: SolidColorBrush = SolidColorBrush::new(if fillc.g > 127 {
                            palette.fore_dark
                        } else {
                            palette.fore_light
                        });
                        let bfill = SolidColorBrush::new(fillc);
                        let mut round_rect = Rect::new(
                            Point::new(x as f64 * SIDE + 5.0, y as f64 * SIDE + 5.0),
                            Size::new(SIDE - 11.0, SIDE - 11.0),
                        );
                        round_rect *= extend;
                        round_rect.origin += drect.origin.to_vector();
                        let radius = Size::new(10.0, 10.0) * extend;
                        ctx.fill_round_rect(bfill, round_rect, radius);
                        ctx.draw_round_rect(&pborder, round_rect, radius);
                        ctx.draw_str(btext, font.clone(), center, score.to_string());
                    }
                    BallType::Special(special) => {
                        let bcircle = SolidColorBrush::new(special_color(*special));
                        let r = NUM_SIZE * extend / 2.0;
                        let circle_rect = Rect::new(
                            Point::new(real_c.x - r, real_c.y - r) + drect.origin.to_vector(),
                            Size::new(2.0 * r, 2.0 * r),
                        );
                        ctx.fill_ellipse(bcircle, circle_rect);
                        ctx.draw_ellipse(&pborder, circle_rect);
                        let bfore = SolidColorBrush::new(palette.fore_light);
                        let pfore = BrushPen::new(&bfore, 3.0);
                        match special {
                            Special::New => {
                                let length = (NUM_SIZE - 20.0) * extend / 2.0;
                                ctx.draw_line(
                                    &pfore,
                                    Point::new(center.x, center.y - length),
                                    Point::new(center.x, center.y + length),
                                );
                                ctx.draw_line(
                                    &pfore,
                                    Point::new(center.x - length, center.y),
                                    Point::new(center.x + length, center.y),
                                );
                            }
                            Special::Delete => {
                                let length = (NUM_SIZE - 20.0) * extend / 2.0;
                                ctx.draw_line(
                                    &pfore,
                                    Point::new(center.x - length, center.y),
                                    Point::new(center.x + length, center.y),
                                );
                            }
                            Special::Random | Special::RandomOld => {
                                ctx.draw_str(&bfore, font.clone(), center, "?");
                            }
                            Special::DoubleScore => {
                                ctx.draw_str(&bfore, font.clone(), center, "$");
                            }
                        }
                    }
                }
            }
        }
    }
}

fn difficulty_str(difficulty: Difficulty) -> &'static str {
    match difficulty {
        Difficulty::Simple => "简单",
        Difficulty::Normal => "正常",
        Difficulty::Hard => "困难",
        Difficulty::Compete => "挑战",
    }
}

fn adjust_color(mut c: Color, is_dark: bool) -> Color {
    if !is_dark {
        c.r = (c.r as u16 + 100).min(255) as u8;
        c.g = (c.g as u16 + 100).min(255) as u8;
        c.b = (c.b as u16 + 100).min(255) as u8;
    }
    c
}

fn square_color(t: i32) -> Color {
    let is_dark = ColorTheme::current() == ColorTheme::Dark;
    const POWER: i32 = 4;
    const RANGE: i32 = 256 / POWER;
    let t = t % (RANGE * 5);
    if t < RANGE {
        adjust_color(Color::new((t * POWER) as u8, 0, 255, 255), is_dark)
    } else if t < RANGE * 2 {
        adjust_color(
            Color::new(255, 0, (255 - (t - RANGE) * POWER) as u8, 255),
            is_dark,
        )
    } else if t < RANGE * 3 {
        adjust_color(
            Color::new(255, ((t - RANGE * 2) * POWER) as u8, 0, 255),
            is_dark,
        )
    } else if t < RANGE * 4 {
        adjust_color(
            Color::new(
                (255 - (t - RANGE * 3) * POWER) as u8,
                255,
                ((t - RANGE * 3) * POWER) as u8,
                255,
            ),
            is_dark,
        )
    } else {
        adjust_color(
            Color::new(0, (255 - (t - RANGE * 4) * POWER) as u8, 255, 255),
            is_dark,
        )
    }
}

struct Palette {
    erase: Color,
    back: Color,
    fore_light: Color,
    fore_dark: Color,
    red_sample: Color,
    red_ball: Color,
    green_border: Color,
    blue_circle: Color,
    yellow_circle: Color,
    purple_circle: Color,
}

impl Palette {
    pub fn current() -> &'static Self {
        let is_dark = ColorTheme::current() == ColorTheme::Dark;
        if is_dark {
            &DARK_PALETTE
        } else {
            &LIGHT_PALETTE
        }
    }
}

static DARK_PALETTE: Palette = Palette {
    erase: Color::new(0, 0, 0, 255),
    back: Color::new(31, 31, 31, 255),
    fore_light: Color::new(223, 223, 223, 255),
    fore_dark: Color::new(31, 31, 31, 255),
    red_sample: Color::new(231, 72, 86, 255),
    red_ball: Color::new(197, 15, 31, 255),
    green_border: Color::new(22, 198, 12, 255),
    blue_circle: Color::new(0, 55, 218, 255),
    yellow_circle: Color::new(193, 156, 0, 255),
    purple_circle: Color::new(136, 23, 152, 255),
};

static LIGHT_PALETTE: Palette = Palette {
    erase: Color::new(255, 255, 255, 255),
    back: Color::new(235, 235, 235, 255),
    fore_light: Color::new(255, 255, 255, 255),
    fore_dark: Color::new(31, 31, 31, 255),
    red_sample: Color::new(255, 122, 136, 255),
    red_ball: Color::new(247, 65, 81, 255),
    green_border: Color::new(22, 198, 12, 255),
    blue_circle: Color::new(50, 105, 255, 255),
    yellow_circle: Color::new(213, 176, 20, 255),
    purple_circle: Color::new(186, 73, 202, 255),
};

fn special_color(s: Special) -> Color {
    let palette = Palette::current();
    match s {
        Special::New => palette.blue_circle,
        Special::Delete => palette.red_ball,
        Special::Random => palette.purple_circle,
        Special::RandomOld => palette.red_sample,
        Special::DoubleScore => palette.yellow_circle,
    }
}

async fn init_balls(window: &Window, state: &mut State) -> bool {
    const SIMPLE: u16 = 100;
    const NORMAL: u16 = 101;
    const HARD: u16 = 102;
    const COMPETE: u16 = 103;
    const OPENR: u16 = 104;
    let res = MessageBox::new()
        .title("二维弹球")
        .instruction("请选择难度")
        .message(
            r#"所有难度的区别仅为方块上数目大小的概率分布。
加号增加球的数目；问号随机更改球的速度方向；
减号使当前球消失；美元符号使本轮得分加倍。
按右键暂停。请不要过于依赖示例球。"#,
        )
        .buttons(MessageBoxButton::Cancel)
        .custom_button(CustomButton::new(SIMPLE, "简单"))
        .custom_button(CustomButton::new(NORMAL, "正常"))
        .custom_button(CustomButton::new(HARD, "困难"))
        .custom_button(CustomButton::new(COMPETE, "挑战"))
        .custom_button(CustomButton::new(OPENR, "打开存档"))
        .style(MessageBoxStyle::Info)
        .show(Some(window))
        .await;
    let difficulty = match res {
        MessageBoxResponse::Custom(OPENR) => return show_open(window, state).await,
        MessageBoxResponse::Custom(SIMPLE) => Difficulty::Simple,
        MessageBoxResponse::Custom(NORMAL) => Difficulty::Normal,
        MessageBoxResponse::Custom(HARD) => Difficulty::Hard,
        MessageBoxResponse::Custom(COMPETE) => Difficulty::Compete,
        _ => return false,
    };
    state.map.init(difficulty);
    state.map.reset()
}

async fn show_stop(window: &Window, difficulty: Difficulty, balls_num: usize, score: u64) -> bool {
    const RETRY: u16 = 100;
    let res = MessageBox::new()
        .title("二维弹球")
        .instruction("游戏结束")
        .message(format!(
            "难度：{}\n球数：{}\n分数：{}",
            difficulty_str(difficulty),
            balls_num,
            score,
        ))
        .buttons(MessageBoxButton::Close)
        .custom_button(CustomButton::new(RETRY, "重新开始"))
        .style(MessageBoxStyle::Info)
        .show(Some(window))
        .await;
    res == MessageBoxResponse::Custom(RETRY)
}

enum ShowCloseResult {
    Close,
    Cancel,
    Retry,
}

async fn show_close(window: &Window, state: &mut State) -> ShowCloseResult {
    const RETRY: u16 = 100;
    let res = MessageBox::new()
        .title("二维弹球")
        .message("游戏尚未结束，是否存档？")
        .buttons(MessageBoxButton::Yes | MessageBoxButton::No | MessageBoxButton::Cancel)
        .custom_button(CustomButton::new(RETRY, "重新开始"))
        .style(MessageBoxStyle::Info)
        .show(Some(window))
        .await;
    match res {
        MessageBoxResponse::Yes => {
            if show_save(window, state).await {
                ShowCloseResult::Close
            } else {
                ShowCloseResult::Cancel
            }
        }
        MessageBoxResponse::No => ShowCloseResult::Close,
        MessageBoxResponse::Custom(RETRY) => ShowCloseResult::Retry,
        _ => ShowCloseResult::Cancel,
    }
}

async fn open_record(window: &Window, path: impl AsRef<Path>, state: &mut State) -> bool {
    let file = File::open(path).await.unwrap();
    let (_, buffer) = file.read_to_end_at(vec![], 0).await.unwrap();
    if let Ok((mut map, ticker)) = Map::from_bytes(buffer) {
        map.update_sample(state.com_point(state.mouse));
        state.map = map;
        state.timer_running.set(ticker.is_none());
        state.ticker = ticker;
        true
    } else {
        MessageBox::new()
            .title("二维弹球")
            .message("存档是由本游戏的不同版本创建的，无法打开")
            .buttons(MessageBoxButton::Ok)
            .style(MessageBoxStyle::Error)
            .show(Some(window))
            .await;
        false
    }
}

async fn show_open(window: &Window, state: &mut State) -> bool {
    let filename = FileBox::new()
        .add_filter(("存档文件", "*.balls"))
        .add_filter(("所有文件", "*.*"))
        .open(Some(window))
        .await;
    if let Some(filename) = filename {
        open_record(window, &filename, state).await
    } else {
        false
    }
}

async fn show_save(window: &Window, state: &mut State) -> bool {
    let filename = FileBox::new()
        .add_filter(("存档文件", "*.balls"))
        .save(Some(window))
        .await;
    if let Some(mut filename) = filename {
        match filename.extension() {
            None => {
                filename.set_extension("balls");
            }
            Some(ex) => {
                if ex != "balls" {
                    filename.add_extension("balls");
                }
            }
        }
        let data = state.map.to_vec(state.ticker.as_ref());
        let mut file = File::create(filename).await.unwrap();
        file.write_all_at(data, 0).await.unwrap();
        return true;
    }
    false
}
