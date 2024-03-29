






var dialog;


var printProgress = null; 


var targetFile;

var docTitle = "";
var docURL   = "";
var progressParams = null;
var switchUI = true;

function ellipseString(aStr, doFront)
{
  if (aStr.length > 3 && (aStr.substr(0, 3) == "..." || aStr.substr(aStr.length-4, 3) == "...")) {
    return aStr;
  }

  var fixedLen = 64;
  if (aStr.length > fixedLen) {
    if (doFront) {
      var endStr = aStr.substr(aStr.length-fixedLen, fixedLen);
      var str = "..." + endStr;
      return str;
    } else {
      var frontStr = aStr.substr(0, fixedLen);
      var str = frontStr + "...";
      return str;
    }
  }
  return aStr;
}


var progressListener = {
    onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
    {
      if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_START)
      {
        
        
        dialog.progress.setAttribute( "mode", "undetermined" );
      }
      
      if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
      {
        
        
        var msg = getString( "printComplete" );
        dialog.title.setAttribute("value", msg);

        
        dialog.progress.setAttribute( "value", 100 );
        dialog.progress.setAttribute( "mode", "normal" );
        var percentPrint = getString( "progressText" );
        percentPrint = replaceInsert( percentPrint, 1, 100 );
        dialog.progressText.setAttribute("value", percentPrint);

        var fm = Components.classes["@mozilla.org/focus-manager;1"]
                     .getService(Components.interfaces.nsIFocusManager);
        if (fm && fm.activeWindow == window) {
          
          
          
          
          
          
          
          
          
          
          
          
          
          opener.focus();
        }

        window.close();
      }
    },
    
    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress)
    {
      if (switchUI) 
      {
        dialog.tempLabel.setAttribute("hidden", "true");
        dialog.progress.setAttribute("hidden", "false");
        dialog.cancel.setAttribute("disabled", "false");

        var progressLabel = getString("progress");
        if (progressLabel == "") {
          progressLabel = "Progress:"; 
        }
        switchUI = false;
      }

      if (progressParams)
      {
        var docTitleStr = ellipseString(progressParams.docTitle, false);
        if (docTitleStr != docTitle) {
          docTitle = docTitleStr;
          dialog.title.value = docTitle;
        }
        var docURLStr = progressParams.docURL;
        if (docURLStr != docURL && dialog.title != null) {
          docURL = docURLStr;
          if (docTitle == "") {
            dialog.title.value = ellipseString(docURLStr, true);
          }
        }
      }

      
      var percent;
      if ( aMaxTotalProgress > 0 ) 
      {
        percent = Math.round( (aCurTotalProgress*100)/aMaxTotalProgress );
        if ( percent > 100 )
          percent = 100;
        
        dialog.progress.removeAttribute( "mode");
        
        
        dialog.progress.setAttribute( "value", percent );

        
        var percentPrint = getString( "progressText" );
        percentPrint = replaceInsert( percentPrint, 1, percent );
        dialog.progressText.setAttribute("value", percentPrint);
      } 
      else 
      {
        
        dialog.progress.setAttribute( "mode", "undetermined" );
        
        dialog.progressText.setAttribute("value", "");
      }
    },

	  onLocationChange: function(aWebProgress, aRequest, aLocation, aFlags)
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
     if (iid.equals(Components.interfaces.nsIWebProgressListener) || iid.equals(Components.interfaces.nsISupportsWeakReference))
      return this;
     
     throw Components.results.NS_NOINTERFACE;
    }
};

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

function loadDialog() 
{
}

function replaceInsert( text, index, value ) {
   var result = text;
   var regExp = new RegExp( "#"+index );
   result = result.replace( regExp, value );
   return result;
}

function onLoad() {

    
    printProgress = window.arguments[0];
    if (window.arguments[1])
    {
      progressParams = window.arguments[1].QueryInterface(Components.interfaces.nsIPrintProgressParams)
      if (progressParams)
      {
        docTitle = ellipseString(progressParams.docTitle, false);
        docURL   = ellipseString(progressParams.docURL, true);
      }
    }

    if ( !printProgress ) {
        dump( "Invalid argument to printProgress.xul\n" );
        window.close()
        return;
    }

    dialog = new Object;
    dialog.strings = new Array;
    dialog.title        = document.getElementById("dialog.title");
    dialog.titleLabel   = document.getElementById("dialog.titleLabel");
    dialog.progress     = document.getElementById("dialog.progress");
    dialog.progressText = document.getElementById("dialog.progressText");
    dialog.progressLabel = document.getElementById("dialog.progressLabel");
    dialog.tempLabel    = document.getElementById("dialog.tempLabel");
    dialog.cancel       = document.getElementById("cancel");

    dialog.progress.setAttribute("hidden", "true");
    dialog.cancel.setAttribute("disabled", "true");

    var progressLabel = getString("preparing");
    if (progressLabel == "") {
      progressLabel = "Preparing..."; 
    }
    dialog.tempLabel.value = progressLabel;

    dialog.title.value = docTitle;

    
    var object = this;
    doSetOKCancel("", function () { return object.onCancel();});

    
    loadDialog();

    
    printProgress.registerListener(progressListener);
    moveToAlertPosition();
    
    window.setTimeout(doneIniting, 500);
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
