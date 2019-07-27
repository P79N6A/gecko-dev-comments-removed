


"use strict";



const URL_FOUND = "resource://gre/modules/devtools/DevToolsUtils.js";
const URL_NOT_FOUND = "resource://gre/modules/devtools/this/is/not/here.js";




add_task(function* test_missing() {
  yield DevToolsUtils.fetch(URL_NOT_FOUND).then(result => {
    do_print(result);
    ok(false, "fetch resolved unexpectedly for non-existent resource:// URI");
  }, () => {
    ok(true, "fetch rejected as the resource:// URI was non-existent.");
  });
});




add_task(function* test_normal() {
  yield DevToolsUtils.fetch(URL_FOUND).then(result => {
    notDeepEqual(result.content, "",
      "resource:// URI seems to be read correctly.");
  });
});
