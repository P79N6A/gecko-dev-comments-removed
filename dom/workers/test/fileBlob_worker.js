


onmessage = function(event) {
  var file = event.data;

  var rtnObj = new Object();

  rtnObj.size = file.size;
  rtnObj.type = file.type;

  postMessage(rtnObj);
};
