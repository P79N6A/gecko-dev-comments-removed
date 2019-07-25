



var xhr = new XMLHttpRequest();
xhr.open("GET", "testXHR.txt");
xhr.send(null);
xhr.open("GET", "testXHR.txt");
xhr.send(null);
postMessage("done");
