




































const NS_XUL  = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";

var gUpdateHistory = {
  _view: null,
  
  


  onLoad: function() {
    this._view = document.getElementById("historyItems");
    
    var um = 
        Components.classes["@mozilla.org/updates/update-manager;1"].
        getService(Components.interfaces.nsIUpdateManager);
    var uc = um.updateCount;
    if (uc) {
      while (this._view.hasChildNodes())
        this._view.removeChild(this._view.firstChild);
    
      var bundle = document.getElementById("updateBundle");
      
      for (var i = 0; i < uc; ++i) {
        var update = um.getUpdateAt(i);
        if (!update || !update.name)
          continue;
        
        var element = document.createElementNS(NS_XUL, "update");
        this._view.appendChild(element);
        element.name = bundle.getFormattedString("updateFullName", 
          [update.name, update.buildID]);
        element.type = bundle.getString("updateType_" + update.type);
        element.installDate = this._formatDate(update.installDate);
        element.detailsURL = update.detailsURL;
        element.status = update.statusText;
      }
    }
    var cancelbutton = document.documentElement.getButton("cancel");
    cancelbutton.focus();
  },
  
  





  _formatDate: function(seconds) {
    var sdf = 
        Components.classes["@mozilla.org/intl/scriptabledateformat;1"].
        getService(Components.interfaces.nsIScriptableDateFormat);
    var date = new Date(seconds);
    return sdf.FormatDateTime("", sdf.dateFormatLong, 
                              sdf.timeFormatSeconds,
                              date.getFullYear(),
                              date.getMonth() + 1,
                              date.getDate(),
                              date.getHours(),
                              date.getMinutes(),
                              date.getSeconds());
  }
};

