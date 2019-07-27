


onnotificationclick = function(e) {
  self.clients.matchAll().then(function(clients) {
    if (clients.length === 0) {
      dump("********************* CLIENTS LIST EMPTY! Test will timeout! ***********************\n");
      return;
    }

    clients.forEach(function(client) {
      client.postMessage("done");
    });
  });
}
