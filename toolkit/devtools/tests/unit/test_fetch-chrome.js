


"use strict";



const URL_FOUND = "chrome://global/locale/devtools/debugger.properties";
const URL_NOT_FOUND = "chrome://this/is/not/here.js";




add_task(function* test_missing() {
  yield DevToolsUtils.fetch(URL_NOT_FOUND).then(result => {
    do_print(result);
    ok(false, "fetch resolved unexpectedly for non-existent chrome:// URI");
  }, () => {
    ok(true, "fetch rejected as the chrome:// URI was non-existent.");
  });
});




add_task(function* test_normal() {
  yield DevToolsUtils.fetch(URL_FOUND).then(result => {
    notDeepEqual(result.content, "",
      "chrome:// URI seems to be read correctly.");
  });
});
