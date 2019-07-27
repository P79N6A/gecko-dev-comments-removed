


onmessage = function(e) {
  self.update();
  clients.getServiced().then(function(c) {
    if (c.length == 0) {
      
      return;
    }

    c[0].postMessage('FINISH');
  });
}
