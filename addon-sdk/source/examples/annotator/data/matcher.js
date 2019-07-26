




















self.on('message', function onMessage(annotations) {
  annotations.forEach(
    function(annotation) {
      if(annotation.url == document.location.toString()) {
        createAnchor(annotation);
      }
  });

  $('.annotated').css('border', 'solid 3px yellow');

  $('.annotated').bind('mouseenter', function(event) {
    self.port.emit('show', $(this).attr('annotation'));
    event.stopPropagation();
    event.preventDefault();
  });

  $('.annotated').bind('mouseleave', function() {
    self.port.emit('hide');
  });
});


function createAnchor(annotation) {
  annotationAnchorAncestor = $('#' + annotation.ancestorId)[0] || document.body;
  annotationAnchor = $(annotationAnchorAncestor).parent().find(
                     ':contains("' + annotation.anchorText + '")').last();
  annotationAnchor.addClass('annotated');
  annotationAnchor.attr('annotation', annotation.annotationText);
}
