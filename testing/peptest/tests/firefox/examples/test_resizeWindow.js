









































let window = getWindow();
let width = window.outerWidth;
let height = window.outerHeight;

performAction('resize_by', function() {
  window.resizeBy(100, 100);
});

performAction('resize_to', function() {
  window.resizeTo(800, 600);
});


window.resizeTo(width, height);
