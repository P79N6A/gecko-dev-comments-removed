



"use strict";




const TEST_PAGE = [
  'data:text/html;charset=utf-8,',
  '<style type="text/css">',
  '  div {',
  '    width: 300px;height: 300px;border-radius: 50%;',
  '    background: red url(chrome://global/skin/icons/warning-64.png);',
  '  }',
  '</style>',
  '<div></div>'
].join("\n");

add_task(function*() {
  yield addTab(TEST_PAGE);
  let {toolbox, inspector, view} = yield openRuleView();

  yield selectNode("div", inspector);

  yield testImageDimension(view);
  yield testPickerDimension(view);
});

function* testImageDimension(ruleView) {
  info("Testing background-image tooltip dimensions");

  let tooltip = ruleView.tooltips.previewTooltip;
  let panel = tooltip.panel;
  let {valueSpan} = getRuleViewProperty(ruleView, "div", "background");
  let uriSpan = valueSpan.querySelector(".theme-link");

  
  
  yield assertHoverTooltipOn(tooltip, uriSpan);

  info("Showing the tooltip");
  let onShown = tooltip.once("shown");
  tooltip.show();
  yield onShown;

  
  
  let imageRect = panel.querySelector("image").getBoundingClientRect();
  let panelRect = panel.getBoundingClientRect();

  ok(panelRect.width >= imageRect.width,
    "The panel is wide enough to show the image");
  ok(panelRect.height >= imageRect.height,
    "The panel is high enough to show the image");

  let onHidden = tooltip.once("hidden");
  tooltip.hide();
  yield onHidden;
}

function* testPickerDimension(ruleView) {
  info("Testing color-picker tooltip dimensions");

  let {valueSpan} = getRuleViewProperty(ruleView, "div", "background");
  let swatch = valueSpan.querySelector(".ruleview-colorswatch");
  let cPicker = ruleView.tooltips.colorPicker;

  let onShown = cPicker.tooltip.once("shown");
  swatch.click();
  yield onShown;

  
  
  let w = cPicker.tooltip.panel.querySelector("iframe").width;
  let h = cPicker.tooltip.panel.querySelector("iframe").height;
  let panelRect = cPicker.tooltip.panel.getBoundingClientRect();

  ok(panelRect.width >= w, "The panel is wide enough to show the picker");
  ok(panelRect.height >= h, "The panel is high enough to show the picker");

  let onHidden = cPicker.tooltip.once("hidden");
  let onRuleViewChanged = ruleView.once("ruleview-changed");
  cPicker.hide();
  yield onHidden;
  yield onRuleViewChanged;
}
