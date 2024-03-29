



function getExampleDotComURL(conformanceDirRelativePath) {
  var prefix = 'http://example.com/tests/dom/canvas/test/webgl-conformance/';
  return prefix + conformanceDirRelativePath;
}

function setExampleDotComImage(imageElemID, imagePath, relPathToBase, onLoadCallback) {
  var img = document.getElementById(imageElemID);
  if (!img)
    throw 'Bad `imageElemID`: ' + imageElemID;

  if (onLoadCallback)
    img.onload = onLoadCallback;

  img.onerror = function() {
    console.log('Failed to load image from: ' + img.src);
    console.log('Loading same-domain backup image.');

    img.onerror = function() {
      console.log('Failed to load backup image from: ' + img.src);
      return;
    };

    img.src = relPathToBase + imagePath;
  };

  img.src = getExampleDotComURL(imagePath);
}
