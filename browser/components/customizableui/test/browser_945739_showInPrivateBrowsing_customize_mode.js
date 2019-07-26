



"use strict";

const kWidgetId = "test-private-browsing-customize-mode-widget";




add_task(function testPrivateBrowsingCustomizeModeWidget() {
  CustomizableUI.createWidget({
    id: kWidgetId,
    showInPrivateBrowsing: false
  });

  let normalWidgetArray = CustomizableUI.getUnusedWidgets(gNavToolbox.palette);
  normalWidgetArray = normalWidgetArray.map((w) => w.id);
  ok(normalWidgetArray.indexOf(kWidgetId) > -1,
     "Widget should appear as unused in non-private window");

  let privateWindow = yield openAndLoadWindow({private: true});
  let privateWidgetArray = CustomizableUI.getUnusedWidgets(privateWindow.gNavToolbox.palette);
  privateWidgetArray = privateWidgetArray.map((w) => w.id);
  is(privateWidgetArray.indexOf(kWidgetId), -1,
     "Widget should not appear as unused in private window");
  privateWindow.close();

  CustomizableUI.destroyWidget(kWidgetId); 
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});
