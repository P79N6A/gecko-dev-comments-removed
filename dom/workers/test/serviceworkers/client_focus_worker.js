onmessage = function(e) {
  if (!e.source) {
    dump("ERROR: message doesn't have a source.");
  }

  
  if (e.source instanceof WindowClient) {
    
    e.source.focus().then(function(client) {
      client.postMessage(client.focused);
    });
  } else {
    dump("ERROR: client should be a WindowClient");
  }
};
