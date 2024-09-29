#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]
#![feature(let_chains)]

use std::{
    cell::RefCell,
    rc::{Rc, Weak},
    sync::LazyLock,
};

use balls::{Difficulty, Map, MapTicker};
use compio::runtime::spawn;
use futures_util::FutureExt;
use winio::{Canvas, Color, Point, Rect, Size, SolidColorBrush, Window, block_on};

thread_local! {
    static MAP: RefCell<Map> = RefCell::new(Map::default());
}

fn main() {
    block_on(async {
        let window = Window::new().unwrap();
        window.set_size(Size::new(800.0, 600.0)).unwrap();

        let canvas = Canvas::new(&window).unwrap();

        MAP.with_borrow_mut(|m| m.init(Difficulty::Simple));
        let map_ticker = Rc::new(RefCell::new(None));

        spawn(render(Rc::downgrade(&window), Rc::downgrade(&canvas))).detach();
        spawn(redraw(Rc::downgrade(&canvas), Rc::downgrade(&map_ticker))).detach();
        wait_close(window).await;
    })
}

async fn render(window: Weak<Window>, canvas: Weak<Canvas>) {
    while let Some(window) = window.upgrade()
        && let Some(canvas) = canvas.upgrade()
    {
        let csize = window.client_size().unwrap();
        // Avoid rounding error.
        canvas.set_size(csize + Size::new(1., 1.)).unwrap();
        canvas.set_loc(Point::zero()).unwrap();
        canvas.redraw().unwrap();
        futures_util::select! {
            _ = window.wait_size().fuse() => {}
            _ = window.wait_move().fuse() => {}
        }
    }
}

async fn redraw(canvas: Weak<Canvas>, map_ticker: Weak<RefCell<Option<MapTicker<'static>>>>) {
    const BLACK: Color = Color::new(0, 0, 0, 255);
    const BACK: Color = Color::new(31, 31, 31, 255);

    while let Some(canvas) = canvas.upgrade() {
        let ctx = canvas.wait_redraw().await.unwrap();

        let size = canvas.size().unwrap();
        let brush = SolidColorBrush::new(BLACK);
        ctx.fill_rect(brush, Rect::from_size(size)).unwrap();

        if let Some(map_ticker) = map_ticker.upgrade() {
            MAP.with_borrow(|map| {
                let columns = map.column_len() as f64;
                let rows = map.row_len() as f64;
                let sidew = size.width / columns;
                let sideh = size.height / rows;
                let sidel = sidew.min(sideh);
                let dw = sidel * columns - 1.0;
                let dh = sidel * rows - 1.0;
                let dx = (size.width - dw) / 2.0;
                let dy = (size.height - dh) / 2.0;
                let brush = SolidColorBrush::new(BACK);
                ctx.fill_rect(brush, Rect::new(Point::new(dx, dy), Size::new(dw, dh)))
                    .unwrap();
            });
        }
    }
}

async fn wait_close(window: Rc<Window>) {
    window.wait_close().await;
}
