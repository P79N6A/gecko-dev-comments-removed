




































 
var gStrings = new Array;
const interval = 500; 

function nsDownloadProgressListener() {
}

nsDownloadProgressListener.prototype = {
    rateChanges: 0,
    rateChangeLimit: 0,
    priorRate: "",
    lastUpdate: -500,
    doc: null,
    get document() {
      return this.doc;
    },
    set document(newval) {
      return this.doc = newval;
    },
    onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus, aDownload)
    {
      if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP)
      {
        var aDownloadID = aDownload.targetFile.path;
        var elt = this.doc.getElementById(aDownloadID).firstChild.firstChild;

        var timeRemainingCol = elt.nextSibling.nextSibling.nextSibling;
        timeRemainingCol.setAttribute("label", "");
        
        var speedCol = timeRemainingCol.nextSibling.nextSibling;
        speedCol.setAttribute("label", "");

        var elapsedCol = speedCol.nextSibling;
        elapsedCol.setAttribute("label", "");

        
        
        var event = this.doc.createEvent('Events');
        event.initEvent('select', false, true);
        this.doc.getElementById("downloadView").dispatchEvent(event);
      }
    },

    onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress,
                               aCurTotalProgress, aMaxTotalProgress, aDownload)
    {
      var overallProgress = aCurTotalProgress;
      
      var now = ( new Date() ).getTime();
      
      if ( now - this.lastUpdate < interval && aMaxTotalProgress != "-1" &&  parseInt(aCurTotalProgress) < parseInt(aMaxTotalProgress) ) {
        return;
      }

      
      this.lastUpdate = now;

      var aDownloadID = aDownload.targetFile.path
      var elt = this.doc.getElementById(aDownloadID).firstChild.firstChild;
      if (this.doc.getElementById("TimeElapsed").getAttribute("hidden") != "true") {
        elapsedCol = elt.nextSibling.nextSibling.nextSibling.nextSibling.nextSibling.nextSibling;
        
        elapsedCol.setAttribute("label", formatSeconds( now / 1000 - aDownload.startTime / 1000000, this.doc ));
      }
      
      var percent;
      var progressCol = elt.nextSibling;
      if ( aMaxTotalProgress > 0)
      {
        percent = Math.floor((overallProgress*100.0)/aMaxTotalProgress);
        if ( percent > 100 )
          percent = 100;

        
        progressCol.setAttribute( "value", percent );

        progressCol.setAttribute("mode", "normal");
      }
      else
      {
        percent = -1;

        
        progressCol.setAttribute( "mode", "undetermined" );
      }

      
      
      var status = getString( "progressMsgNoRate", this.doc );

      
      status = replaceInsert( status, 1, parseInt( overallProgress/1024 + .5 ) );

      
      if ( aMaxTotalProgress != "-1" )
         status = replaceInsert( status, 2, parseInt( aMaxTotalProgress/1024 + .5 ) );
      else
         status = replaceInsert( status, 2, "??" );
      
      var rateMsg = getString( "rateMsg", this.doc );
      var rate = aDownload.speed;
      if ( rate )
      {
        
        var kRate = (rate / 1024).toFixed(1); 
		
        
        if ( kRate != this.priorRate )
        {
          if ( this.rateChanges++ == this.rateChangeLimit )
          {
             
             this.priorRate = kRate;
             this.rateChanges = 0;
          }
          else
          {
            
            kRate = this.priorRate;
          }
        }
        else
          this.rateChanges = 0;

        
        rateMsg = replaceInsert( rateMsg, 1, kRate );
      }
      else
        rateMsg = replaceInsert( rateMsg, 1, "??.?" );

      var timeRemainingCol = elt.nextSibling.nextSibling.nextSibling;

      
      var statusCol = timeRemainingCol.nextSibling;
      statusCol.setAttribute("label", status);

      var speedCol = statusCol.nextSibling;
      speedCol.setAttribute("label", rateMsg);
      
      if (this.doc.getElementById("ProgressPercent").getAttribute("hidden") != "true") {
        var progressText = elt.nextSibling.nextSibling;
        if (percent < 0)
          progressText.setAttribute("label", "");
        else {
          var percentMsg = getString( "percentMsg", this.doc );      
          percentMsg = replaceInsert( percentMsg, 1, percent );
          progressText.setAttribute("label", percentMsg);
        }
      }
      
      if ( rate && (aMaxTotalProgress > 0) )
      {
        var rem = ( aMaxTotalProgress - aCurTotalProgress ) / rate;
        rem = parseInt( rem + .5 );
        timeRemainingCol.setAttribute("label", formatSeconds( rem, this.doc ));
      }
      else
        timeRemainingCol.setAttribute("label", getString( "unknownTime", this.doc ));
    },
    onLocationChange: function(aWebProgress, aRequest, aLocation, aDownload)
    {
    },
    onStatusChange: function(aWebProgress, aRequest, aStatus, aMessage, aDownload)
    {
    },
    onSecurityChange: function(aWebProgress, aRequest, state, aDownload)
    {
    },
    QueryInterface : function(iid)
    {
     if (iid.equals(Components.interfaces.nsIDownloadProgressListener) ||
         iid.equals(Components.interfaces.nsISupports))
      return this;

     throw Components.results.NS_NOINTERFACE;
    }
};

var nsDownloadProgressListenerFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
        throw Components.results.NS_ERROR_NO_AGGREGATION;

    return (new nsDownloadProgressListener()).QueryInterface(iid);
  }
};

var nsDownloadProgressListenerModule = {

  registerSelf: function (compMgr, fileSpec, location, type)
  { 
    var compReg = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);
    compReg.registerFactoryLocation(Components.ID("{09cddbea-1dd2-11b2-aa15-c41ffea19d79}"),
                                    "Download Progress Listener",
                                    "@mozilla.org/download-manager/listener;1",
                                    fileSpec, location, type);
  },
  canUnload: function(compMgr)
  {
    return true;
  },

  getClassObject: function (compMgr, cid, iid) {
    if (!cid.equals(Components.ID("{09cddbea-1dd2-11b2-aa15-c41ffea19d79}")))
        throw Components.results.NS_ERROR_NO_INTERFACE;
    
    if (!iid.equals(Components.interfaces.nsIFactory))
        throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

    return nsDownloadProgressListenerFactory;
  }
};

function NSGetModule(compMgr, fileSpec) {
    return nsDownloadProgressListenerModule;
}

function replaceInsert( text, index, value ) {
   var result = text;
   var regExp = new RegExp( "#"+index );
   result = result.replace( regExp, value );
   return result;
}

function getString( stringId, doc ) {
   
   if ( !gStrings[ stringId ] ) {
      
      var elem = doc.getElementById( "strings."+stringId );
      try {
        if ( elem
           &&
           elem.childNodes
           &&
           elem.childNodes[0]
           &&
           elem.childNodes[0].nodeValue ) {
         gStrings[ stringId ] = elem.childNodes[0].nodeValue;
        } else {
          
          gStrings[ stringId ] = "";
        }
      } catch (e) { gStrings[ stringId ] = ""; }
   }
   return gStrings[ stringId ];
}

function formatSeconds( secs, doc )
{
  
  secs = parseInt( secs + .5 );
  var hours = parseInt( secs/3600 );
  secs -= hours*3600;
  var mins = parseInt( secs/60 );
  secs -= mins*60;
  var result;
  if ( hours )
    result = getString( "longTimeFormat", doc );
  else
    result = getString( "shortTimeFormat", doc );

  if ( hours < 10 )
     hours = "0" + hours;
  if ( mins < 10 )
     mins = "0" + mins;
  if ( secs < 10 )
     secs = "0" + secs;

  
  result = replaceInsert( result, 1, hours );
  result = replaceInsert( result, 2, mins );
  result = replaceInsert( result, 3, secs );

  return result;
}


