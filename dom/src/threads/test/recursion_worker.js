onerror = function(event) {
  
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
  postMessage("Done");
}

var count = 0;
onmessage = function(event) {
  switch (++count) {
    case 1:
      recurse();
      break;
    case 2:
      recurse2();
      
      
      return;
    default:
  }
  throw "Never should have gotten here!";
}
