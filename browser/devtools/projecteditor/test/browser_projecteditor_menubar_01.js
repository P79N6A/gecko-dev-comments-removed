



"use strict";



let test = asyncTest(function*() {
  let projecteditor = yield addProjectEditorTabForTempDirectory({
    menubar: false
  });
  ok(projecteditor, "ProjectEditor has loaded");

  let fileMenu = projecteditor.document.getElementById("file-menu");
  let editMenu = projecteditor.document.getElementById("edit-menu");
  ok (fileMenu, "The menu has loaded in the projecteditor document");
  ok (editMenu, "The menu has loaded in the projecteditor document");

  let projecteditor2 = yield addProjectEditorTabForTempDirectory();
  let menubar = projecteditor2.menubar;
  fileMenu = projecteditor2.document.getElementById("file-menu");
  editMenu = projecteditor2.document.getElementById("edit-menu");
  ok (!fileMenu, "The menu has NOT loaded in the projecteditor document");
  ok (!editMenu, "The menu has NOT loaded in the projecteditor document");
  ok (content.document.querySelector("#file-menu"), "The menu has loaded in the specified element");
  ok (content.document.querySelector("#edit-menu"), "The menu has loaded in the specified element");
});
