



"use strict";





const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body {',
  '    background: url("chrome://global/skin/icons/warning-64.png"), linear-gradient(white, #F06 400px);',
  '  }',
  '</style>',
  'Testing the color picker tooltip!'
].join("\n");

let test = asyncTest(function*() {
  yield addTab("data:text/html;charset=utf-8,rule view color picker tooltip test");
  content.document.body.innerHTML = PAGE_CONTENT;
  let {toolbox, inspector, view} = yield openRuleView();

  let value = getRuleViewProperty(view, "body", "background").valueSpan;
  let swatch = value.querySelector(".ruleview-colorswatch");
  let url = value.querySelector(".theme-link");
  yield testImageTooltipAfterColorChange(swatch, url, view);
});

function* testImageTooltipAfterColorChange(swatch, url, ruleView) {
  info("First, verify that the image preview tooltip works");
  let anchor = yield isHoverTooltipTarget(ruleView.tooltips.previewTooltip, url);
  ok(anchor, "The image preview tooltip is shown on the url span");
  is(anchor, url, "The anchor returned by the showOnHover callback is correct");

  info("Open the color picker tooltip and change the color");
  let picker = ruleView.tooltips.colorPicker;
  let onShown = picker.tooltip.once("shown");
  swatch.click();
  yield onShown;
  yield simulateColorPickerChange(picker, [0, 0, 0, 1], {
    element: content.document.body,
    name: "backgroundImage",
    value: 'url("chrome://global/skin/icons/warning-64.png"), linear-gradient(rgb(0, 0, 0), rgb(255, 0, 102) 400px)'
  });

  let spectrum = yield picker.spectrum;
  let onHidden = picker.tooltip.once("hidden");
  EventUtils.sendKey("RETURN", spectrum.element.ownerDocument.defaultView);
  yield onHidden;

  info("Verify again that the image preview tooltip works");
  
  
  url = getRuleViewProperty(ruleView, "body", "background").valueSpan.querySelector(".theme-link");
  anchor = yield isHoverTooltipTarget(ruleView.tooltips.previewTooltip, url);
  ok(anchor, "The image preview tooltip is shown on the url span");
  is(anchor, url, "The anchor returned by the showOnHover callback is correct");
}
