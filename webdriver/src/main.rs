use fantoccini::{
    actions::{self, InputSource, MouseActions, PointerAction}, Client, ClientBuilder, Locator
};
use std::time::{Duration, Instant};

struct Building {
    name: &'static str,
    id: &'static str,
    price: u32,
    cps: f32,
}

struct Context {
    client: Client,
    buildings: [Building; 5],
}

impl Context {
    async fn new() -> Self {
        Self {
            client: ClientBuilder::native().connect("").await.expect(""),
            buildings: [
                Building {name: "Cursor",   id: "product0", cps: 0.1, price: 15},
                Building {name: "Grandma",  id: "product1", cps: 1.0, price: 100},
                Building {name: "Farm",     id: "product2", cps: 8.0, price: 1100},
                Building {name: "Mine",     id: "product3", cps: 47.0, price: 12000},
                Building {name: "Factory",  id: "product4", cps: 260.0, price: 130000},
            ]
        }
    }

    async fn click_cookie(&self) {}
}

#[tokio::main(flavor = "current_thread")]
async fn main() -> Result<(), fantoccini::error::CmdError> {
    let c = ClientBuilder::native()
        .connect("http://localhost:4444")
        .await
        .expect("failed to connect to WebDriver");

    c.goto("file:///home/haru/projects/coding/code_scratch/webdriver/cookie_clicker/Cookie Clicker.html").await?;

    //c.wait().for_element(Locator::Css(".fc-cta-consent")).await?.click().await?;
    c.wait()
        .for_element(Locator::Css("#langSelect-EN"))
        .await?
        .click()
        .await?;

    let element_cookie = c.wait().for_element(Locator::Css("#bigCookie")).await?;

    let mouse_actions = MouseActions::new("mouse".into())
        .then(PointerAction::MoveToElement {
            element: element_cookie.clone(),
            duration: None,
            x: 0,
            y: 0,
        })
        .then(PointerAction::Down {
            button: actions::MOUSE_BUTTON_LEFT,
        })
        .then(PointerAction::Up {
            button: actions::MOUSE_BUTTON_LEFT,
        });

    let mut elapsed: Duration = Duration::new(0, 0);
    let mut count = 0;

    let now = Instant::now();
    while elapsed.as_millis() < 4000 {
        if elapsed.as_millis() % 100 == 0 {
            c.perform_actions(mouse_actions.clone()).await?;
            count += 1;
        }
        elapsed = now.elapsed();
    }

    println!(
        "Coooies: {}",
        c.wait()
            .for_element(Locator::Id("cookies"))
            .await?
            .text()
            .await?
    );
    println!("Count: {count}");

    //let cookie_click_action = MouseActions::new("cookie_click".into()).then(PointerAction::);
    //c.perform_actions(cookie_click_action).await?;
    //actions::Actions

    c.close().await
}
