



onclose = function() {
  var xhr = new XMLHttpRequest();
  xhr.open("POST", "closeOnGC_server.sjs" + location.search, false);
  xhr.send();
};

postMessage("ready");
