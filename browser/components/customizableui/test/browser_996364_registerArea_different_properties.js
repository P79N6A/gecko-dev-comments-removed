



"use strict";



add_task(function() {
  try {
    CustomizableUI.registerArea("area-996364", {});
    CustomizableUI.registerArea("area-996364", {});
  } catch (ex) {
    ok(false, ex.message);
  }

  CustomizableUI.unregisterArea("area-996364", true);
});

add_task(function() {
  let exceptionThrown = false;
  try {
    CustomizableUI.registerArea("area-996364-2", {type: CustomizableUI.TYPE_TOOLBAR, defaultCollapsed: "false"});
  } catch (ex) {
    exceptionThrown = true;
  }
  ok(exceptionThrown, "defaultCollapsed is not allowed as an external property");

  
});

add_task(function() {
  let exceptionThrown;
  try {
    CustomizableUI.registerArea("area-996364-3", {type: CustomizableUI.TYPE_TOOLBAR});
    CustomizableUI.registerArea("area-996364-3", {type: CustomizableUI.TYPE_MENU_PANEL});
  } catch (ex) {
    exceptionThrown = ex;
  }
  ok(exceptionThrown, "Exception expected, an area cannot change types: " + (exceptionThrown ? exceptionThrown : "[no exception]"));

  CustomizableUI.unregisterArea("area-996364-3", true);
});

add_task(function() {
  let exceptionThrown;
  try {
    CustomizableUI.registerArea("area-996364-4", {type: CustomizableUI.TYPE_MENU_PANEL});
    CustomizableUI.registerArea("area-996364-4", {type: CustomizableUI.TYPE_TOOLBAR});
  } catch (ex) {
    exceptionThrown = ex;
  }
  ok(exceptionThrown, "Exception expected, an area cannot change types: " + (exceptionThrown ? exceptionThrown : "[no exception]"));

  CustomizableUI.unregisterArea("area-996364-4", true);
});

add_task(function() {
  let exceptionThrown;
  try {
    CustomizableUI.registerArea("area-996899-1", { anchor: "PanelUI-menu-button",
                                                   type: CustomizableUI.TYPE_MENU_PANEL,
                                                   defaultPlacements: [] });
    CustomizableUI.registerArea("area-996899-1", { anchor: "home-button",
                                                   type: CustomizableUI.TYPE_MENU_PANEL,
                                                   defaultPlacements: [] });
  } catch (ex) {
    exceptionThrown = ex;
  }
  ok(!exceptionThrown, "Changing anchors shouldn't throw an exception: " + (exceptionThrown ? exceptionThrown : "[no exception]"));
  CustomizableUI.unregisterArea("area-996899-1", true);
});

add_task(function() {
  let exceptionThrown;
  try {
    CustomizableUI.registerArea("area-996899-2", { anchor: "PanelUI-menu-button",
                                                   type: CustomizableUI.TYPE_MENU_PANEL,
                                                   defaultPlacements: [] });
    CustomizableUI.registerArea("area-996899-2", { anchor: "PanelUI-menu-button",
                                                   type: CustomizableUI.TYPE_MENU_PANEL,
                                                   defaultPlacements: ["feed-button"] });
  } catch (ex) {
    exceptionThrown = ex;
  }
  ok(!exceptionThrown, "Changing defaultPlacements shouldn't throw an exception: " + (exceptionThrown ? exceptionThrown : "[no exception]"));
  CustomizableUI.unregisterArea("area-996899-2", true);
});

add_task(function() {
  let exceptionThrown;
  try {
    CustomizableUI.registerArea("area-996899-3", { legacy: true });
    CustomizableUI.registerArea("area-996899-3", { legacy: false });
  } catch (ex) {
    exceptionThrown = ex;
  }
  ok(exceptionThrown, "Changing 'legacy' should throw an exception: " + (exceptionThrown ? exceptionThrown : "[no exception]"));
  CustomizableUI.unregisterArea("area-996899-3", true);
});

add_task(function() {
  let exceptionThrown;
  try {
    CustomizableUI.registerArea("area-996899-4", { overflowable: true });
    CustomizableUI.registerArea("area-996899-4", { overflowable: false });
  } catch (ex) {
    exceptionThrown = ex;
  }
  ok(exceptionThrown, "Changing 'overflowable' should throw an exception: " + (exceptionThrown ? exceptionThrown : "[no exception]"));
  CustomizableUI.unregisterArea("area-996899-4", true);
});
