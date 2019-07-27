




onactivate = function(e) {
  e.waitUntil(new Promise(function(resolve, reject) {
    registration.onupdatefound = function(e) {
      dump("Update found for scope " + registration.scope + "\n");
      clients.matchAll().then(function(clients) {
        if (!clients.length) {
          dump("No clients found\n");
          reject("No clients found");
        }

        if (registration.scope.match(/updatefoundevent\.html$/)) {
          clients[0].postMessage("finish");
          resolve();
        } else {
          dump("Scope did not match");
        }
      }, reject);
    }
  }));
}
