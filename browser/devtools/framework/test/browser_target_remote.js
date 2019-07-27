


let { devtools } =
  Cu.import("resource://gre/modules/devtools/Loader.jsm", {});


function test() {
  waitForExplicitFinish();

  getChromeActors((client, response) => {
    let options = {
      form: response,
      client: client,
      chrome: true
    };

    devtools.TargetFactory.forRemoteTab(options).then(target => {
      target.on("close", () => {
        ok(true, "Target was closed");
        finish();
      });
      client.close();
    });
  });
}
