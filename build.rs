fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("windows") {
        println!("cargo:rerun-if-changed=build.rs");
        println!("cargo:rerun-if-changed=info.rc");
        println!("cargo:rerun-if-changed=logo.ico");
        embed_resource::compile("info.rc", embed_resource::NONE);
    }
}
