



"use strict";



let test = asyncTest(function*() {
  let projecteditor = yield addProjectEditorTabForTempDirectory();
  ok(projecteditor, "ProjectEditor has loaded");

});