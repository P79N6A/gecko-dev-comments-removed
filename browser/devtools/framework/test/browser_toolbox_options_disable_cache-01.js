


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

  
  yield checkCacheStateForAllTabs([true, true, true, true]);

  
  yield setDisableCacheCheckboxChecked(tabs[0], true);
  yield checkCacheStateForAllTabs([false, false, true, true]);

  yield finishUp();
});
