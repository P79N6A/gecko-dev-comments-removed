



"use strict";



add_task(function*() {
  let projecteditor = yield addProjectEditorTabForTempDirectory({
    menubar: false
  });
  ok(projecteditor, "ProjectEditor has loaded");

  let contextMenuPopup = projecteditor.document.querySelector("#context-menu-popup");
  let textEditorContextMenuPopup = projecteditor.document.querySelector("#texteditor-context-popup");
  ok (contextMenuPopup, "The menu has loaded in the projecteditor document");
  ok (textEditorContextMenuPopup, "The menu has loaded in the projecteditor document");

  let projecteditor2 = yield addProjectEditorTabForTempDirectory();
  contextMenuPopup = projecteditor2.document.getElementById("context-menu-popup");
  textEditorContextMenuPopup = projecteditor2.document.getElementById("texteditor-context-popup");
  ok (!contextMenuPopup, "The menu has NOT loaded in the projecteditor document");
  ok (!textEditorContextMenuPopup, "The menu has NOT loaded in the projecteditor document");
  ok (content.document.querySelector("#context-menu-popup"), "The menu has loaded in the specified element");
  ok (content.document.querySelector("#texteditor-context-popup"), "The menu has loaded in the specified element");
});
