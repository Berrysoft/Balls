fn main() {
    if std::env::var("CARGO_CFG_TARGET_OS").as_deref() == Ok("windows") {
        println!("cargo:rerun-if-changed=build.rs");
        println!("cargo:rerun-if-changed=logo.ico");
        winres::WindowsResource::new()
            .set_icon("logo.ico")
            .compile()
            .unwrap();
    }
    #[cfg(feature = "vc-ltl")]
    {
        use std::path::Path;

        let target_arch = std::env::var("CARGO_CFG_TARGET_ARCH").unwrap();
        let target_platform = if target_arch.contains("x86_64") {
            "x64"
        } else if target_arch.contains("x86") {
            "Win32"
        } else if target_arch.contains("aarch64") {
            "ARM64"
        } else {
            println!(
                "cargo:warning=VC-LTL does not support {target_arch} platform. So the VC-LTL \
                 stage is skipped."
            );
            return;
        };
        let Some(vc_ltl_path) = std::env::var_os("VC_LTL_LIB") else {
            return;
        };
        let library_folder = Path::new(&vc_ltl_path).join(target_platform);
        println!("cargo:rustc-link-search={}", library_folder.display());
    }
}
