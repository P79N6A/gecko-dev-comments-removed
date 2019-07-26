


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

    teardown(ed, win);
  });
}