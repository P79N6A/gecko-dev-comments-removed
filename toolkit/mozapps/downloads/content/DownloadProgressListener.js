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
  this._statusFormatKBMB = aStringBundle.getString("statusFormatKBMB");
  this._statusFormatKBKB = aStringBundle.getString("statusFormatKBKB");
  this._statusFormatMBMB = aStringBundle.getString("statusFormatMBMB");
  this._statusFormatUnknownMB = aStringBundle.getString("statusFormatUnknownMB");
  this._statusFormatUnknownKB = aStringBundle.getString("statusFormatUnknownKB");
  this._remain = aStringBundle.getString("remain");
  this._unknownFilesize = aStringBundle.getString("unknownFilesize");
  this._longTimeFormat = aStringBundle.getString("longTimeFormat");
  this._shortTimeFormat = aStringBundle.getString("shortTimeFormat");
  
}

DownloadProgressListener.prototype = 
{
  rateChanges: 0,
  rateChangeLimit: 0,
  priorRate: 0,
  lastUpdate: -500,
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
    
    var now = (new Date()).getTime();
    
    
    if (now - this.lastUpdate < interval && aMaxTotalProgress != "-1" &&  
        parseInt(aCurTotalProgress) < parseInt(aMaxTotalProgress)) {
      return;
    }

    
    this.lastUpdate = now;

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
    }
    else {
      percent = -1;

      
      download.setAttribute("progressmode", "undetermined");
    }

    
    
    
    var status = this._statusFormat;

    
    var KBProgress = parseInt(overallProgress/1024 + .5);
    var KBTotal = parseInt(aMaxTotalProgress/1024 + .5);
    var kbProgress = this._formatKBytes(KBProgress, KBTotal);
    status = this._replaceInsert(status, 1, kbProgress);
    if (download)
      download.setAttribute("status-internal", kbProgress);

    var rate = aDownload.speed;
    if (rate) {
      
      var kRate = rate / 1024; 
      kRate = parseInt( kRate * 10 + .5 ); 
      
      if (kRate != this.priorRate) {
        if (this.rateChanges++ == this.rateChangeLimit) {
            
            this.priorRate = kRate;
            this.rateChanges = 0;
        }
        else {
          
          kRate = this.priorRate;
        }
      }
      else
        this.rateChanges = 0;

      var fraction = kRate % 10;
      kRate = parseInt((kRate - fraction) / 10);

      
      if (kRate < 100)
        kRate += "." + fraction;
      status = this._replaceInsert(status, 2, kRate);
    }
    else
      status = this._replaceInsert(status, 2, "??.?");

    
    if (rate && (aMaxTotalProgress > 0)) {
      var rem = (aMaxTotalProgress - aCurTotalProgress) / rate;
      rem = parseInt(rem + .5);
      
      status = this._replaceInsert(status, 3, this._formatSeconds(rem, this.doc) + " " + this._remain);
    }
    else
      status = this._replaceInsert(status, 3, this._unknownFilesize);
    
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
  
  
  
  
  
  _formatKBytes: function (aKBytes, aTotalKBytes)
  {
    var progressHasMB = parseInt(aKBytes/1024) > 0;
    var totalHasMB = parseInt(aTotalKBytes/1024) > 0;
    
    var format = "";
    if (!progressHasMB && !totalHasMB) {
      if (!aTotalKBytes) {
      	 format = this._statusFormatUnknownKB;
        format = this._replaceInsert(format, 1, aKBytes);
      } else {
        format = this._statusFormatKBKB;
        format = this._replaceInsert(format, 1, aKBytes);
        format = this._replaceInsert(format, 2, aTotalKBytes);
      }
    }
    else if (progressHasMB && totalHasMB) {
      format = this._statusFormatMBMB;
      format = this._replaceInsert(format, 1, (aKBytes / 1024).toFixed(1));
      format = this._replaceInsert(format, 2, (aTotalKBytes / 1024).toFixed(1));
    }
    else if (totalHasMB && !progressHasMB) {
      format = this._statusFormatKBMB;
      format = this._replaceInsert(format, 1, aKBytes);
      format = this._replaceInsert(format, 2, (aTotalKBytes / 1024).toFixed(1));
    }
    else if (progressHasMB && !totalHasMB) {
      format = this._statusFormatUnknownMB;
      format = this._replaceInsert(format, 1, (aKBytes / 1024).toFixed(1));
    }
    else {
      
      dump("*** huh?!\n");
    }
    
    return format;  
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
