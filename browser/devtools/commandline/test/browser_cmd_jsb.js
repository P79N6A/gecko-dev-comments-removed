




const TEST_URI = "http://example.com/browser/browser/devtools/commandline/" +
                 "test/browser_cmd_jsb_script.jsi";

function test() {
  return Task.spawn(testTask).then(finish, helpers.handleError);
}

function* testTask() {
  let options = yield helpers.openTab("about:blank");
  yield helpers.openToolbar(options);

  let notifyPromise = wwNotifyOnce();

  helpers.audit(options, [
    {
      setup: 'jsb',
      check: {
        input:  'jsb',
        hints:     ' <url> [options]',
        markup: 'VVV',
        status: 'ERROR'
      }
    },
    {
      setup: 'jsb ' + TEST_URI,
      
      exec: {
        output: '',
        error: false
      }
    }
  ]);

  let { subject } = yield notifyPromise;
  let scratchpadWin = subject.QueryInterface(Ci.nsIDOMWindow);
  yield helpers.listenOnce(scratchpadWin, "load");

  let scratchpad = scratchpadWin.Scratchpad;

  yield observeOnce(scratchpad);

  let result = scratchpad.getText();
  result = result.replace(/[\r\n]]*/g, "\n");
  let correct = "function somefunc() {\n" +
            "  if (true) // Some comment\n" +
            "    doSomething();\n" +
            "  for (let n = 0; n < 500; n++) {\n" +
            "    if (n % 2 == 1) {\n" +
            "      console.log(n);\n" +
            "      console.log(n + 1);\n" +
            "    }\n" +
            "  }\n" +
            "}";
  is(result, correct, "JS has been correctly prettified");

  if (scratchpadWin) {
    scratchpadWin.close();
    scratchpadWin = null;
  }

  yield helpers.closeToolbar(options);
  yield helpers.closeTab(options);
}







function wwNotifyOnce() {
  return new Promise(resolve => {
    let onNotify = (subject, topic, data) => {
      if (topic == "domwindowopened") {
        Services.ww.unregisterNotification(onNotify);
        resolve({ subject: subject, topic: topic, data: data });
      }
    };

    Services.ww.registerNotification(onNotify);
  });
}





function observeOnce(scratchpad) {
  return new Promise(resolve => {
    let observer = {
      onReady: function() {
        scratchpad.removeObserver(observer);
        resolve();
      },
    };
    scratchpad.addObserver(observer);
  });
}
