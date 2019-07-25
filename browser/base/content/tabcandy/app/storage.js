
Storage = {
  init: function() {
    var file = Components.classes["@mozilla.org/file/directory_service;1"].  
      getService(Components.interfaces.nsIProperties).  
      get("ProfD", Components.interfaces.nsIFile);  


    Utils.log(file); 
  }
};

Storage.init();

