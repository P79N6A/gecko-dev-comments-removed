



"use strict";





const PAGE_CONTENT = [
  '<style type="text/css">',
  '  body {',
  '    background: url("chrome://global/skin/icons/warning-64.png"), linear-gradient(white, #F06 400px);',
  '  }',
  '</style>',
  'Testing the color picker tooltip!'
].join("\n");

function test() {
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function load(evt) {
    gBrowser.selectedBrowser.removeEventListener("load", load, true);
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,rule view color picker tooltip test";
}

function createDocument() {
  content.document.body.innerHTML = PAGE_CONTENT;

  openRuleView((inspector, ruleView) => {
    inspector.once("inspector-updated", () => {
      let value = getRuleViewProperty("background", ruleView).valueSpan;
      let swatch = value.querySelector(".ruleview-colorswatch");
      let url = value.querySelector(".theme-link");
      testImageTooltipAfterColorChange(swatch, url, ruleView);
    });
  });
}

function testImageTooltipAfterColorChange(swatch, url, ruleView) {
  Task.spawn(function*() {
    info("First, verify that the image preview tooltip works");
    let anchor = yield isHoverTooltipTarget(ruleView.previewTooltip, url);
    ok(anchor, "The image preview tooltip is shown on the url span");
    is(anchor, url, "The anchor returned by the showOnHover callback is correct");

    info("Open the color picker tooltip and change the color");
    let picker = ruleView.colorPicker;
    let onShown = picker.tooltip.once("shown");
    swatch.click();
    yield onShown;
    yield simulateColorChange(picker, [0, 0, 0, 1], {
      element: content.document.body,
      name: "backgroundImage",
      value: 'url("chrome://global/skin/icons/warning-64.png"), linear-gradient(rgb(0, 0, 0), rgb(255, 0, 102) 400px)'
    });

    let spectrum = yield picker.spectrum;
    let onHidden = picker.tooltip.once("hidden");
    EventUtils.sendKey("RETURN", spectrum.element.ownerDocument.defaultView);
    yield onHidden;

    info("Verify again that the image preview tooltip works");
    
    
    url = getRuleViewProperty("background", ruleView).valueSpan.querySelector(".theme-link");
    let anchor = yield isHoverTooltipTarget(ruleView.previewTooltip, url);
    ok(anchor, "The image preview tooltip is shown on the url span");
    is(anchor, url, "The anchor returned by the showOnHover callback is correct");
  }).then(finish);
}
