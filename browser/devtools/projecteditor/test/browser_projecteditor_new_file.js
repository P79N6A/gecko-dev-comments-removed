



"use strict";



add_task(function*() {
  let projecteditor = yield addProjectEditorTabForTempDirectory();
  ok(projecteditor, "ProjectEditor has loaded");

});