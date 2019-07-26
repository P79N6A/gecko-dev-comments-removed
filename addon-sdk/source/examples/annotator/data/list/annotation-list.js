










self.on("message", function onMessage(storedAnnotations) {
  var annotationList = $('#annotation-list');
  annotationList.empty();
  storedAnnotations.forEach(
    function(storedAnnotation) {
      var annotationHtml = $('#template .annotation-details').clone();
      annotationHtml.find('.url').text(storedAnnotation.url)
                                 .attr('href', storedAnnotation.url);
      annotationHtml.find('.url').bind('click', function(event) {
        event.stopPropagation();
        event.preventDefault();
        self.postMessage(storedAnnotation.url);
      });
      annotationHtml.find('.selection-text')
                    .text(storedAnnotation.anchorText);
      annotationHtml.find('.annotation-text')
                    .text(storedAnnotation.annotationText);
      annotationList.append(annotationHtml);
    });
});
