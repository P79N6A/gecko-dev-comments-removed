



"use strict";

function test() {
  waitForExplicitFinish();
  setup((ed, win) => {
    
    let src = win.document.querySelector("iframe").getAttribute("src");
    ok(~src.indexOf(".CodeMirror"), "correct iframe is there");

    
    is(ed.getMode(), Editor.modes.text, "getMode");
    ed.setMode(Editor.modes.js);
    is(ed.getMode(), Editor.modes.js, "setMode");

    
    is(ed.getText(), "Hello.", "getText");
    ed.setText("Hi.\nHow are you?");
    is(ed.getText(), "Hi.\nHow are you?", "setText");
    is(ed.getText(1), "How are you?", "getText(num)");
    is(ed.getText(5), "", "getText(num) when num is out of scope");

    ed.replaceText("YOU", { line: 1, ch: 8 }, { line: 1, ch: 11 });
    is(ed.getText(1), "How are YOU?", "replaceText(str, from, to)");
    ed.replaceText("you?", { line: 1, ch: 8 });
    is(ed.getText(1), "How are you?", "replaceText(str, from)");
    ed.replaceText("Hello.");
    is(ed.getText(), "Hello.", "replaceText(str)");

    ed.insertText(", sir/madam", { line: 0, ch: 5});
    is(ed.getText(), "Hello, sir/madam.", "insertText");

    
    ed.extend({ whoami: () => "Anton", whereami: () => "Mozilla" });
    is(ed.whoami(), "Anton", "extend/1");
    is(ed.whereami(), "Mozilla", "extend/2");

    
    ed.setText("Hello!\nHow are you?");
    ok(!ed.hasLineClass(0, "test"), "no test line class");
    ed.addLineClass(0, "test");
    ok(ed.hasLineClass(0, "test"), "test line class is there");
    ed.removeLineClass(0, "test");
    ok(!ed.hasLineClass(0, "test"), "test line class is gone");

    teardown(ed, win);
  });
}