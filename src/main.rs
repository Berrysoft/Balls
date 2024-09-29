#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
#![feature(let_chains)]

use std::{
    cell::RefCell,
    rc::{Rc, Weak},
};

use balls::{BallType, CLIENT_WIDTH, Difficulty, Map, MapTicker, NUM_SIZE, RADIUS, SIDE, Special};
use compio::runtime::spawn;
use futures_util::FutureExt;
use winio::{
    BrushPen, Canvas, Color, DrawingFontBuilder, HAlign, Point, Rect, Size, SolidColorBrush,
    VAlign, Window, block_on,
};

thread_local! {
    static MAP: RefCell<Map> = RefCell::new(Map::default());
}

#[derive(Debug, Default)]
struct State {
    ticker: Option<MapTicker<'static>>,
    drect: Rect,
}

impl State {
    pub fn set_drect(&mut self, canvas: &Canvas) {
        let size = canvas.size().unwrap();
        let drect = MAP.with_borrow(|map| {
            let columns = map.column_len() as f64;
            let rows = map.row_len() as f64;
            let sidew = size.width / columns;
            let sideh = size.height / rows;
            let sidel = sidew.min(sideh);
            let dw = sidel * columns - 1.0;
            let dh = sidel * rows - 1.0;
            let dx = (size.width - dw) / 2.0;
            let dy = (size.height - dh) / 2.0;
            Rect::new(Point::new(dx, dy), Size::new(dw, dh))
        });
        self.drect = drect;
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
        window.set_size(Size::new(800.0, 600.0)).unwrap();

        let canvas = Canvas::new(&window).unwrap();

        let state = Rc::new(RefCell::new(State::default()));

        MAP.with_borrow_mut(|m| {
            m.init(Difficulty::Simple);
            m.reset();
        });

        spawn(render(
            Rc::downgrade(&window),
            Rc::downgrade(&canvas),
            Rc::downgrade(&state),
        ))
        .detach();
        spawn(redraw(Rc::downgrade(&canvas), Rc::downgrade(&state))).detach();
        wait_close(window).await;
    })
}

async fn render(window: Weak<Window>, canvas: Weak<Canvas>, state: Weak<RefCell<State>>) {
    while let Some(window) = window.upgrade()
        && let Some(canvas) = canvas.upgrade()
    {
        let csize = window.client_size().unwrap();
        // Avoid rounding error.
        canvas.set_size(csize + Size::new(1., 1.)).unwrap();
        canvas.set_loc(Point::zero()).unwrap();
        if let Some(state) = state.upgrade() {
            state.borrow_mut().set_drect(&canvas);
        }
        canvas.redraw().unwrap();
        futures_util::select! {
            _ = window.wait_size().fuse() => {}
            _ = window.wait_move().fuse() => {}
            p = canvas.wait_mouse_move().fuse() => {
                if let Some(state) = state.upgrade() {
                    let state = state.borrow();
                    if state.ticker.is_none() {
                        let p = p.unwrap();
                        MAP.with_borrow_mut(|map| map.update_sample(state.com_point(p)));
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

async fn redraw(canvas: Weak<Canvas>, state: Weak<RefCell<State>>) {
    while let Some(canvas) = canvas.upgrade() {
        let ctx = canvas.wait_redraw().await.unwrap();

        let size = canvas.size().unwrap();
        let brush = SolidColorBrush::new(BLACK);
        ctx.fill_rect(brush, Rect::from_size(size)).unwrap();

        if let Some(state) = state.upgrade() {
            let state = state.borrow();
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
            MAP.with_borrow(|map| {
                if state.ticker.is_none() {
                    let sample_pos = map.sample();
                    let brush = SolidColorBrush::new(RED_SAMPLE);
                    ctx.fill_ellipse(brush, state.ball_rect(sample_pos))
                        .unwrap();
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
                                let fillc = square_color(score.0);
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
                                ctx.draw_str(btext, font.clone(), center, score.0.to_string())
                                    .unwrap();
                            }
                            BallType::Special(special) => {
                                let bcircle = SolidColorBrush::new(special_color(*special));
                                let r = NUM_SIZE * extend / 2.0;
                                let circle_rect = Rect::new(
                                    Point::new(real_c.x - r, real_c.y - r)
                                        + drect.origin.to_vector(),
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
            });
        }
    }
}

async fn wait_close(window: Rc<Window>) {
    window.wait_close().await;
}
