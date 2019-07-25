




onmessage = function(event) {
  var blob = event.data.blob;
  var start = event.data.start;
  var end = event.data.end;
  var contentType = event.data.contentType;

  var slicedBlob;
  if (contentType == undefined && end == undefined) {
    slicedBlob = blob.mozSlice(start);
  } else if (contentType == undefined) {
    slicedBlob = blob.mozSlice(start, end);
  } else {
    slicedBlob = blob.mozSlice(start, end, contentType);
  }

  var rtnObj = new Object();

  rtnObj.size = slicedBlob.size;
  rtnObj.type = slicedBlob.type;

  postMessage(rtnObj);
};
