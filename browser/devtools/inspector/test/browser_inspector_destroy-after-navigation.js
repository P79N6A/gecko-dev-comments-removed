


"use strict";



const URL_1 = "data:text/plain;charset=UTF-8,abcde";
const URL_2 = "data:text/plain;charset=UTF-8,12345";

add_task(function* () {
  let { toolbox } = yield openInspectorForURL(URL_1);

  info("Navigating to different URL.");
  let navigated = toolbox.target.once("navigate");
  navigateTo(toolbox, URL_2);

  info("Waiting for 'navigate' event from toolbox target.");
  yield navigated;

  info("Destroying toolbox");
  try {
    yield toolbox.destroy();
    ok(true, "Toolbox destroyed");
  } catch (e) {
    ok(false, "An exception occured while destroying toolbox");
    console.error(e);
  }
});
