


"use strict";



loadHelperScript("helper_disable_cache.js");

add_task(function*() {
  
  registerCleanupFunction(() => {
    info("Resetting devtools.cache.disabled to false.");
    Services.prefs.setBoolPref("devtools.cache.disabled", false);
  });

  
  for (let tab of tabs) {
    yield initTab(tab, tab.startToolbox);
  }

  
  yield setDisableCacheCheckboxChecked(tabs[0], true);

  
  tabs[2].toolbox = yield gDevTools.showToolbox(tabs[2].target, "options");
  yield checkCacheEnabled(tabs[2], false);

  
  yield tabs[2].toolbox.destroy();
  tabs[2].target = TargetFactory.forTab(tabs[2].tab);
  yield checkCacheEnabled(tabs[2], true);

  
  tabs[2].toolbox = yield gDevTools.showToolbox(tabs[2].target, "options");
  yield checkCacheEnabled(tabs[2], false);

  
  yield setDisableCacheCheckboxChecked(tabs[2], false);
  yield checkCacheStateForAllTabs([true, true, true, true]);

  yield finishUp();
});
