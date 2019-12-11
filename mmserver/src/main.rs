mod decoder;

use actix_web::{web, App, HttpRequest, HttpServer};
use std::sync::Mutex;

fn decode(data: web::Data<Mutex<decoder::Decoder>>, req: HttpRequest) -> String {
    let hash = req.match_info().get("hash").unwrap();
    let mut decoder = data.lock().unwrap();
    let mobile = decoder.look_up(&hash.to_string());
    mobile.to_string()
}

fn main() {
    let decoder = web::Data::new(Mutex::new(decoder::init()));

    println!("Start http server");
    HttpServer::new(move || {
        App::new()
            .register_data(decoder.clone())
            .route("/{hash}", web::get().to(decode))
    })
    .bind("127.0.0.1:8000")
    .expect("Can not bind to port 8000")
    .run()
    .unwrap();

}


