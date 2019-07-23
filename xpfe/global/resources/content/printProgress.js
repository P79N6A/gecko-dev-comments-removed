











































const TITLE_COMPLETE_DECK = 1;
const PROGRESS_METER_DECK = 1;



var dialog;                 
var percentFormat;          
var printProgress  = null;  
var progressParams = null;  
var switchUI       = true;  



var progressListener =
{
  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus)
  {
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_START)
    {
      
      setProgressPercentage(-1);
    }
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
    {
      
      dialog.titleDeck.selectedIndex = TITLE_COMPLETE_DECK;
      setProgressPercentage(100);
      window.close();
    }
  },

  onProgressChange: function(aWebProgress,      aRequest,
                             aCurSelfProgress,  aMaxSelfProgress,
                             aCurTotalProgress, aMaxTotalProgress)
  {
    if (switchUI)
    {
      
      dialog.progressDeck.selectedIndex = PROGRESS_METER_DECK;
      dialog.cancel.removeAttribute("disabled");
      switchUI = false;
    }
    setProgressTitle();

    
    if (aMaxTotalProgress > 0)
    {
      var percentage = Math.round(aCurTotalProgress * 100 / aMaxTotalProgress);
      setProgressPercentage(percentage);
    }
    else
    {
      
      setProgressPercentage(-1);
    }
  },

  onLocationChange: function(aWebProgress, aRequest, aLocation)
  {
    
  },

  onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage)
  {
    if (aMessage)
      dialog.title.value = aMessage;
  },

  onSecurityChange: function(aWebProgress, aRequest, aStatus)
  {
    
  },

  QueryInterface : function(iid)
  {
    if (iid.equals(Components.interfaces.nsIWebProgressListener) || iid.equals(Components.interfaces.nsISupportsWeakReference))
      return this;
    throw Components.results.NS_NOINTERFACE;
  }
};


function setProgressTitle()
{
  if (progressParams)
  {
    dialog.title.crop  = progressParams.docTitle ? "end" : "center";
    dialog.title.value = progressParams.docTitle || progressParams.docURL;
  }
}


function setProgressPercentage(aPercentage)
{
  
  if (aPercentage < 0)
  {
    dialog.progress.mode = "undetermined";
    dialog.progressText.value = "";
  }
  else
  {
    dialog.progress.removeAttribute("mode");
    dialog.progress.value = aPercentage;
    dialog.progressText.value = percentFormat.replace("#1", aPercentage);
  }
}


function onLoad()
{
  
  printProgress = window.arguments[0];
  if (!printProgress)
  {
    dump("Invalid argument to printProgress.xul\n");
    window.close();
    return;
  }

  dialog = {
    titleDeck   : document.getElementById("dialog.titleDeck"),
    title       : document.getElementById("dialog.title"),
    progressDeck: document.getElementById("dialog.progressDeck"),
    progress    : document.getElementById("dialog.progress"),
    progressText: document.getElementById("dialog.progressText"),
    cancel      : document.documentElement.getButton("cancel")
  };
  percentFormat = dialog.progressText.getAttribute("basevalue");
  
  dialog.cancel.setAttribute("disabled", "true");

  
  if (window.arguments.length > 1 && window.arguments[1])
  {
    progressParams = window.arguments[1].QueryInterface(Components.interfaces.nsIPrintProgressParams);
    setProgressTitle();
  }

  
  printProgress.registerListener(progressListener);
  printProgress.doneIniting();
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
    catch(ex){}
  }
}



function onCancel()
{
  
  try
  {
    printProgress.processCanceledByUser = true;
  }
  catch(ex)
  {
    return true;
  }
  
  
  return false;
}
