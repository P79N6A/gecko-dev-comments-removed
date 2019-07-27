


"use strict";




const TEST_URI = "chrome://browser/content/devtools/filter-frame.xhtml";
const {CSSFilterEditorWidget} = require("devtools/shared/widgets/FilterWidget");

const FAST_VALUE_MULTIPLIER = 10;
const SLOW_VALUE_MULTIPLIER = 0.1;
const DEFAULT_VALUE_MULTIPLIER = 1;

add_task(function*() {
  yield promiseTab("about:blank");
  let [host, win, doc] = yield createHost("bottom", TEST_URI);

  const container = doc.querySelector("#container");
  const initialValue = "drop-shadow(rgb(0, 0, 0) 1px 1px 0px)";
  let widget = new CSSFilterEditorWidget(container, initialValue);
  widget.el.querySelector("input").setSelectionRange(13, 13);

  let value = 1;

  triggerKey = triggerKey.bind(widget);

  info("Test simple arrow keys");
  triggerKey(40);

  value -= DEFAULT_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should decrease value using down arrow");

  triggerKey(38);

  value += DEFAULT_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should decrease value using down arrow");

  info("Test shift key multiplier");
  triggerKey(38, "shiftKey");

  value += FAST_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should increase value by fast multiplier using up arrow");

  triggerKey(40, "shiftKey");

  value -= FAST_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should decrease value by fast multiplier using down arrow");

  info("Test alt key multiplier");
  triggerKey(38, "altKey");

  value += SLOW_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should increase value by slow multiplier using up arrow");

  triggerKey(40, "altKey");

  value -= SLOW_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should decrease value by slow multiplier using down arrow");

  triggerKey(40, "shiftKey");

  value -= FAST_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should decrease to negative");

  triggerKey(40);

  value -= DEFAULT_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should decrease negative numbers correctly");

  triggerKey(38);

  value += DEFAULT_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should increase negative values correctly");

  triggerKey(40, "altKey");
  triggerKey(40, "altKey");

  value -= SLOW_VALUE_MULTIPLIER * 2;
  is(widget.getValueAt(0), val(value),
     "Should decrease float numbers correctly");

  triggerKey(38, "altKey");

  value += SLOW_VALUE_MULTIPLIER;
  is(widget.getValueAt(0), val(value),
     "Should increase float numbers correctly");

  triggerKey = null;
});



function triggerKey(key, modifier) {
  const filter = this.el.querySelector(".filters").children[0];
  const input = filter.querySelector("input");

  this._keyDown({
    target: input,
    keyCode: key,
    [modifier]: true,
    preventDefault: function() {}
  });
}

function val(value) {
  let v = value.toFixed(1);

  if (v.indexOf(".0") > -1) {
    v = v.slice(0, -2);
  }
  return `rgb(0, 0, 0) ${v}px 1px 0px`;
}
