

















































var contentAreaDNDObserver = {
  onDrop: function (aEvent, aXferData, aDragSession)
    {
      var url = transferUtils.retrieveURLFromData(aXferData.data, aXferData.flavour.contentType);

      
      
      
      if (!url || !url.length || url.indexOf(" ", 0) != -1 ||
          /^\s*(javascript|data):/.test(url))
        return;

      switch (document.documentElement.getAttribute('windowtype')) {
        case "navigator:browser":
          
          nsDragAndDrop.dragDropSecurityCheck(aEvent, aDragSession, url);

          loadURI(getShortcutOrURI(url));
          break;
        case "navigator:view-source":
          viewSource(url);
          break;
      }
      
      
      
      aEvent.preventDefault();
    },

  getSupportedFlavours: function ()
    {
      var flavourSet = new FlavourSet();
      flavourSet.appendFlavour("text/x-moz-url");
      flavourSet.appendFlavour("text/unicode");
      flavourSet.appendFlavour("application/x-moz-file", "nsIFile");
      return flavourSet;
    }
  
};
