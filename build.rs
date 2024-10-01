fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("windows") {
        println!("cargo:rerun-if-changed=build.rs");
        println!("cargo:rerun-if-changed=logo.ico");
        winres::WindowsResource::new()
            .set_icon("logo.ico")
            .compile()
            .unwrap();
    }
}
