function loop() {
  self.clients.getServiced().then(function(result) {
    setTimeout(loop, 0);
  });
}

onactivate = function(e) {
  
  loop();
}
