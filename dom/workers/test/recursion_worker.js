





onerror = function(event) {
  postMessage(event.message);
};


function recurse() {
  recurse();
}


function recurse2() {
  var xhr = new XMLHttpRequest();
  xhr.onreadystatechange = function() {
    xhr.open("GET", "nonexistent.file");
  }
  xhr.open("GET", "nonexistent.file");
}

var messageCount = 0;
onmessage = function(event) {
  switch (++messageCount) {
    case 2:
      recurse2();

      
      
      
      postMessage("Done");
      return;

    case 1:
      recurse();
      throw "Exception should have prevented us from getting here!";

    default:
      throw "Weird number of messages: " + messageCount;
  }

  throw "Impossible to get here!";
}
