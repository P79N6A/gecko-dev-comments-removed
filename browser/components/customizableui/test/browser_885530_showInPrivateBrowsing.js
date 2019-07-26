



"use strict";

const kWidgetId = "some-widget";

function assertWidgetExists(aWindow, aExists) {
  if (aExists) {
    ok(aWindow.document.getElementById(kWidgetId),
       "Should have found test widget in the window");
  } else {
    is(aWindow.document.getElementById(kWidgetId), null,
       "Should not have found test widget in the window");
  }
}



add_task(function() {
  let wrapper = CustomizableUI.createWidget({
    id: kWidgetId
  });
  ok(wrapper.showInPrivateBrowsing,
     "showInPrivateBrowsing should have defaulted to true.");
  CustomizableUI.destroyWidget(kWidgetId);
});




add_task(function() {
  let plain1 = yield openAndLoadWindow();
  let private1 = yield openAndLoadWindow({private: true});
  CustomizableUI.createWidget({
    id: kWidgetId,
    removable: true,
    showInPrivateBrowsing: false
  });
  CustomizableUI.addWidgetToArea(kWidgetId,
                                 CustomizableUI.AREA_NAVBAR);
  assertWidgetExists(plain1, true);
  assertWidgetExists(private1, false);

  
  
  let plain2 = yield openAndLoadWindow();
  let private2 = yield openAndLoadWindow({private: true});
  assertWidgetExists(plain2, true);
  assertWidgetExists(private2, false);

  
  
  CustomizableUI.addWidgetToArea(kWidgetId,
                                 CustomizableUI.AREA_TABSTRIP);
  assertWidgetExists(plain1, true);
  assertWidgetExists(plain2, true);
  assertWidgetExists(private1, false);
  assertWidgetExists(private2, false);

  
  CustomizableUI.moveWidgetWithinArea(kWidgetId, 0);
  assertWidgetExists(plain1, true);
  assertWidgetExists(plain2, true);
  assertWidgetExists(private1, false);
  assertWidgetExists(private2, false);

  CustomizableUI.removeWidgetFromArea("some-widget");
  assertWidgetExists(plain1, false);
  assertWidgetExists(plain2, false);
  assertWidgetExists(private1, false);
  assertWidgetExists(private2, false);

  plain1.close();
  plain2.close();
  private1.close();
  private2.close();

  CustomizableUI.destroyWidget("some-widget");
});




add_task(function() {
  let plain1 = yield openAndLoadWindow();
  let private1 = yield openAndLoadWindow({private: true});

  CustomizableUI.createWidget({
    id: kWidgetId,
    removable: true,
    showInPrivateBrowsing: true
  });
  CustomizableUI.addWidgetToArea(kWidgetId,
                                 CustomizableUI.AREA_NAVBAR);
  assertWidgetExists(plain1, true);
  assertWidgetExists(private1, true);

  
  
  let plain2 = yield openAndLoadWindow();
  let private2 = yield openAndLoadWindow({private: true});

  assertWidgetExists(plain2, true);
  assertWidgetExists(private2, true);

  
  
  CustomizableUI.addWidgetToArea(kWidgetId,
                                 CustomizableUI.AREA_TABSTRIP);
  assertWidgetExists(plain1, true);
  assertWidgetExists(plain2, true);
  assertWidgetExists(private1, true);
  assertWidgetExists(private2, true);

  
  CustomizableUI.moveWidgetWithinArea(kWidgetId, 0);
  assertWidgetExists(plain1, true);
  assertWidgetExists(plain2, true);
  assertWidgetExists(private1, true);
  assertWidgetExists(private2, true);

  CustomizableUI.removeWidgetFromArea("some-widget");
  assertWidgetExists(plain1, false);
  assertWidgetExists(plain2, false);
  assertWidgetExists(private1, false);
  assertWidgetExists(private2, false);

  plain1.close();
  plain2.close();
  private1.close();
  private2.close();

  CustomizableUI.destroyWidget("some-widget");
});

add_task(function asyncCleanup() {
  yield resetCustomization();
});
