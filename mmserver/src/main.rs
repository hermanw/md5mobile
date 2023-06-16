mod decoder;

use actix_web::{web, App, HttpServer, HttpRequest};
use std::sync::Mutex;
use std::env;

async fn decode(data: web::Data<Mutex<decoder::Decoder>>, req: HttpRequest) -> String {
    let hash = req.match_info().get("hash").unwrap();
    let mut decoder = data.lock().unwrap();
    let mobile = decoder.look_up(&hash.to_string());
    mobile.to_string()
}

#[actix_web::main]
async fn main() -> std::io::Result<()> {
    let mut port = "8000";
    let args: Vec<String> = env::args().collect();
    if args.len() > 1 {
        port = args[1].as_str()
    }

    let decoder = web::Data::new(Mutex::new(decoder::init()));

    println!("Start http server");
    HttpServer::new(move || {
        App::new()
            .app_data(decoder.clone())
            .service(web::resource("/").to(decode))
    })
    .bind(("127.0.0.1", port.parse().unwrap()))
    .expect("Can not bind HttpServer")
    .run()
    .await
}


