





































var mozmillInit = {}; Components.utils.import('resource://mozmill/modules/init.js', mozmillInit);

var MozMill = {
  onLoad: function() {
    
    this.initialized = true;
  },

  onMenuItemCommand: function() {
    var utils = {}; Components.utils.import('resource://mozmill/modules/utils.js', utils);
    var mmWindows = utils.getWindowByTitle('MozMill IDE');
    if (!mmWindows){
      var height = utils.getPreference("mozmill.height", 740);
      var width = utils.getPreference("mozmill.width", 635);
      
      var left = utils.getPreference("mozmill.screenX", 0);
      var top = utils.getPreference("mozmill.screenY", 0);

      if (left == 0){
        
        var width = window.screen.availWidth/2.5;
        var height = window.screen.availHeight;
        window.resizeTo((window.screen.availWidth - width), window.screen.availHeight);

        var height = window.innerHeight;
        var left = window.innerWidth;
      }
      
      var paramString = "chrome,resizable,height=" + height +
                               ",width=" + width + ",left="+left+",top="+top;
      var w = window.open("chrome://mozmill/content/mozmill.xul", "", paramString);
    } else { mmWindows[0].focus(); }
  }
};

window.addEventListener("load", function(e) { MozMill.onLoad(e); }, false);

 
function mozMillTestWindow() {
  window.openDialog("chrome://mozmill/content/testwindow.html", "_blank", "chrome,dialog=no, resizable");
}







