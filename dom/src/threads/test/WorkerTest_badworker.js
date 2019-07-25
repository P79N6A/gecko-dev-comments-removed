




































function onmessage(event) {
  var xhr = new XMLHttpRequest();
  xhr.open("GET", event.data, false);

  try {
    xhr.send();
  }
  catch (e) {
    
    if (e.result != 2152924148) {
      throw e;
    }
  }

  if (xhr.responseText) {
    throw "Shouldn't be able to xhr to a chrome URL!";
  }

  xhr = new XMLHttpRequest();

  xhr.onload = function(event) {
    postMessage(xhr.responseText);
  };

  xhr.open("GET", "testXHR.txt");
  xhr.send();
}
