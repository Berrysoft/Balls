#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
#![feature(let_chains)]

use std::{
    cell::RefCell,
    path::Path,
    rc::{Rc, Weak},
    time::Duration,
};

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
use futures_channel::oneshot;
use futures_util::{FutureExt, lock::Mutex};
use winio::{
    BrushPen, Canvas, Color, CustomButton, DrawingFontBuilder, FileBox, HAlign, MessageBox,
    MessageBoxButton, MessageBoxResponse, MessageBoxStyle, MouseButton, Point, Rect, Size,
    SolidColorBrush, VAlign, Window, block_on,
};

#[derive(Debug)]
struct State {
    map: Rc<RefCell<Map>>,
    ticker: Option<MapTicker>,
    drect: Rect,
    mouse: Point,
    timer_running: bool,
    close_tx: Option<oneshot::Sender<()>>,
}

impl State {
    pub fn new(close_tx: oneshot::Sender<()>) -> Self {
        Self {
            map: Rc::new(RefCell::new(Map::default())),
            ticker: None,
            drect: Rect::zero(),
            mouse: Point::zero(),
            timer_running: true,
            close_tx: Some(close_tx),
        }
    }

    pub fn send_close(&mut self) {
        if let Some(tx) = self.close_tx.take() {
            tx.send(()).ok();
        }
    }
}

impl State {
    pub fn set_drect(&mut self, canvas: &Canvas) {
        let size = canvas.size().unwrap();
        let map = self.map.borrow();
        let columns = map.column_len() as f64;
        let rows = map.row_len() as f64;
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

fn main() {
    block_on(async {
        let window = Window::new().unwrap();
        window.set_size(Size::new(450.0, 600.0)).unwrap();

        #[cfg(windows)]
        window.set_icon_by_id(1).unwrap();

        let (close_tx, close_rx) = oneshot::channel();
        let state = Rc::new(Mutex::new(State::new(close_tx)));

        {
            let mut state = state.lock().await;

            if let Some(path) = std::env::args_os().nth(1)
                && compio::fs::metadata(&path).await.is_ok()
            {
                if !open_record(&window, &path, &mut state).await {
                    return;
                }
            } else if !init_balls(&window, &mut state).await {
                return;
            }
        }

        let canvas = Canvas::new(&window).unwrap();

        spawn(render(
            Rc::downgrade(&window),
            Rc::downgrade(&canvas),
            Rc::downgrade(&state),
        ))
        .detach();
        spawn(tick(
            Rc::downgrade(&window),
            Rc::downgrade(&canvas),
            Rc::downgrade(&state),
        ))
        .detach();
        spawn(redraw(Rc::downgrade(&canvas), Rc::downgrade(&state))).detach();
        wait_close(window, Rc::downgrade(&state), close_rx).await;
    })
}

fn difficulty_str(difficulty: Difficulty) -> &'static str {
    match difficulty {
        Difficulty::Simple => "简单",
        Difficulty::Normal => "正常",
        Difficulty::Hard => "困难",
        Difficulty::Compete => "挑战",
    }
}

async fn render(window: Weak<Window>, canvas: Weak<Canvas>, state: Weak<Mutex<State>>) {
    while let Some(window) = window.upgrade()
        && let Some(canvas) = canvas.upgrade()
    {
        let csize = window.client_size().unwrap();
        // Avoid rounding error.
        canvas.set_size(csize).unwrap();
        canvas.set_loc(Point::zero()).unwrap();
        if let Some(state) = state.upgrade() {
            let mut state = state.lock().await;
            state.set_drect(&canvas);
        }
        canvas.redraw().unwrap();
        futures_util::select! {
            _ = window.wait_size().fuse() => {}
            _ = window.wait_move().fuse() => {}
            p = canvas.wait_mouse_move().fuse() => {
                if let Some(state) = state.upgrade() {
                    let mut state = state.lock().await;
                    let p = p.unwrap();
                    state.mouse = p;
                    if state.ticker.is_none() {
                        let mut map = state.map.borrow_mut();
                        map.update_sample(state.com_point(p));
                    }
                }
            }
            b = canvas.wait_mouse_up().fuse() => {
                if let Some(state) = state.upgrade() {
                    let mut state = state.lock().await;
                    match b {
                        MouseButton::Left => {
                            if state.ticker.is_none() {
                                state.ticker = Some(Map::start(state.map.clone(), state.com_point(state.mouse)));
                            }
                        }
                        MouseButton::Right => {
                            if state.ticker.is_some() {
                                state.timer_running = !state.timer_running;
                            }
                        }
                        _ => {}
                    }
                }
            }
        }
    }
}

fn square_color(t: i32) -> Color {
    const POWER: i32 = 4;
    const RANGE: i32 = 256 / POWER;
    let t = t % (RANGE * 5);
    if t < RANGE {
        Color::new((t * POWER) as u8, 0, 255, 255)
    } else if t < RANGE * 2 {
        Color::new(255, 0, (255 - (t - RANGE) * POWER) as u8, 255)
    } else if t < RANGE * 3 {
        Color::new(255, ((t - RANGE * 2) * POWER) as u8, 0, 255)
    } else if t < RANGE * 4 {
        Color::new(
            (255 - (t - RANGE * 3) * POWER) as u8,
            255,
            ((t - RANGE * 3) * POWER) as u8,
            255,
        )
    } else {
        Color::new(0, (255 - (t - RANGE * 4) * POWER) as u8, 255, 255)
    }
}

const BLACK: Color = Color::new(0, 0, 0, 255);
const BACK: Color = Color::new(31, 31, 31, 255);
const FORE: Color = Color::new(223, 223, 223, 255);
const RED_SAMPLE: Color = Color::new(231, 72, 86, 255);
const RED_BALL: Color = Color::new(197, 15, 31, 255);
const GREEN_BORDER: Color = Color::new(22, 198, 12, 255);
const BLUE_CIRCLE: Color = Color::new(0, 55, 218, 255);
const YELLOW_CIRCLE: Color = Color::new(193, 156, 0, 255);
const PURPLE_CIRCLE: Color = Color::new(136, 23, 152, 255);

fn special_color(s: Special) -> Color {
    match s {
        Special::New => BLUE_CIRCLE,
        Special::Delete => RED_BALL,
        Special::Random => PURPLE_CIRCLE,
        Special::RandomOld => RED_SAMPLE,
        Special::DoubleScore => YELLOW_CIRCLE,
    }
}

async fn redraw(canvas: Weak<Canvas>, state: Weak<Mutex<State>>) {
    while let Some(canvas) = canvas.upgrade() {
        let ctx = canvas.wait_redraw().await.unwrap();

        let size = canvas.size().unwrap();
        let brush = SolidColorBrush::new(BLACK);
        ctx.fill_rect(brush, Rect::from_size(size)).unwrap();

        if let Some(state) = state.upgrade() {
            let state = state.lock().await;
            let drect = state.drect;
            let brush = SolidColorBrush::new(BACK);
            ctx.fill_rect(brush, drect).unwrap();

            let extend = drect.width() / CLIENT_WIDTH;
            let font = DrawingFontBuilder::new()
                .family("Arial")
                .size(NUM_SIZE * extend)
                .halign(HAlign::Center)
                .valign(VAlign::Center)
                .build();
            let map = state.map.borrow();
            if state.ticker.is_none() {
                let sample_pos = map.sample();
                if sample_pos.x >= RADIUS
                    && sample_pos.x <= CLIENT_WIDTH - RADIUS
                    && sample_pos.y >= RADIUS
                    && sample_pos.y <= CLIENT_HEIGHT - RADIUS
                {
                    let brush = SolidColorBrush::new(RED_SAMPLE);
                    ctx.fill_ellipse(brush, state.ball_rect(sample_pos))
                        .unwrap();
                }
            }
            let bball = if map.doubled_score() {
                SolidColorBrush::new(YELLOW_CIRCLE)
            } else {
                SolidColorBrush::new(RED_BALL)
            };
            let start_pos = map.startp();
            let end_shooting = state
                .ticker
                .as_ref()
                .map(|ticker| ticker.is_end())
                .unwrap_or_default();
            if !end_shooting {
                ctx.fill_ellipse(&bball, state.ball_rect(start_pos))
                    .unwrap();
            }
            if let Some(ticker) = state.ticker.as_ref() {
                if let Some(new_start) = ticker.new_startp() {
                    ctx.fill_ellipse(&bball, state.ball_rect(new_start))
                        .unwrap();
                }
                for b in ticker.balls() {
                    ctx.fill_ellipse(&bball, state.ball_rect(b.pos)).unwrap();
                }
            }
            let pborder = BrushPen::new(SolidColorBrush::new(GREEN_BORDER), 3.0);
            for (y, row) in map.balls().iter().enumerate() {
                for (x, t) in row.iter().enumerate() {
                    let cx = x as f64 * SIDE + SIDE / 2.0;
                    let cy = y as f64 * SIDE + SIDE / 2.0;
                    let real_c = Point::new(cx, cy) * extend;
                    let center = real_c + state.drect.origin.to_vector();
                    match t {
                        BallType::None => {}
                        BallType::Normal(score) => {
                            let fillc = square_color(*score);
                            let btext =
                                SolidColorBrush::new(if fillc.g > 127 { BACK } else { FORE });
                            let bfill = SolidColorBrush::new(fillc);
                            let mut round_rect = Rect::new(
                                Point::new(x as f64 * SIDE + 5.0, y as f64 * SIDE + 5.0),
                                Size::new(SIDE - 11.0, SIDE - 11.0),
                            );
                            round_rect *= extend;
                            round_rect.origin += drect.origin.to_vector();
                            let radius = Size::new(10.0, 10.0) * extend;
                            ctx.fill_round_rect(bfill, round_rect, radius).unwrap();
                            ctx.draw_round_rect(&pborder, round_rect, radius).unwrap();
                            ctx.draw_str(btext, font.clone(), center, score.to_string())
                                .unwrap();
                        }
                        BallType::Special(special) => {
                            let bcircle = SolidColorBrush::new(special_color(*special));
                            let r = NUM_SIZE * extend / 2.0;
                            let circle_rect = Rect::new(
                                Point::new(real_c.x - r, real_c.y - r) + drect.origin.to_vector(),
                                Size::new(2.0 * r, 2.0 * r),
                            );
                            ctx.fill_ellipse(bcircle, circle_rect).unwrap();
                            ctx.draw_ellipse(&pborder, circle_rect).unwrap();
                            let bfore = SolidColorBrush::new(FORE);
                            let pfore = BrushPen::new(&bfore, 3.0);
                            match special {
                                Special::New => {
                                    let length = (NUM_SIZE - 20.0) * extend / 2.0;
                                    ctx.draw_line(
                                        &pfore,
                                        Point::new(center.x, center.y - length),
                                        Point::new(center.x, center.y + length),
                                    )
                                    .unwrap();
                                    ctx.draw_line(
                                        &pfore,
                                        Point::new(center.x - length, center.y),
                                        Point::new(center.x + length, center.y),
                                    )
                                    .unwrap();
                                }
                                Special::Delete => {
                                    let length = (NUM_SIZE - 20.0) * extend / 2.0;
                                    ctx.draw_line(
                                        &pfore,
                                        Point::new(center.x - length, center.y),
                                        Point::new(center.x + length, center.y),
                                    )
                                    .unwrap();
                                }
                                Special::Random | Special::RandomOld => {
                                    ctx.draw_str(&bfore, font.clone(), center, "?").unwrap();
                                }
                                Special::DoubleScore => {
                                    ctx.draw_str(&bfore, font.clone(), center, "$").unwrap();
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#[allow(clippy::await_holding_refcell_ref)] // false positive
async fn tick(window: Weak<Window>, canvas: Weak<Canvas>, state: Weak<Mutex<State>>) {
    let mut interval = interval(Duration::from_millis(10));
    while let Some(window) = window.upgrade()
        && let Some(state) = state.upgrade()
        && let Some(canvas) = canvas.upgrade()
    {
        interval.tick().await;

        let mut state = state.lock().await;
        if state.timer_running {
            if let Some(mut ticker) = state.ticker.take() {
                if ticker.tick() {
                    state.ticker = Some(ticker);
                } else {
                    drop(ticker);
                    let mut map = state.map.borrow_mut();
                    if map.reset() {
                        map.update_sample(state.com_point(state.mouse));
                    } else {
                        let difficulty = map.difficulty();
                        let balls_num = map.balls_num();
                        let score = map.score();
                        drop(map); // avoid to hold it across await
                        let cont = show_stop(&window, difficulty, balls_num, score).await;
                        if !cont {
                            state.send_close();
                        }
                        if !init_balls(&window, &mut state).await {
                            state.send_close();
                        }
                    }
                }
            }

            let map = state.map.borrow();
            let difficulty = map.difficulty();
            let title = if let Some(ticker) = state.ticker.as_ref() {
                format!(
                    "二维弹球 - {} 球数：{} 剩余球数：{} 分数：{}",
                    difficulty_str(difficulty),
                    map.balls_num(),
                    ticker.remain(),
                    map.score()
                )
            } else {
                format!(
                    "二维弹球 - {} 球数：{} 分数：{}",
                    difficulty_str(difficulty),
                    map.balls_num(),
                    map.score()
                )
            };
            window.set_text(title).unwrap();
        }

        canvas.redraw().unwrap();
    }
}

async fn wait_close(
    window: Rc<Window>,
    state: Weak<Mutex<State>>,
    mut close_rx: oneshot::Receiver<()>,
) {
    loop {
        futures_util::select! {
            _ = window.wait_close().fuse() => {},
            _ = close_rx => {
                break;
            }
        }

        let running = if let Some(state) = state.upgrade() {
            let mut state = state.lock().await;
            std::mem::replace(&mut state.timer_running, false)
        } else {
            false
        };

        if !show_close(&window, state.clone()).await {
            break;
        }

        if let Some(state) = state.upgrade() {
            state.lock().await.timer_running = running;
        }
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
        .custom_button(CustomButton::new(SIMPLE, "简单"))
        .custom_button(CustomButton::new(NORMAL, "正常"))
        .custom_button(CustomButton::new(HARD, "困难"))
        .custom_button(CustomButton::new(COMPETE, "挑战"))
        .custom_button(CustomButton::new(OPENR, "打开存档"))
        .style(MessageBoxStyle::Info)
        .show(Some(window))
        .await
        .unwrap();
    let difficulty = match res {
        MessageBoxResponse::Custom(OPENR) => return show_open(window, state).await,
        MessageBoxResponse::Custom(SIMPLE) => Difficulty::Simple,
        MessageBoxResponse::Custom(NORMAL) => Difficulty::Normal,
        MessageBoxResponse::Custom(HARD) => Difficulty::Hard,
        MessageBoxResponse::Custom(COMPETE) => Difficulty::Compete,
        _ => return false,
    };
    let mut map = state.map.borrow_mut();
    map.init(difficulty);
    map.reset()
}

async fn show_stop(window: &Window, difficulty: Difficulty, balls_num: usize, score: u64) -> bool {
    const YES: u16 = 100;
    const NO: u16 = 101;
    let res = MessageBox::new()
        .title("二维弹球")
        .instruction("游戏结束")
        .message(format!(
            "难度：{}\n球数：{}\n分数：{}",
            difficulty_str(difficulty),
            balls_num,
            score,
        ))
        .custom_button(CustomButton::new(YES, "重新开始"))
        .custom_button(CustomButton::new(NO, "关闭"))
        .style(MessageBoxStyle::Info)
        .show(Some(window))
        .await
        .unwrap();
    res == MessageBoxResponse::Custom(YES)
}

async fn show_close(window: &Window, state: Weak<Mutex<State>>) -> bool {
    let res = MessageBox::new()
        .title("二维弹球")
        .message("游戏尚未结束，是否存档？")
        .buttons(MessageBoxButton::Yes | MessageBoxButton::No | MessageBoxButton::Cancel)
        .style(MessageBoxStyle::Info)
        .show(Some(window))
        .await
        .unwrap();
    match res {
        MessageBoxResponse::Yes => !show_save(window, state).await,
        MessageBoxResponse::Cancel => true,
        _ => false,
    }
}

async fn open_record(window: &Window, path: impl AsRef<Path>, state: &mut State) -> bool {
    let file = File::open(path).await.unwrap();
    let (_, buffer) = file.read_to_end_at(vec![], 0).await.unwrap();
    if let Ok((map, ticker)) = Map::from_bytes(&buffer) {
        map.borrow_mut().update_sample(state.com_point(state.mouse));
        state.map = map;
        state.timer_running = ticker.is_none();
        state.ticker = ticker;
        true
    } else {
        MessageBox::new()
            .title("二维弹球")
            .message("存档是由本游戏的不同版本创建的，无法打开")
            .buttons(MessageBoxButton::Ok)
            .style(MessageBoxStyle::Error)
            .show(Some(window))
            .await
            .unwrap();
        false
    }
}

async fn show_open(window: &Window, state: &mut State) -> bool {
    let filename = FileBox::new()
        .add_filter(("存档文件", "*.balls"))
        .add_filter(("所有文件", "*.*"))
        .open(Some(window))
        .await
        .unwrap();
    if let Some(filename) = filename {
        open_record(window, &filename, state).await
    } else {
        false
    }
}

async fn show_save(window: &Window, state: Weak<Mutex<State>>) -> bool {
    let filename = FileBox::new()
        .add_filter(("存档文件", "*.balls"))
        .save(Some(window))
        .await
        .unwrap();
    if let Some(filename) = filename {
        if let Some(state) = state.upgrade() {
            let data = {
                let state = state.lock().await;
                let map = state.map.borrow();
                map.to_vec(state.ticker.as_ref())
            };
            let mut file = File::create(filename).await.unwrap();
            file.write_all_at(data, 0).await.unwrap();
            return true;
        }
    }
    false
}
