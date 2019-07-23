







































var dialog;


var printProgress = null; 


var targetFile;

var progressParams = null;


var progressListener = {
    onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
    {
      
      if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
      {
        window.close();
      }
    },
    
    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
    {
      if (progressParams)
      {
        dialog.title.crop = progressParams.docTitle ? "end" : "center";
        dialog.title.value = progressParams.docTitle || progressParams.docURL;
      }
    },

	  onLocationChange: function(aWebProgress, aRequest, aLocation)
    {
      
    },

    onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage)
    {
      if (aMessage != "")
        dialog.title.setAttribute("value", aMessage);
    },

    onSecurityChange: function(aWebProgress, aRequest, state)
    {
      
    },

    QueryInterface : function(iid)
    {
     if (iid.equals(Components.interfaces.nsIWebProgressListener) || 
         iid.equals(Components.interfaces.nsISupportsWeakReference) ||
         iid.equals(Components.interfaces.nsISupports))
      return this;
     
     throw Components.results.NS_NOINTERFACE;
    }
};

function onLoad() {
    
    printProgress = window.arguments[0];

    if ( !printProgress ) {
        dump( "Invalid argument to printPreviewProgress.xul\n" );
        window.close()
        return;
    }

    dialog          = new Object;
    dialog.strings  = new Array;
    dialog.title      = document.getElementById("dialog.title");
    dialog.titleLabel = document.getElementById("dialog.titleLabel");

    if (window.arguments[1]) {
      progressParams = window.arguments[1].QueryInterface(Components.interfaces.nsIPrintProgressParams)
      if (progressParams) {
        dialog.title.crop = progressParams.docTitle ? "end" : "center";
        dialog.title.value = progressParams.docTitle || progressParams.docURL;
      }
    }

    
    printProgress.registerListener(progressListener);
    moveToAlertPosition();

    
    window.setTimeout(doneIniting, 100);
}

function onUnload() 
{
  if (printProgress)
  {
   try 
   {
     printProgress.unregisterListener(progressListener);
     printProgress = null;
   }
    
   catch( exception ) {}
  }
}

function getString( stringId ) {
   
   if (!(stringId in dialog.strings)) {
      
      var elem = document.getElementById( "dialog.strings."+stringId );
      try {
        if ( elem
           &&
           elem.childNodes
           &&
           elem.childNodes[0]
           &&
           elem.childNodes[0].nodeValue ) {
         dialog.strings[ stringId ] = elem.childNodes[0].nodeValue;
        } else {
          
          dialog.strings[ stringId ] = "";
        }
      } catch (e) { dialog.strings[ stringId ] = ""; }
   }
   return dialog.strings[ stringId ];
}


function onCancel () 
{
  
   try 
   {
     printProgress.processCanceledByUser = true;
   }
   catch( exception ) {return true;}
    
  
  return false;
}

function doneIniting() 
{
  
  printProgress.doneIniting();
}
