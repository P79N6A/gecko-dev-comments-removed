









function addDiv(t) {
  var div = document.createElement('div');
  document.body.appendChild(div);
  if (t && typeof t.add_cleanup === 'function') {
    t.add_cleanup(function() { div.remove(); });
  }
  return div;
}




function waitForFrame() {
  return new Promise(function(resolve, reject) {
    window.requestAnimationFrame(resolve);
  });
}
