


onmessage = function(e) {
  self.registration.update();
  clients.matchAll().then(function(c) {
    if (c.length == 0) {
      
      return;
    }

    c[0].postMessage('FINISH');
  });
}
