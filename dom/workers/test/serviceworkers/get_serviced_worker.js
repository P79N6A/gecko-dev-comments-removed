function loop() {
  self.clients.getServiced().then(function(result) {
    setTimeout(loop, 0);
  });
}

onactivate = function(e) {
  
  loop();
}

onclose = function(e) {
  for (var i = 0; i < 100; ++i) {
    self.clients.getServiced();
  }
}
