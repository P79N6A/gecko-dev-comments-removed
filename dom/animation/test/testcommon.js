









function addDiv(t) {
  var div = document.createElement('div');
  document.body.appendChild(div);
  if (t && typeof t.add_cleanup === 'function') {
    t.add_cleanup(function() {
      if (div.parentNode) {
        div.parentNode.removeChild(div);
      }
    });
  }
  return div;
}




function waitForFrame() {
  return new Promise(function(resolve, reject) {
    window.requestAnimationFrame(resolve);
  });
}






function waitForAllPlayers(players) {
  return Promise.all(players.map(function(player) { return player.ready; }));
}
