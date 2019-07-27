





"use strict";

function checkUrlbarFocus(win) {
  let urlbar = win.gURLBar;
  is(win.document.activeElement, urlbar.inputField, "URL Bar should be focused");
  is(urlbar.value, "", "URL Bar should be empty");
}

function openNewPrivateWindow() {
  let deferred = Promise.defer();
  whenNewWindowLoaded({private: true}, win => {
    executeSoon(() => deferred.resolve(win));
  });
  return deferred.promise;
}

add_task(function* () {
  let win = yield openNewPrivateWindow();
  checkUrlbarFocus(win);
  win.close();
});
