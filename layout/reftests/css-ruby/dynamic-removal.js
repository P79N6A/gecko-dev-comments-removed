function getElements(className) {
  return Array.from(document.getElementsByClassName(className));
}
window.onload = function() {
  
  document.body.clientWidth;

  getElements('remove').forEach(function(e) {
    e.parentNode.removeChild(e);
  });
  getElements('remove-after').forEach(function(e) {
    e.parentNode.removeChild(e.nextSibling);
  });
};
