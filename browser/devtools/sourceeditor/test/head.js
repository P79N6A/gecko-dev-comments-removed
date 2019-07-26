



"use strict";

const require = Cu.import("resource://gre/modules/devtools/Loader.jsm", {}).devtools.require;
const Editor  = require("devtools/sourceeditor/editor");

function setup(cb) {
  const opt = "chrome,titlebar,toolbar,centerscreen,resizable,dialog=no";
  const url = "data:text/xml;charset=UTF-8,<?xml version='1.0'?>" +
    "<?xml-stylesheet href='chrome://global/skin/global.css'?>" +
    "<window xmlns='http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul'" +
    " title='Editor' width='600' height='500'><box flex='1'/></window>";

  let win = Services.ww.openWindow(null, url, "_blank", opt, null);

  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);

    waitForFocus(function () {
      let box = win.document.querySelector("box");
      let editor = new Editor({
        value: "Hello.",
        lineNumbers: true,
        foldGutter: true,
        gutters: [ "CodeMirror-linenumbers", "breakpoints", "CodeMirror-foldgutter" ]
      });

      editor.appendTo(box)
        .then(() => cb(editor, win))
        .then(null, (err) => ok(false, err.message));
    }, win);
  }, false);
}

function ch(exp, act, label) {
  is(exp.line, act.line, label + " (line)");
  is(exp.ch, act.ch, label + " (ch)");
}

function teardown(ed, win) {
  ed.destroy();
  win.close();
  finish();
}