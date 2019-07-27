


"use strict";



const TEST_URI = "chrome://browser/content/devtools/filter-frame.xhtml";
const {CSSFilterEditorWidget} = require("devtools/shared/widgets/FilterWidget");

add_task(function* () {
  yield promiseTab("about:blank");

  let [host, win, doc] = yield createHost("bottom", TEST_URI);

  const container = doc.querySelector("#container");
  let widget = new CSSFilterEditorWidget(container, "none");
  
  yield widget.once("render");

  const VALUE = "blur(2px) contrast(150%)";
  const NAME = "Test";

  yield showFilterPopupPresetsAndCreatePreset(widget, NAME, VALUE);

  let onRender = widget.once("render");
  
  widget.setCssValue("saturate(100%) brightness(150%)");
  yield onRender;

  let preset = widget.el.querySelector(".preset");

  onRender = widget.once("render");
  widget._presetClick({
    target: preset
  });

  yield onRender;

  is(widget.getCssValue(), VALUE,
     "Should set widget's value correctly");
  is(widget.el.querySelector(".presets-list .footer input").value, NAME,
     "Should set input's value to name");
});
