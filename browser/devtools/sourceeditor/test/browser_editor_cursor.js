



"use strict";

function test() {
  waitForExplicitFinish();
  setup((ed, win) => {
    ch(ed.getCursor(), { line: 0, ch: 0 }, "default cursor position is ok");
    ed.setText("Hello.\nHow are you?");

    ed.setCursor({ line: 1, ch: 5 });
    ch(ed.getCursor(), { line: 1, ch: 5 }, "setCursor({ line, ch })");

    ch(ed.getPosition(7), { line: 1, ch: 0}, "getPosition(num)");
    ch(ed.getPosition(7, 1)[0], { line: 1, ch: 0}, "getPosition(num, num)[0]");
    ch(ed.getPosition(7, 1)[1], { line: 0, ch: 1}, "getPosition(num, num)[1]");

    ch(ed.getOffset({ line: 1, ch: 0 }), 7, "getOffset(num)");
    ch(ed.getOffset({ line: 1, ch: 0 }, { line: 0, ch: 1 })[0], 7, "getOffset(num, num)[0]");
    ch(ed.getOffset({ line: 1, ch: 0 }, { line: 0, ch: 1 })[0], 2, "getOffset(num, num)[1]");

    is(ed.getSelection(), "", "nothing is selected");
    ed.setSelection({ line: 0, ch: 0 }, { line: 0, ch: 5 });
    is(ed.getSelection(), "Hello", "setSelection");

    ed.extendSelection({ start: 0, length: 5 });
    is(ed.getSelection(), ".\nHow", "extendSelection");

    ed.dropSelection();
    is(ed.getSelection(), "", "dropSelection");

    teardown(ed, win);
  });
}