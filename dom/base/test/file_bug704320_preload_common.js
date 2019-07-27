

var loadCount = 0;




function incrementLoad(tag) {
  loadCount++;
  if (loadCount == 3) {
    window.parent.postMessage("childLoadComplete", window.location.origin);
  } else if (loadCount > 3) {
    document.write("<h1>Too Many Load Events!</h1>");
    window.parent.postMessage("childOverload", window.location.origin);
  }
}


function postfail(msg) {
  window.parent.postMessage("fail-" + msg, window.location.origin);
}


