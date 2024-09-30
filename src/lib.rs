use std::{
    cell::RefCell,
    f64::consts::PI,
    io::{Cursor, Read},
    rc::Rc,
};

use bitflags::bitflags;
use euclid::Angle;
use rand::rngs::ThreadRng;
use rand_distr::{
    Distribution, Normal,
    uniform::{UniformFloat, UniformInt, UniformSampler},
};
use winio::{Point, Vector};

pub const COLUMNS: usize = 6;
pub const ROWS: usize = 8;
pub const RADIUS: f64 = 20.0;
pub const SIDE: f64 = 200.0;
pub const CLIENT_WIDTH: f64 = SIDE * COLUMNS as f64;
pub const CLIENT_HEIGHT: f64 = SIDE * ROWS as f64;
pub const SPEED: f64 = 50.0;
pub const NUM_SIZE: f64 = 100.0;
pub const RECORD_VERSION: i32 = 2;

#[repr(i32)]
#[derive(Debug, Default, PartialEq, Eq, Clone, Copy)]
pub enum Difficulty {
    #[default]
    Simple,
    Normal,
    Hard,
    Compete,
}

impl Difficulty {
    pub fn from_i32(v: i32) -> Option<Self> {
        match v {
            0 => Some(Self::Simple),
            1 => Some(Self::Normal),
            2 => Some(Self::Hard),
            3 => Some(Self::Compete),
            _ => None,
        }
    }
}

#[derive(Debug, PartialEq, Eq, Clone, Copy)]
pub enum Special {
    New,
    Delete,
    Random,
    RandomOld,
    DoubleScore,
}

#[derive(Debug, Default, PartialEq, Eq, Clone, Copy)]
pub enum BallType {
    #[default]
    None,
    Normal(i32),
    Special(Special),
}

impl BallType {
    pub fn is_none(&self) -> bool {
        matches!(self, Self::None)
    }

    pub fn is_normal(&self) -> bool {
        matches!(self, Self::Normal(_))
    }

    pub fn is_special(&self) -> bool {
        !(self.is_none() || self.is_normal())
    }

    pub fn to_i32(&self) -> i32 {
        match self {
            Self::None => 0,
            Self::Normal(s) => *s,
            Self::Special(s) => match s {
                Special::New => -1,
                Special::Delete => -2,
                Special::Random => -3,
                Special::RandomOld => -4,
                Special::DoubleScore => -5,
            },
        }
    }

    pub fn from_i32(v: i32) -> Option<Self> {
        match v {
            1.. => Some(Self::Normal(v)),
            0 => Some(Self::None),
            -1 => Some(Self::Special(Special::New)),
            -2 => Some(Self::Special(Special::Delete)),
            -3 => Some(Self::Special(Special::Random)),
            -4 => Some(Self::Special(Special::RandomOld)),
            -5 => Some(Self::Special(Special::DoubleScore)),
            _ => None,
        }
    }
}

#[derive(Debug, PartialEq)]
pub struct Ball {
    pub pos: Point,
    pub speed: Vector,
}

impl Ball {
    pub fn new(pos: Point, speed: Vector) -> Self {
        Self { pos, speed }
    }
}

#[derive(Debug, Default)]
pub struct Map {
    map: [[BallType; COLUMNS]; ROWS],
    difficulty: Difficulty,
    rng: ThreadRng,
    score: u64,
    start: f64,
    startv: Vector,
    sample: Point,
    balls_num: usize,
    doubled_score: bool,
}

bitflags! {
    #[derive(Debug, PartialEq, Eq)]
    struct BounceSide: u8 {
        const LEFT = 0x1;
        const TOP = 0x2;
        const RIGHT = 0x4;
        const BOTTOM = 0x8;

        const LT = Self::LEFT.bits() | Self::TOP.bits();
        const RT = Self::RIGHT.bits() | Self::TOP.bits();
        const LB = Self::LEFT.bits() | Self::BOTTOM.bits();
        const RB = Self::RIGHT.bits() | Self::BOTTOM.bits();

        const LT_A = 0x10;
        const RT_A = 0x20;
        const LB_A = 0x40;
        const RB_A = 0x80;
    }
}

impl Map {
    pub fn init(&mut self, difficulty: Difficulty) {
        self.difficulty = difficulty;
        self.balls_num = 1;
        self.start = CLIENT_WIDTH / 2.0;
        self.map = Default::default();
        self.score = 0;
    }

    pub fn balls(&self) -> &[[BallType; COLUMNS]; ROWS] {
        &self.map
    }

    pub fn difficulty(&self) -> Difficulty {
        self.difficulty
    }

    pub fn balls_num(&self) -> usize {
        self.balls_num
    }

    pub fn score(&self) -> u64 {
        self.score
    }

    pub const fn column_len(&self) -> usize {
        COLUMNS
    }

    pub const fn row_len(&self) -> usize {
        ROWS
    }

    pub fn sample(&self) -> Point {
        self.sample
    }

    pub fn doubled_score(&self) -> bool {
        self.doubled_score
    }

    #[inline]
    pub fn startp(&self) -> Point {
        Point::new(self.start, CLIENT_HEIGHT - RADIUS)
    }

    #[inline]
    fn get_map(&self, r: usize, c: usize) -> Option<&BallType> {
        self.map.get(r).and_then(|arr| arr.get(c))
    }

    #[inline]
    fn is_normal(&self, r: usize, c: usize) -> Option<bool> {
        self.get_map(r, c).map(|b| b.is_normal())
    }

    #[inline]
    fn is_side(&self, r: usize, c: usize) -> bool {
        self.is_normal(r, c).unwrap_or(true)
    }

    #[inline]
    fn is_pluge(&self, r: usize, c: usize) -> bool {
        self.is_normal(r, c).unwrap_or(false)
    }

    fn bounce_side(&self, c: usize, r: usize) -> BounceSide {
        let mut result = BounceSide::empty();
        if self.is_side(r, c.wrapping_sub(1)) {
            result |= BounceSide::LEFT;
        }
        if self.is_side(r.wrapping_sub(1), c) {
            result |= BounceSide::TOP;
        }
        if self.is_side(r, c + 1) {
            result |= BounceSide::RIGHT;
        }
        if self.is_side(r + 1, c) {
            result |= BounceSide::BOTTOM;
        }
        if self.is_pluge(r.wrapping_sub(1), c.wrapping_sub(1)) && !result.contains(BounceSide::LT) {
            result |= BounceSide::LT_A;
        }
        if self.is_pluge(r.wrapping_sub(1), c + 1) && !result.contains(BounceSide::RT) {
            result |= BounceSide::RT_A;
        }
        if self.is_pluge(r + 1, c.wrapping_sub(1)) && !result.contains(BounceSide::LB) {
            result |= BounceSide::LB_A;
        }
        if self.is_pluge(r + 1, c + 1) && !result.contains(BounceSide::RB) {
            result |= BounceSide::RB_A;
        }
        result
    }

    fn hit(&mut self, r: usize, c: usize) {
        if let Some(b) = self.map.get_mut(r).and_then(|arr| arr.get_mut(c)) {
            match b {
                BallType::Normal(s) => {
                    if *s == 1 {
                        *b = BallType::None;
                    } else {
                        *s -= 1;
                    }
                }
                _ => unreachable!(),
            }
            self.score += 1;
        }
    }

    fn bounce(&mut self, b: &mut Ball) -> bool {
        let c = (b.pos.x.round() / SIDE).floor();
        let r = (b.pos.y.round() / SIDE).floor();
        let ls = c * SIDE;
        let rs = (c + 1.0) * SIDE;
        let ts = r * SIDE;
        let bs = (r + 1.0) * SIDE;

        let c = clip(c as usize, 0, COLUMNS - 1);
        let r = clip(r as usize, 0, ROWS - 1);

        let bside = self.bounce_side(c, r);

        let current = &mut self.map[r][c];
        match current {
            BallType::None | BallType::Normal(_) => {}
            BallType::Special(special) => {
                let center = Point::new(ls + SIDE / 2.0, ts + SIDE / 2.0);
                let dis = (b.pos - center).length();
                if dis <= NUM_SIZE / 2.0 + RADIUS + 10.0 {
                    match special {
                        Special::New => {
                            self.balls_num += 1;
                            *current = BallType::None;
                        }
                        Special::Delete => {
                            *current = BallType::None;
                            return true;
                        }
                        Special::Random | Special::RandomOld => {
                            let thetad = Normal::new(
                                if b.speed.angle_from_x_axis().radians + b.speed.x < 0.0 {
                                    0.0
                                } else {
                                    PI
                                },
                                PI / 2.0,
                            )
                            .unwrap();
                            let theta = thetad.sample(&mut self.rng);
                            b.speed = Vector::from_angle_and_length(Angle::radians(theta), SPEED);
                            *current = BallType::Special(Special::RandomOld);
                        }
                        Special::DoubleScore => {
                            self.doubled_score = true;
                            *current = BallType::None;
                        }
                    }
                }
            }
        }

        b.pos += b.speed;
        let nbside = get_side(b.pos, ls, ts, rs, bs);

        if bside.contains(BounceSide::LEFT) && nbside.contains(BounceSide::LEFT) {
            change_ball(&mut b.speed.x, &mut b.pos.x, ls, true);
            self.hit(r, c.wrapping_sub(1));
        } else if bside.contains(BounceSide::RIGHT) && nbside.contains(BounceSide::RIGHT) {
            change_ball(&mut b.speed.x, &mut b.pos.x, rs, false);
            self.hit(r, c + 1);
        } else if bside.contains(BounceSide::TOP) && nbside.contains(BounceSide::TOP) {
            change_ball(&mut b.speed.y, &mut b.pos.y, ts, true);
            self.hit(r.wrapping_sub(1), c);
        } else if bside.contains(BounceSide::BOTTOM)
            && b.pos.y + RADIUS >= CLIENT_HEIGHT
            && b.speed.y > 0.0
        {
            return true;
        } else if bside.contains(BounceSide::BOTTOM) && nbside.contains(BounceSide::BOTTOM) {
            change_ball(&mut b.speed.y, &mut b.pos.y, bs, false);
            self.hit(r + 1, c);
        }

        if bside.contains(BounceSide::LT_A) && nbside.contains(BounceSide::LT) {
            let sc = Point::new(ls + RADIUS, ts + RADIUS);
            change_ball_arc(b, &sc, false);
            self.hit(r.wrapping_sub(1), c.wrapping_sub(1));
        } else if bside.contains(BounceSide::RT_A) && nbside.contains(BounceSide::RT) {
            let sc = Point::new(rs - RADIUS, ts + RADIUS);
            change_ball_arc(b, &sc, true);
            self.hit(r.wrapping_sub(1), c + 1);
        } else if bside.contains(BounceSide::LB_A) && nbside.contains(BounceSide::LB) {
            let sc = Point::new(ls + RADIUS, bs - RADIUS);
            change_ball_arc(b, &sc, true);
            self.hit(r + 1, c.wrapping_sub(1));
        } else if bside.contains(BounceSide::RB_A) && nbside.contains(BounceSide::RB) {
            let sc = Point::new(rs - RADIUS, bs - RADIUS);
            change_ball_arc(b, &sc, false);
            self.hit(r + 1, c + 1);
        }

        if b.speed.y.abs() < 1.0 {
            b.speed.y = if b.speed.y > 0.0 { 1.0 } else { -1.0 };
        }
        false
    }

    pub fn is_over(&self) -> bool {
        !self.map[ROWS - 1].iter().all(|b| b.is_none())
    }

    fn get_start(&self, p: Point, speed: f64) -> Vector {
        let pp = p - self.startp();
        let mut result = pp.normalize() * speed;
        if result.y > -speed / SPEED {
            result.y = -speed / SPEED;
        }
        result
    }

    pub fn reset(&mut self) -> bool {
        for r in (1..ROWS).rev() {
            for c in 0..COLUMNS {
                let t = &self.map[r - 1][c];
                match t {
                    BallType::Special(Special::RandomOld) => {
                        self.map[r][c] = BallType::None;
                    }
                    _ => self.map[r][c] = *t,
                }
            }
        }

        if self.is_over() {
            return false;
        }

        let fbnum = self.balls_num as f64;
        let distr = match self.difficulty {
            Difficulty::Simple => Normal::new(fbnum / 2.0, fbnum / 6.0).unwrap(),
            Difficulty::Normal => Normal::new(fbnum, fbnum / 3.0).unwrap(),
            Difficulty::Hard => Normal::new(fbnum * 1.5, fbnum / 2.0).unwrap(),
            Difficulty::Compete => Normal::new(fbnum.powf(1.1), fbnum / 3.0).unwrap(),
        };
        let uni: UniformFloat<f64> = UniformFloat::new(0.0, 1.0);
        for c in 0..COLUMNS {
            if uni.sample(&mut self.rng) < 0.6 {
                let v = distr.sample(&mut self.rng).round() as i32;
                if v > 0 {
                    self.map[0][c] = BallType::Normal(v);
                    continue;
                }
            }
            self.map[0][c] = BallType::None;
        }
        let idist: UniformInt<usize> = UniformInt::new(0, COLUMNS);
        if uni.sample(&mut self.rng) < 0.5 {
            let c = idist.sample(&mut self.rng);
            self.map[0][c] = BallType::Special(Special::Random);
        }
        if uni.sample(&mut self.rng) < 0.2 {
            let c = idist.sample(&mut self.rng);
            self.map[0][c] = BallType::Special(Special::Delete);
        }
        if uni.sample(&mut self.rng) < 0.2 {
            let c = idist.sample(&mut self.rng);
            self.map[0][c] = BallType::Special(Special::DoubleScore);
        }
        if self.balls_num < (i32::MAX / 2) as usize
            && (self.difficulty == Difficulty::Compete || uni.sample(&mut self.rng) < 0.5)
        {
            let c = idist.sample(&mut self.rng);
            self.map[0][c] = BallType::Special(Special::New);
        }
        true
    }

    pub fn start(this: Rc<RefCell<Self>>, p: Point) -> MapTicker {
        {
            let mut map = this.borrow_mut();
            map.startv = map.get_start(p, SPEED);
        }
        MapTicker::new(this)
    }

    pub fn update_sample(&mut self, p: Point) {
        let v = self.get_start(p, 0.5);
        let mut tp = self.startp();
        let mut c1;
        let mut c2;
        let mut r;
        loop {
            tp += v;
            c1 = ((tp.x - RADIUS).round() / SIDE) as usize;
            c2 = ((tp.x + RADIUS).round() / SIDE) as usize;
            r = ((tp.y - RADIUS).round() / SIDE) as usize;
            if !(tp.x >= RADIUS
                && tp.x <= CLIENT_WIDTH - RADIUS
                && tp.y >= RADIUS
                && self
                    .get_map(r, c1)
                    .map(|b| !b.is_normal())
                    .unwrap_or_default()
                && self
                    .get_map(r, c2)
                    .map(|b| !b.is_normal())
                    .unwrap_or_default())
            {
                break;
            }
        }
        self.sample = tp - v;
    }

    pub fn to_vec(&self, ticker: Option<&MapTicker>) -> Vec<u8> {
        let buffer = vec![];
        let mut writer = BufferWriter(buffer);

        writer.write_i32_le(RECORD_VERSION);
        writer.write_i32_le(self.balls_num as _);
        writer.write_f64_le(self.start);
        writer.write_f64_le(CLIENT_HEIGHT - RADIUS);
        let end_pos = ticker
            .and_then(|ticker| ticker.new_start)
            .unwrap_or(self.start);
        writer.write_f64_le(end_pos);
        writer.write_f64_le(CLIENT_HEIGHT - RADIUS);
        writer.write_f64_le(self.startv.x);
        writer.write_f64_le(self.startv.y);
        writer.write_i32_le(if self.doubled_score { 1 } else { 0 });
        writer.write_u64_le(self.score);
        writer.write_i32_le(self.difficulty as i32);
        for row in self.map {
            for b in row {
                writer.write_i32_le(b.to_i32());
            }
        }
        if let Some(ticker) = ticker {
            writer.write_i32_le((ticker.stopped + ticker.remain + ticker.balls.len()) as _);
            writer.write_i32_le(ticker.stopped as _);
            writer.write_i32_le(ticker.iloop as _);
            writer.write_u64_le(ticker.balls.len() as _);
            for b in ticker.balls() {
                writer.write_f64_le(b.pos.x);
                writer.write_f64_le(b.pos.y);
                writer.write_f64_le(b.speed.x);
                writer.write_f64_le(b.speed.y);
            }
        } else {
            writer.write_i32_le(self.balls_num as _);
            writer.write_i32_le(self.balls_num as _);
            writer.write_i32_le(0);
            writer.write_u64_le(0);
        }
        writer.0
    }

    pub fn from_bytes(bytes: &[u8]) -> std::io::Result<(Rc<RefCell<Self>>, Option<MapTicker>)> {
        let mut reader = BufferReader(Cursor::new(bytes));

        let ver = reader.read_i32_le();
        if ver != RECORD_VERSION {
            return Err(std::io::Error::new(
                std::io::ErrorKind::InvalidData,
                "record version mismatch",
            ));
        }
        let balls_num = reader.read_i32_le() as usize;
        let start = reader.read_f64_le();
        reader.read_f64_le();
        let end_pos = reader.read_f64_le();
        let new_start = if end_pos != start {
            Some(end_pos)
        } else {
            None
        };
        reader.read_f64_le();
        let x = reader.read_f64_le();
        let y = reader.read_f64_le();
        let startv = Vector::new(x, y);
        let doubled_score = reader.read_i32_le() != 0;
        let score = reader.read_u64_le();
        let difficulty = Difficulty::from_i32(reader.read_i32_le()).ok_or_else(|| {
            std::io::Error::new(std::io::ErrorKind::InvalidData, "invalid difficulty")
        })?;
        let mut map: [[BallType; COLUMNS]; ROWS] = Default::default();
        for row in &mut map {
            for b in row {
                *b = BallType::from_i32(reader.read_i32_le()).ok_or_else(|| {
                    std::io::Error::new(std::io::ErrorKind::InvalidData, "invalid ball type")
                })?;
            }
        }
        let map = Self {
            map,
            difficulty,
            score,
            start,
            startv,
            balls_num,
            doubled_score,
            ..Default::default()
        };
        let map = Rc::new(RefCell::new(map));

        let balls_num = reader.read_i32_le() as usize;
        let stopped = reader.read_i32_le() as usize;
        let iloop = reader.read_i32_le() as usize;
        let balls_len = reader.read_u64_le() as usize;
        if balls_len == 0 {
            Ok((map, None))
        } else {
            let mut balls = vec![];
            for _i in 0..balls_len {
                let posx = reader.read_f64_le();
                let posy = reader.read_f64_le();
                let speedx = reader.read_f64_le();
                let speedy = reader.read_f64_le();
                let b = Ball::new(Point::new(posx, posy), Vector::new(speedx, speedy));
                balls.push(b);
            }
            let ticker = MapTicker {
                map: map.clone(),
                balls,
                remain: balls_num - stopped - balls_len,
                stopped,
                iloop,
                new_start,
            };
            Ok((map, Some(ticker)))
        }
    }
}

fn get_side(pos: Point, ls: f64, ts: f64, rs: f64, bs: f64) -> BounceSide {
    let mut result = BounceSide::empty();
    let x = pos.x.round();
    let y = pos.y.round();
    if x - RADIUS <= ls {
        result |= BounceSide::LEFT;
    }
    if x + RADIUS > rs {
        result |= BounceSide::RIGHT;
    }
    if y - RADIUS <= ts {
        result |= BounceSide::TOP;
    }
    if bs < CLIENT_HEIGHT && y + RADIUS > bs {
        result |= BounceSide::BOTTOM;
    }
    result
}

fn change_ball(speed: &mut f64, pos: &mut f64, side: f64, minus: bool) {
    *speed = -*speed;
    let off = side - (*pos + if minus { -RADIUS } else { RADIUS });
    *pos += off * 2.0;
}

fn change_ball_arc(b: &mut Ball, sc: &Point, minus: bool) {
    let off = *sc - b.pos;
    let noff = Vector::new(off.y, off.x);
    b.pos = *sc + if minus { -noff } else { noff };
    let ns = Vector::new(-b.speed.y, -b.speed.x);
    b.speed = if minus { -ns } else { ns };
}

#[derive(Debug)]
pub struct MapTicker {
    map: Rc<RefCell<Map>>,
    balls: Vec<Ball>,
    remain: usize,
    stopped: usize,
    iloop: usize,
    new_start: Option<f64>,
}

impl MapTicker {
    fn new(map: Rc<RefCell<Map>>) -> Self {
        let remain = map.borrow().balls_num;
        Self {
            map,
            balls: vec![],
            remain,
            stopped: 0,
            iloop: 3,
            new_start: None,
        }
    }

    pub fn new_startp(&self) -> Option<Point> {
        self.new_start
            .map(|x| Point::new(x, CLIENT_HEIGHT - RADIUS))
    }

    pub fn is_end(&self) -> bool {
        self.remain == 0
    }

    pub fn balls(&self) -> &[Ball] {
        &self.balls
    }

    pub fn remain(&self) -> usize {
        self.remain
    }

    pub fn tick(&mut self) -> bool {
        let mut map = self.map.borrow_mut();
        self.iloop = (self.iloop + 1) % 4;
        if !self.is_end() && self.iloop == 0 {
            // Add a new ball
            self.balls.push(Ball::new(map.startp(), map.startv));
            self.remain -= 1;
        }
        let mut i = 0;
        while i < self.balls.len() {
            let b = &mut self.balls[i];
            if map.bounce(b) {
                self.stopped += 1;
                if b.pos.y + RADIUS >= CLIENT_HEIGHT && self.new_start.is_none() {
                    let tp = b.pos - b.speed;
                    let h = CLIENT_HEIGHT - RADIUS - tp.y;
                    let a = h / b.speed.y * b.speed.x;
                    let new_start = clip(tp.x + a, RADIUS, CLIENT_WIDTH - RADIUS);
                    self.new_start = Some(new_start);
                }
                self.balls.remove(i);
            } else {
                i += 1;
            }
        }
        !self.balls.is_empty()
    }
}

fn clip<T: PartialOrd>(v: T, min: T, max: T) -> T {
    if v < min {
        min
    } else if v > max {
        max
    } else {
        v
    }
}

impl Drop for MapTicker {
    fn drop(&mut self) {
        let mut map = self.map.borrow_mut();
        if let Some(start) = self.new_start {
            map.start = start;
        }
        map.doubled_score = false;
    }
}

macro_rules! impl_buffer_writer {
    ($t:ty) => {
        paste::paste! {
            pub fn [< write_ $t _le >](&mut self, v:$t) {
                let bytes = v.to_le_bytes();
                self.0.extend(bytes);
            }
        }
    };
}

struct BufferWriter(Vec<u8>);

impl BufferWriter {
    impl_buffer_writer!(i32);

    impl_buffer_writer!(u64);

    impl_buffer_writer!(f64);
}

macro_rules! impl_buffer_reader {
    ($t:ty) => {
        paste::paste! {
            pub fn [< read_ $t _le >](&mut self) -> $t {
                let mut bytes = [0u8; std::mem::size_of::<$t>()];
                self.0.read_exact(&mut bytes).unwrap();
                $t::from_le_bytes(bytes)
            }
        }
    };
}

struct BufferReader<'a>(Cursor<&'a [u8]>);

impl BufferReader<'_> {
    impl_buffer_reader!(i32);

    impl_buffer_reader!(u64);

    impl_buffer_reader!(f64);
}