cargo build --target wasm32-unknown-unknown --release
wasm-gc target/wasm32-unknown-unknown/release/mm.wasm -o ../../hermanw.github.io/md5mobile/mm.wasm