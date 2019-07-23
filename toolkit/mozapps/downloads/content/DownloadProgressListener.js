# -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*-
# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
# 
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
# 
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
# 
# The Original Code is Mozilla.org Code.
# 
# The Initial Developer of the Original Code is
# Doron Rosenberg.
# Portions created by the Initial Developer are Copyright (C) 2001
# the Initial Developer. All Rights Reserved.
# 
# Contributor(s):
#   Blake Ross <blakeross@telocity.com> (Original Author) 
#   Ben Goodger <ben@bengoodger.com> (v2.0) 
#   Edward Lee <edilee@gmail.com>
#   Shawn Wilsher <me@shawnwilsher.com> (v3.0)
# 
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
# 
# ***** END LICENSE BLOCK *****

function DownloadProgressListener() 
{
  var sb = document.getElementById("downloadStrings");
  this._paused = sb.getString("paused");
  this._statusFormat = sb.getString("statusFormat2");
  this._transferSameUnits = sb.getString("transferSameUnits");
  this._transferDiffUnits = sb.getString("transferDiffUnits");
  this._transferNoTotal = sb.getString("transferNoTotal");
  this._timeMinutesLeft = sb.getString("timeMinutesLeft");
  this._timeSecondsLeft = sb.getString("timeSecondsLeft");
  this._timeFewSeconds = sb.getString("timeFewSeconds");
  this._timeUnknown = sb.getString("timeUnknown");
  this._units = [sb.getString("bytes"),
                 sb.getString("kilobyte"),
                 sb.getString("megabyte"),
                 sb.getString("gigabyte")];

  this.lastSeconds = Infinity;
}

DownloadProgressListener.prototype = 
{
  onDownloadStateChange: function dlPL_onDownloadStateChange(aState, aDownload)
  {
    var dl = getDownload(aDownload.id);
    switch (aDownload.state) {
      case Ci.nsIDownloadManager.DOWNLOAD_QUEUED:
        
        gDownloadsActiveTitle.hidden = false;
      case Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING:
        
        
        if (!dl) {
          
          let uri = Cc["@mozilla.org/network/util;1"].
                    getService(Ci.nsIIOService).
                    newFileURI(aDownload.targetFile);
          let referrer = aDownload.referrer;
          dl = createDownloadItem(aDownload.id,
                                  uri.spec,
                                  aDownload.displayName,
                                  aDownload.source.spec,
                                  aDownload.state,
                                  "",
                                  aDownload.percentComplete,
                                  Math.round(aDownload.startTime / 1000),
                                  referrer ? referrer.spec : null);
        }
        gDownloadsView.insertBefore(dl, gDownloadsActiveTitle.nextSibling);
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_FAILED:
      case Ci.nsIDownloadManager.DOWNLOAD_CANCELED:
      case Ci.nsIDownloadManager.DOWNLOAD_BLOCKED:
        downloadCompleted(aDownload);
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_FINISHED:
        downloadCompleted(aDownload);

        autoRemoveAndClose(aDownload);
        break;
      case Ci.nsIDownloadManager.DOWNLOAD_PAUSED:
        let transfer = dl.getAttribute("status-internal");
        let status = this._replaceInsert(this._paused, 1, transfer);
        dl.setAttribute("status", status);
        break;
    }

    
    try {
      dl.setAttribute("state", aDownload.state);
      gDownloadViewController.onCommandUpdate();
    } catch (e) { }
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus, aDownload)
  {
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP) {
      let dl = getDownload(aDownload.id);
      if (dl)
        dl.setAttribute("status", "");
    }
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress,
                              aCurTotalProgress, aMaxTotalProgress, aDownload)
  {
    var download = getDownload(aDownload.id);
    if (!download) {
      
      let uri = Cc["@mozilla.org/network/util;1"].
                getService(Ci.nsIIOService).newFileURI(aDownload.targetFile);
      let referrer = aDownload.referrer;
      let itm = createDownloadItem(aDownload.id, uri.spec,
                                   aDownload.displayName,
                                   aDownload.source.spec,
                                   aDownload.state,
                                   aDownload.percentComplete,
                                   referrer ? referrer.spec : null);
      download = gDownloadsView.insertBefore(itm, gDownloadsActiveTitle.nextSibling);
    }

    
    gDownloadsActiveTitle.hidden = false;

    
    if (aDownload.percentComplete == -1)
      download.setAttribute("progressmode", "undetermined");
    else {
      download.setAttribute("progressmode", "normal");
      download.setAttribute("progress", aDownload.percentComplete);
    }

    
    var event = document.createEvent("Events");
    event.initEvent("ValueChange", true, true);
    document.getAnonymousElementByAttribute(download, "anonid", "progressmeter")
            .dispatchEvent(event);

    
    
    let status = this._statusFormat;

    
    let ([progress, progressUnits] = this._convertByteUnits(aCurTotalProgress),
         [total, totalUnits] = this._convertByteUnits(aMaxTotalProgress),
         transfer) {
      if (total <= 0)
        transfer = this._transferNoTotal;
      else if (progressUnits == totalUnits)
        transfer = this._transferSameUnits;
      else
        transfer = this._transferDiffUnits;

      transfer = this._replaceInsert(transfer, 1, progress);
      transfer = this._replaceInsert(transfer, 2, progressUnits);
      transfer = this._replaceInsert(transfer, 3, total);
      transfer = this._replaceInsert(transfer, 4, totalUnits);

      
      status = this._replaceInsert(status, 1, transfer);

      download.setAttribute("status-internal", transfer);
    }

    
    let ([rate, unit] = this._convertByteUnits(aDownload.speed)) {
      
      status = this._replaceInsert(status, 2, rate);
      
      status = this._replaceInsert(status, 3, unit);
    }

    
    let (remain) {
      if ((aDownload.speed > 0) && (aMaxTotalProgress > 0)) {
        let seconds = Math.ceil((aMaxTotalProgress - aCurTotalProgress) /
                                aDownload.speed);

        
        
        
        let (diff = seconds - this.lastSeconds) {
          if (diff > 0 && diff <= 10)
            seconds = this.lastSeconds;
          else
            this.lastSeconds = seconds;
        }

        
        if (seconds <= 3)
          remain = this._timeFewSeconds;
        
        else if (seconds <= 60)
          remain = this._replaceInsert(this._timeSecondsLeft, 1, seconds);
        else 
          remain = this._replaceInsert(this._timeMinutesLeft, 1,
                                       Math.ceil(seconds / 60));
      } else {
        remain = this._timeUnknown;
      }

      
      status = this._replaceInsert(status, 4, remain);
    }
    
    download.setAttribute("status", status);

    
    onUpdateProgress();
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

    throw Cr.NS_NOINTERFACE;
  },

  _replaceInsert: function ( text, index, value ) 
  {
    var result = text;
    var regExp = new RegExp( "#"+index );
    result = result.replace( regExp, value );
    return result;
  },
  
  
  
  
  _convertByteUnits: function(aBytes)
  {
    let unitIndex = 0;

    
    
    while ((aBytes >= 999.5) && (unitIndex < this._units.length - 1)) {
      aBytes /= 1024;
      unitIndex++;
    }

    
    
    aBytes = aBytes.toFixed((aBytes > 0) && (aBytes < 100) ? 1 : 0);

    return [aBytes, this._units[unitIndex]];
  }
};
