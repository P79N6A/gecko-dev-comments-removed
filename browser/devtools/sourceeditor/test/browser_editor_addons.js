


"use strict";

function test() {
  waitForExplicitFinish();

  setup((ed, win) => {
    let doc = win.document.querySelector("iframe").contentWindow.document;

    
    ed.setText("Hello   ");
    ed.setOption("showTrailingSpace", false);
    ok(!doc.querySelector(".cm-trailingspace"));
    ed.setOption("showTrailingSpace", true);
    ok(doc.querySelector(".cm-trailingspace"));

    
    ed.setMode(Editor.modes.js);
    ed.setText("function main() {\nreturn 'Hello, World!';\n}");
    executeSoon(() => testFold(doc, ed, win));
  });
}

function testFold(doc, ed, win) {
  
  if (!doc.querySelector(".CodeMirror-foldgutter-open")) {
    executeSoon(() => testFold(doc, ed, win));
    return;
  }

  teardown(ed, win);
}