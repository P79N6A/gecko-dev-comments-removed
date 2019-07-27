





"use strict";

add_task(function*() {
  yield runTest();
});

function* runTest() {
  const baseURL = "chrome://mochitests/content/browser/browser/devtools/sourceeditor/test"
  const injectedText = "Script successfully injected !";

  let {ed, win} = yield setup(null, {
    mode: "ruby",
    externalScripts: [`${baseURL}/cm_script_injection_test.js`,
                      `${baseURL}/cm_mode_ruby.js`]
  });

  is(ed.getText(), injectedText, "The text has been injected");
  is(ed.getOption("mode"), "ruby", "The ruby mode is correctly set");
  teardown(ed, win);
}