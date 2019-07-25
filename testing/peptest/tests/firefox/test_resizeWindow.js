









































let window = pep.getWindow();
let width = window.outerWidth;
let height = window.outerHeight;

pep.performAction('resize_by', function() {
  window.resizeBy(100, 100);
});

pep.performAction('resize_to', function() {
  window.resizeTo(800, 600);
});


window.resizeTo(width, height);
