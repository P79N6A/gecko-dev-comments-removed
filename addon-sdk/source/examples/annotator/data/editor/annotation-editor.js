








var textArea = document.getElementById('annotation-box');

textArea.onkeyup = function(event) {
  if (event.keyCode == 13) {
    self.postMessage(textArea.value);
    textArea.value = '';
  }
};

self.on('message', function() {
  var textArea = document.getElementById('annotation-box');
  textArea.value = '';
  textArea.focus();
});
