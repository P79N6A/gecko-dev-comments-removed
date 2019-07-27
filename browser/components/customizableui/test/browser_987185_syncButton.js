



"use strict";

let syncService = {};
Components.utils.import("resource://services-sync/service.js", syncService);

let needsSetup;
let originalSync;
let service = syncService.Service;
let syncWasCalled = false;

add_task(function* testSyncButtonFunctionality() {
  info("Check Sync button functionality");
  storeInitialValues();
  mockFunctions();

  
  CustomizableUI.addWidgetToArea("sync-button", CustomizableUI.AREA_PANEL);

  
  yield PanelUI.show();
  info("The panel menu was opened");

  let syncButton = document.getElementById("sync-button");
  ok(syncButton, "The Sync button was added to the Panel Menu");
  syncButton.click();
  info("The sync button was clicked");

  yield waitForCondition(() => syncWasCalled);
});

add_task(function* asyncCleanup() {
  
  yield resetCustomization();
  ok(CustomizableUI.inDefaultState, "The panel UI is in default state again.");

  if (isPanelUIOpen()) {
    let panelHidePromise = promisePanelHidden(window);
    PanelUI.hide();
    yield panelHidePromise;
  }

  restoreValues();
});

function mockFunctions() {
  
  gSyncUI._needsSetup = function() false;

  
  service.errorHandler.syncAndReportErrors = mocked_syncAndReportErrors;
}

function mocked_syncAndReportErrors() {
  syncWasCalled = true;
}

function restoreValues() {
  gSyncUI._needsSetup = needsSetup;
  service.sync = originalSync;
}

function storeInitialValues() {
  needsSetup = gSyncUI._needsSetup;
  originalSync = service.sync;
}
