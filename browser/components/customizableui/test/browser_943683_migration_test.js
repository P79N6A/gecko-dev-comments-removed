



"use strict";

const kWidgetId = "test-addonbar-migration";
const kWidgetId2 = "test-addonbar-migration2";

let addonbar = document.getElementById(CustomizableUI.AREA_ADDONBAR);
let navbar = document.getElementById(CustomizableUI.AREA_NAVBAR);

let btn;
let btn2;


add_task(function() {
  btn = createDummyXULButton(kWidgetId, "Test");
  btn2 = createDummyXULButton(kWidgetId2, "Test2");
  addonbar.insertItem(btn.id);
  ok(btn.parentNode == navbar.customizationTarget, "Button should end up in navbar");
  let migrationArray = addonbar.getMigratedItems();
  is(migrationArray.length, 1, "Should have migrated 1 item");
  is(migrationArray[0], kWidgetId, "Should have migrated our 1 item");

  addonbar.currentSet = addonbar.currentSet + "," + kWidgetId2;
  ok(btn2.parentNode == navbar.customizationTarget, "Second button should end up in the navbar");
  migrationArray = addonbar.getMigratedItems();
  is(migrationArray.length, 2, "Should have migrated 2 items");
  isnot(migrationArray.indexOf(kWidgetId2), -1, "Should have migrated our second item");

  let otherWindow = yield openAndLoadWindow(undefined, true);
  try {
    let addonBar = otherWindow.document.getElementById("addon-bar");
    let otherMigrationArray = addonBar.getMigratedItems();
    is(migrationArray.length, otherMigrationArray.length,
       "Other window should have the same number of migrated items.");
    if (migrationArray.length == otherMigrationArray.length) {
      for (let widget of migrationArray) {
        isnot(otherMigrationArray.indexOf(widget), -1,
              "Migrated widget " + widget + " should also be listed as migrated in the other window.");
      }
    }
  } finally {
    yield promiseWindowClosed(otherWindow);
  }
  btn.remove();
  btn2.remove();
  CustomizableUI.reset();
});
