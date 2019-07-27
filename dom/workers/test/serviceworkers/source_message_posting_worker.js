onmessage = function(e) {
  if (!e.source) {
    dump("ERROR: message doesn't have a source.");
  }

  
  if (e.source instanceof  WindowClient) {
    e.source.postMessage(e.data);
  } else {
    e.source.postMessage("ERROR. source is not a window client.");
  }
};
