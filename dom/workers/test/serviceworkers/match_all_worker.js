function loop() {
  self.clients.matchAll().then(function(result) {
    setTimeout(loop, 0);
  });
}

onactivate = function(e) {
  
  loop();
}
