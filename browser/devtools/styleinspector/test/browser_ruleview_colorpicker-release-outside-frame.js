



"use strict";




const PAGE_CONTENT = "data:text/html;charset=utf-8," +
  '<body style="color: red">Test page for bug 1160720';

add_task(function*() {
  yield addTab(PAGE_CONTENT);
  let {toolbox, inspector, view} = yield openRuleView();

  let cSwatch = getRuleViewProperty(view, "element", "color").valueSpan
    .querySelector(".ruleview-colorswatch");

  let picker = yield openColorPickerForSwatch(cSwatch, view);
  let spectrum = yield picker.spectrum;
  let change = spectrum.once("changed");

  info("Pressing mouse down over color picker.");
  let onRuleViewChanged = view.once("ruleview-changed");
  EventUtils.synthesizeMouseAtCenter(spectrum.dragger, {
    type: "mousedown",
  }, spectrum.dragger.ownerDocument.defaultView);
  yield onRuleViewChanged;

  let value = yield change;
  info(`Color changed to ${value} on mousedown.`);

  
  
  
  spectrum.once("changed", (event, newValue) => {
    is(newValue, value, "Value changed on mousemove without a button pressed.");
  });

  
  
  

  info("Moving mouse over color picker without any buttons pressed.");

  EventUtils.synthesizeMouse(spectrum.dragger, 10, 10, {
    button: -1, 
    type: "mousemove",
  }, spectrum.dragger.ownerDocument.defaultView);
});

function* openColorPickerForSwatch(swatch, view) {
  let cPicker = view.tooltips.colorPicker;
  ok(cPicker, "The rule-view has the expected colorPicker property");

  let cPickerPanel = cPicker.tooltip.panel;
  ok(cPickerPanel, "The XUL panel for the color picker exists");

  let onShown = cPicker.tooltip.once("shown");
  swatch.click();
  yield onShown;

  ok(true, "The color picker was shown on click of the color swatch");

  return cPicker;
}
