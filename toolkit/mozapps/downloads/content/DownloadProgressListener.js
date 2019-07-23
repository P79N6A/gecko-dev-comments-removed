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

var gStrings = [];
const interval = 500; 

function DownloadProgressListener (aDocument, aStringBundle) 
{
  this.doc = aDocument;
  
  this._statusFormat = aStringBundle.getString("statusFormat");
  this._transferSameUnits = aStringBundle.getString("transferSameUnits");
  this._transferDiffUnits = aStringBundle.getString("transferDiffUnits");
  this._transferNoTotal = aStringBundle.getString("transferNoTotal");
  this._remain = aStringBundle.getString("remain");
  this._unknownTimeLeft = aStringBundle.getString("unknownTimeLeft");
  this._longTimeFormat = aStringBundle.getString("longTimeFormat");
  this._shortTimeFormat = aStringBundle.getString("shortTimeFormat");
  this._units = [aStringBundle.getString("bytes"),
                 aStringBundle.getString("kilobyte"),
                 aStringBundle.getString("megabyte"),
                 aStringBundle.getString("gigabyte")];
}

DownloadProgressListener.prototype = 
{
  doc: null,

  onDownloadStateChange: function dlPL_onDownloadStateChange(aState, aDownload)
  {
    var downloadID = "dl" + aDownload.id;
    var download = this.doc.getElementById(downloadID);
    if (download)
      download.setAttribute("state", aDownload.state);
  },

  onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus, aDownload)
  {
    if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP) {
      var downloadID = "dl" + aDownload.id;
      var download = this.doc.getElementById(downloadID);
      if (download)
        download.setAttribute("status", "");
    }
  },

  onProgressChange: function(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress,
                              aCurTotalProgress, aMaxTotalProgress, aDownload)
  {
    var overallProgress = aCurTotalProgress;

    var downloadID = "dl" + aDownload.id;
    var download = this.doc.getElementById(downloadID);

    
    var percent;
    if (aMaxTotalProgress > 0) {
      percent = Math.floor((overallProgress*100.0)/aMaxTotalProgress);
      if (percent > 100)
        percent = 100;

      
      if (download) {
        download.setAttribute("progress", percent);

        download.setAttribute("progressmode", "normal");
        
        onUpdateProgress();
      }
    } else {
      percent = -1;

      
      download.setAttribute("progressmode", "undetermined");
    }

    
    
    
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

      if (download)
        download.setAttribute("status-internal", transfer);
    }

    
    let ([rate, unit] = this._convertByteUnits(aDownload.speed)) {
      
      status = this._replaceInsert(status, 2, rate);
      
      status = this._replaceInsert(status, 3, unit);
    }

    
    let (remain) {
      if ((aDownload.speed > 0) && (aMaxTotalProgress > 0))
        remain = this._formatSeconds((aMaxTotalProgress - aCurTotalProgress) /
                                     aDownload.speed) + " " + this._remain;
      else
        remain = this._unknownTimeLeft;

      
      status = this._replaceInsert(status, 4, remain);
    }
    
    if (download)
      download.setAttribute("status", status);
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
  },

  _formatSeconds: function (secs, doc)
  {
    
    secs = parseInt(secs + .5);
    var hours = parseInt(secs/3600);
    secs -= hours * 3600;
    
    var mins = parseInt(secs/60);
    secs -= mins * 60;
    var result = hours ? this._longTimeFormat : this._shortTimeFormat;

    if (hours < 10)
      hours = "0" + hours;
    if (mins < 10)
      mins = "0" + mins;
    if (secs < 10)
      secs = "0" + secs;

    
    result = this._replaceInsert(result, 1, hours);
    result = this._replaceInsert(result, 2, mins);
    result = this._replaceInsert(result, 3, secs);

    return result;
  }
  
}; 
