



"use strict";



const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body {',
  '    background-color: #ededed;',
  '  }',
  '</style>',
  'Testing the color picker tooltip!'
].join("\n");

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8,rule view color picker tooltip test");
  content.document.body.innerHTML = PAGE_CONTENT;
  let {toolbox, inspector, view} = yield openRuleView();

  let swatch = getRuleViewProperty(view, "body", "background-color").valueSpan
    .querySelector(".ruleview-colorswatch");
  yield testPressingEscapeRevertsChanges(swatch, view);
});

function* testPressingEscapeRevertsChanges(swatch, ruleView) {
  let cPicker = ruleView.tooltips.colorPicker;

  let onShown = cPicker.tooltip.once("shown");
  swatch.click();
  yield onShown;

  yield simulateColorPickerChange(cPicker, [0, 0, 0, 1], {
    element: content.document.body,
    name: "backgroundColor",
    value: "rgb(0, 0, 0)"
  });

  is(swatch.style.backgroundColor, "rgb(0, 0, 0)",
    "The color swatch's background was updated");
  is(getRuleViewProperty(ruleView, "body", "background-color").valueSpan.textContent,
    "#000", "The text of the background-color css property was updated");

  let spectrum = yield cPicker.spectrum;

  
  let onHidden = cPicker.tooltip.once("hidden");
  EventUtils.sendKey("ESCAPE", spectrum.element.ownerDocument.defaultView);
  yield onHidden;

  yield waitForSuccess(() => {
    return content.getComputedStyle(content.document.body).backgroundColor === "rgb(237, 237, 237)";
  }, "The element's background-color was reverted");
}
