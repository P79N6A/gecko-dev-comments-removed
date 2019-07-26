




onmessage = function(event) {
  throw "No messages should reach me!";
}

var xhr = new XMLHttpRequest();
xhr.open("GET", "testXHR.txt", false);
xhr.addEventListener("loadstart", function ()
{
  
  postMessage("TERMINATE");
  
  while(1) { true; }
});
xhr.send(null);
