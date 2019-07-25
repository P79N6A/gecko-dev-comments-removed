



































addEventListener("DOMWillOpenModalDialog", function (event) {
  
  
  if (event.isTrusted) {
    
    
    
    sendSyncMessage("Panorama:DOMWillOpenModalDialog");
  }
}, true);
