
















var matchedElement = null;
var originalBgColor = null;
var active = false;

function resetMatchedElement() {
  if (matchedElement) {
    matchedElement.css('background-color', originalBgColor);
    matchedElement.unbind('click.annotator');
  }
}

self.on('message', function onMessage(activation) {
  active = activation;
  if (!active) {
    resetMatchedElement();
  }
});

function getInnerText(element) {
  
  var list = [];
  element.find("*").andSelf().contents()
    .filter(function () {
      return this.nodeType == 3 && this.parentNode.tagName != "SCRIPT";
    })
    .each(function () {
      list.push(this.nodeValue);
    });
  return list.join("");
}

$('*').mouseenter(function() {
  if (!active || $(this).hasClass('annotated')) {
    return;
  }
  resetMatchedElement();
  ancestor = $(this).closest("[id]");
  matchedElement = $(this).first();
  originalBgColor = matchedElement.css('background-color');
  matchedElement.css('background-color', 'yellow');
  matchedElement.bind('click.annotator', function(event) {
    event.stopPropagation();
    event.preventDefault();
    self.port.emit('show',
      [
        document.location.toString(),
        ancestor.attr("id"),
        getInnerText(matchedElement)
      ]
   );
  });
});

$('*').mouseout(function() {
  resetMatchedElement();
});
