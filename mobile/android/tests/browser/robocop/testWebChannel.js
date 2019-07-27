




const { classes: Cc, interfaces: Ci, utils: Cu } = Components; 

Cu.import("resource://gre/modules/Promise.jsm"); 
Cu.import("resource://gre/modules/Services.jsm"); 
Cu.import("resource://gre/modules/XPCOMUtils.jsm"); 
XPCOMUtils.defineLazyModuleGetter(this, "WebChannel",
  "resource://gre/modules/WebChannel.jsm"); 

const HTTP_PATH = "http://mochi.test:8888";
const HTTP_ENDPOINT = "/tests/robocop/testWebChannel.html";

const gChromeWin = Services.wm.getMostRecentWindow("navigator:browser");
let BrowserApp = gChromeWin.BrowserApp;




function ok(passed, text) {
  do_report_result(passed, text, Components.stack.caller, false);
}

function is(lhs, rhs, text) {
  do_report_result(lhs === rhs, "[ " + lhs + " === " + rhs + " ] " + text,
    Components.stack.caller, false);
}




let gTests = [
  {
    desc: "WebChannel generic message",
    run: function* () {
      return new Promise(function(resolve, reject) {
        let tab;
        let channel = new WebChannel("generic", Services.io.newURI(HTTP_PATH, null, null));
        channel.listen(function (id, message, target) {
          is(id, "generic");
          is(message.something.nested, "hello");
          channel.stopListening();
          BrowserApp.closeTab(tab);
          resolve();
        });

        tab = BrowserApp.addTab(HTTP_PATH + HTTP_ENDPOINT + "?generic");
      });
    }
  },
  {
    desc: "WebChannel two way communication",
    run: function* () {
      return new Promise(function(resolve, reject) {
        let tab;
        let channel = new WebChannel("twoway", Services.io.newURI(HTTP_PATH, null, null));

        channel.listen(function (id, message, sender) {
          is(id, "twoway");
          ok(message.command);

          if (message.command === "one") {
            channel.send({ data: { nested: true } }, sender);
          }

          if (message.command === "two") {
            is(message.detail.data.nested, true);
            channel.stopListening();
            BrowserApp.closeTab(tab);
            resolve();
          }
        });

        tab = BrowserApp.addTab(HTTP_PATH + HTTP_ENDPOINT + "?twoway");
      });
    }
  },
  {
    desc: "WebChannel multichannel",
    run: function* () {
      return new Promise(function(resolve, reject) {
        let tab;
        let channel = new WebChannel("multichannel", Services.io.newURI(HTTP_PATH, null, null));

        channel.listen(function (id, message, sender) {
          is(id, "multichannel");
          BrowserApp.closeTab(tab);
          resolve();
        });

        tab = BrowserApp.addTab(HTTP_PATH + HTTP_ENDPOINT + "?multichannel");
      });
    }
  }
]; 

add_task(function test() {
  for (let test of gTests) {
    do_print("Running: " + test.desc);
    yield test.run();
  }
});

run_next_test();
