



















































function nsProgressDialog() {
    
    this.mParent      = null;
    this.mOperation   = null;
    this.mStartTime   = ( new Date() ).getTime();
    this.observer     = null;
    this.mLastUpdate  = Number.MIN_VALUE; 
    this.mInterval    = 750; 
    this.mElapsed     = 0;
    this.mLoaded      = false;
    this.fields       = new Array;
    this.strings      = new Array;
    this.mSource      = null;
    this.mTarget      = null;
    this.mTargetFile  = null;
    this.mMIMEInfo    = null;
    this.mDialog      = null;
    this.mDisplayName = null;
    this.mPaused      = false;
    this.mRequest     = null;
    this.mCompleted   = false;
    this.mMode        = "normal";
    this.mPercent     = -1;
    this.mRate        = 0;
    this.mBundle      = null;
    this.mCancelDownloadOnClose = true;
}

const nsIProgressDialog      = Components.interfaces.nsIProgressDialog;
const nsIWindowWatcher       = Components.interfaces.nsIWindowWatcher;
const nsIWebProgressListener = Components.interfaces.nsIWebProgressListener;
const nsITextToSubURI        = Components.interfaces.nsITextToSubURI;
const nsIChannel             = Components.interfaces.nsIChannel;
const nsIFileURL             = Components.interfaces.nsIFileURL;
const nsIURL                 = Components.interfaces.nsIURL;
const nsILocalFile           = Components.interfaces.nsILocalFile;

nsProgressDialog.prototype = {
    
    debug: false,

    
    dialogChrome:   "chrome://global/content/nsProgressDialog.xul",
    dialogFeatures: "chrome,titlebar,minimizable=yes,dialog=no",

    
    get saving()            { return this.MIMEInfo == null ||
                              this.MIMEInfo.preferredAction == Components.interfaces.nsIMIMEInfo.saveToDisk; },
    get parent()            { return this.mParent; },
    set parent(newval)      { return this.mParent = newval; },
    get operation()         { return this.mOperation; },
    set operation(newval)   { return this.mOperation = newval; },
    get observer()          { return this.mObserver; },
    set observer(newval)    { return this.mObserver = newval; },
    get startTime()         { return this.mStartTime; },
    set startTime(newval)   { return this.mStartTime = newval/1000; }, 
    get lastUpdate()        { return this.mLastUpdate; },
    set lastUpdate(newval)  { return this.mLastUpdate = newval; },
    get interval()          { return this.mInterval; },
    set interval(newval)    { return this.mInterval = newval; },
    get elapsed()           { return this.mElapsed; },
    set elapsed(newval)     { return this.mElapsed = newval; },
    get loaded()            { return this.mLoaded; },
    set loaded(newval)      { return this.mLoaded = newval; },
    get source()            { return this.mSource; },
    set source(newval)      { return this.mSource = newval; },
    get target()            { return this.mTarget; },
    get targetFile()        { return this.mTargetFile; },
    get MIMEInfo()          { return this.mMIMEInfo; },
    set MIMEInfo(newval)    { return this.mMIMEInfo = newval; },
    get dialog()            { return this.mDialog; },
    set dialog(newval)      { return this.mDialog = newval; },
    get displayName()       { return this.mDisplayName; },
    set displayName(newval) { return this.mDisplayName = newval; },
    get paused()            { return this.mPaused; },
    get completed()         { return this.mCompleted; },
    get mode()              { return this.mMode; },
    get percent()           { return this.mPercent; },
    get rate()              { return this.mRate; },
    get kRate()             { return this.mRate / 1024; },
    get cancelDownloadOnClose() { return this.mCancelDownloadOnClose; },
    set cancelDownloadOnClose(newval) { return this.mCancelDownloadOnClose = newval; },

    set target(newval) {
        
        
        if (newval instanceof nsIFileURL && newval.file instanceof nsILocalFile) {
            this.mTargetFile = newval.file.QueryInterface(nsILocalFile);
        } else {
            this.mTargetFile = null;
        }

        return this.mTarget = newval;
    },

    
    set paused(newval)      { return this.setPaused(newval); },
    set completed(newval)   { return this.setCompleted(newval); },
    set mode(newval)        { return this.setMode(newval); },
    set percent(newval)     { return this.setPercent(newval); },
    set rate(newval)        { return this.setRate(newval); },

    

    
    open: function( aParentWindow ) {
        
        this.parent    = aParentWindow;

        
        var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                   .getService( nsIWindowWatcher );
        this.dialog = ww.openWindow( this.parent,
                                     this.dialogChrome,
                                     null,
                                     this.dialogFeatures,
                                     this );
    },

    init: function( aSource, aTarget, aDisplayName, aMIMEInfo, aStartTime,
                    aTempFile, aOperation ) {
      this.source = aSource;
      this.target = aTarget;
      this.displayName = aDisplayName;
      this.MIMEInfo = aMIMEInfo;
      if ( aStartTime ) {
          this.startTime = aStartTime;
      }
      this.operation = aOperation;
    },

    
    
    
    
    

    
    onStateChange: function( aWebProgress, aRequest, aStateFlags, aStatus, aDownload ) {
        if ( aStateFlags & nsIWebProgressListener.STATE_STOP ) {
            
            if ( this.targetFile != null ) {
                
                this.completed = true;
                return;
            }

            
            
            try {
                var chan = aRequest.QueryInterface(nsIChannel);
                if (chan.URI.equals(this.target)) {
                    
                    this.completed = true;
                }
            }
            catch (e) {
            }
        }
    },

    
    onProgressChange: function( aWebProgress,
                                aRequest,
                                aCurSelfProgress,
                                aMaxSelfProgress,
                                aCurTotalProgress,
                                aMaxTotalProgress,
                                aDownload ) {
      return this.onProgressChange64(aWebProgress, aRequest, aCurSelfProgress,
              aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress, aDownload);
    },

    onProgressChange64: function( aWebProgress,
                                  aRequest,
                                  aCurSelfProgress,
                                  aMaxSelfProgress,
                                  aCurTotalProgress,
                                  aMaxTotalProgress,
                                  aDownload ) {
        
        var now = ( new Date() ).getTime();

        
        if ( now - this.lastUpdate < this.interval ) {
            return;
        }

        
        this.lastUpdate = now;

        
        this.elapsed = now - this.startTime;

        
        if ( aMaxTotalProgress > 0) {
            this.percent = Math.floor( ( aCurTotalProgress * 100.0 ) / aMaxTotalProgress );
        } else {
            this.percent = -1;
        }

        
        if ( !this.loaded ) {
            return;
        }

        
        this.setValue( "timeElapsed", this.formatSeconds( this.elapsed / 1000 ) );

        
        
        var status = this.getString( "progressMsg" );

        
        status = this.replaceInsert( status, 1, parseInt( aCurTotalProgress/1024 + .5 ) );

        
        if ( aMaxTotalProgress != "-1" ) {
            status = this.replaceInsert( status, 2, parseInt( aMaxTotalProgress/1024 + .5 ) );
        } else {
            status = this.replaceInsert( status, 2, "??" );
        }

        
        if ( this.elapsed ) {
            
            
            if ( aDownload ) {
                this.rate = aDownload.speed;
            } else {
                this.rate = ( aCurTotalProgress * 1000 ) / this.elapsed;
            }
            status = this.replaceInsert( status, 3, this.kRate.toFixed(1) );
        } else {
            
            status = this.replaceInsert( status, 3, "??.?" );
        }

        
        this.setValue( "status", status );

        
        if ( this.rate && ( aMaxTotalProgress > 0 ) ) {
            
            var rem = Math.round( ( aMaxTotalProgress - aCurTotalProgress ) / this.rate );
            this.setValue( "timeLeft", this.formatSeconds( rem ) );
        } else {
            
            this.setValue( "timeLeft", this.getString( "unknownTime" ) );
        }
    },

    
    onStatusChange: function( aWebProgress, aRequest, aStatus, aMessage, aDownload ) {
        
        if ( aStatus != Components.results.NS_OK ) {
            if ( this.loaded ) {
                
                var prompter = Components.classes[ "@mozilla.org/embedcomp/prompt-service;1" ]
                                   .getService( Components.interfaces.nsIPromptService );
                
                var title = this.getProperty( this.saving ? "savingAlertTitle" : "openingAlertTitle",
                                              [ this.fileName() ],
                                              1 );
                prompter.alert( this.dialog, title, aMessage );

                
                if ( !this.completed ) {
                    this.onCancel();
                }
            } else {
                
                
                
                this.dialog.setTimeout( function(obj,wp,req,stat,msg){obj.onStatusChange(wp,req,stat,msg)},
                                        100, this, aWebProgress, aRequest, aStatus, aMessage );
            }
        }
    },

    
    onLocationChange: function( aWebProgress, aRequest, aLocation, aDownload ) {
    },

    onSecurityChange: function( aWebProgress, aRequest, aState, aDownload ) {
    },

    
    observe: function( anObject, aTopic, aData ) {
        
        
        switch ( aTopic ) {
        case "onload":
            this.onLoad();
            break;
        case "oncancel":
            this.onCancel();
            break;
        case "onpause":
            this.onPause();
            break;
        case "onlaunch":
            this.onLaunch();
            break;
        case "onreveal":
            this.onReveal();
            break;
        case "onunload":
            this.onUnload();
            break;
        case "oncompleted":
            
            
            this.completed = true;
            break;
        default:
            break;
        }
    },

    

    
    
    QueryInterface: function (iid) {
        if (iid.equals(Components.interfaces.nsIProgressDialog) ||
            iid.equals(Components.interfaces.nsIDownload) ||
            iid.equals(Components.interfaces.nsITransfer) ||
            iid.equals(Components.interfaces.nsIWebProgressListener) ||
            iid.equals(Components.interfaces.nsIWebProgressListener2) ||
            iid.equals(Components.interfaces.nsIDownloadProgressListener) ||
            iid.equals(Components.interfaces.nsIObserver) ||
            iid.equals(Components.interfaces.nsIInterfaceRequestor) ||
            iid.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    

    getInterface: function(iid) {
        if (iid.equals(Components.interfaces.nsIPrompt) ||
            iid.equals(Components.interfaces.nsIAuthPrompt)) {
            
            var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                               .getService(Components.interfaces.nsIWindowWatcher);
            var prompt;
            if (iid.equals(Components.interfaces.nsIPrompt))
                prompt = ww.getNewPrompter(this.parent);
            else
                prompt = ww.getNewAuthPrompter(this.parent);
            return prompt;
        }
        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    

    
    onLoad: function() {
        
        this.loaded = true;

        
        this.loadDialog();

        
        if ( this.dialog.opener ) {
            this.dialog.moveToAlertPosition();
        } else {
            this.dialog.centerWindowOnScreen();
        }

        
        
        
        
        if ( !this.completed && !this.saving ) {
            this.dialogElement( "keep" ).focus();
        } else {
            this.dialogElement( "cancel" ).focus();
        }
    },

    
    loadDialog: function() {
        
        if ( !this.saving ) {
            
            this.setValue( "sourceLabel", this.getString( "openingSource" ) );

            
            if ( this.MIMEInfo && this.MIMEInfo.preferredApplicationHandler ) {
                var appName = this.MIMEInfo.preferredApplicationHandler.leafName;
                if ( appName == null || appName.length == 0 ) {
                    this.hide( "targetRow" );
                } else {
                    
                    this.setValue( "targetLabel", this.getString( "openingTarget" ) );
                    
                    this.setValue( "target", appName );
                }
           } else {
               this.hide( "targetRow" );
           }
        } else {
            
            if (this.targetFile != null) {
                this.setValue( "target", this.targetFile.path );
            } else {
                this.setValue( "target", this.target.spec );
                this.hide( "pauseResume" );
                this.hide( "launch" );
                this.hide( "reveal" );
            }
        }

        
        this.setValue( "source", this.source.spec );

        var now = ( new Date() ).getTime();

        
        if ( !this.elapsed ) {
            this.elapsed = now - this.startTime;
        }

        
        this.setValue( "timeElapsed", this.formatSeconds( this.elapsed / 1000 ) );
        this.setValue( "timeLeft", this.getString( "unknownTime" ) );

        
        
        if ( !this.saving || !this.targetFile ) {
            
            this.hide( "keep" );
        } else {
            
            var prefs = Components.classes[ "@mozilla.org/preferences-service;1" ]
                            .getService( Components.interfaces.nsIPrefBranch );
            if ( prefs ) {
                this.dialogElement( "keep" ).checked = prefs.getBoolPref( "browser.download.progressDnldDialog.keepAlive" );
            }
        }

        
        this.setTitle();
    },

    
    
    onCancel: function() {
        
        if ( !this.completed ) {
            if ( this.operation ) {
                const NS_BINDING_ABORTED = 0x804b0002;
                this.operation.cancel(NS_BINDING_ABORTED);
                
            }
            if ( this.observer ) {
                this.observer.observe( this, "oncancel", "" );
            }
            this.paused = false;
        }
        
        
        if ( this.dialog ) {
            
            this.dialog.close();
        }
    },

    
    
    onUnload: function() {
        
        if ( this.saving ) {
            var prefs = Components.classes["@mozilla.org/preferences-service;1"]
                          .getService( Components.interfaces.nsIPrefBranch );
            if ( prefs ) {
                prefs.setBoolPref( "browser.download.progressDnldDialog.keepAlive", this.dialogElement( "keep" ).checked );
            }
         }
         this.dialog = null; 
         if ( this.mCancelDownloadOnClose ) {
             this.onCancel();
         }
    },

    
    
    onPause: function() {
         this.paused = !this.paused;
    },

    
    
    onLaunch: function() {
         try {
           const kDontAskAgainPref  = "browser.download.progressDnlgDialog.dontAskForLaunch";
           try {
             var pref = Components.classes["@mozilla.org/preferences-service;1"]
                              .getService(Components.interfaces.nsIPrefBranch);
             var dontAskAgain = pref.getBoolPref(kDontAskAgainPref);
           } catch (e) {
             
             dontAskAgain = false;
           }
           if ( !dontAskAgain && this.targetFile.isExecutable() ) {
             try {
               var promptService = Components.classes["@mozilla.org/embedcomp/prompt-service;1"]
                                               .getService( Components.interfaces.nsIPromptService );
             } catch (ex) {
               
               return;
             }
             var title = this.getProperty( "openingAlertTitle",
                                           [ this.fileName() ],
                                           1 );
             var msg = this.getProperty( "securityAlertMsg",
                                         [ this.fileName() ],
                                         1 );
             var dontaskmsg = this.getProperty( "dontAskAgain",
                                                [ ], 0 );
             var checkbox = {value:0};
             var okToProceed = promptService.confirmCheck(this.dialog, title, msg, dontaskmsg, checkbox);
             try {
               if (checkbox.value != dontAskAgain)
                 pref.setBoolPref(kDontAskAgainPref, checkbox.value);
             } catch (ex) {}

             if ( !okToProceed )
               return;
           }
           this.targetFile.launch();
           this.dialog.close();
         } catch ( exception ) {
             
             dump( "nsProgressDialog::onLaunch failed: " + exception + "\n" );
         }
    },

    
    
    onReveal: function() {
         try {
             this.targetFile.reveal();
             this.dialog.close();
         } catch ( exception ) {
         }
    },

    
    fileName: function() {
        if ( this.targetFile != null )
            return this.targetFile.leafName;
        try {
            var escapedFileName = this.target.QueryInterface(nsIURL).fileName;
            var textToSubURI = Components.classes["@mozilla.org/intl/texttosuburi;1"]
                                         .getService(nsITextToSubURI);
            return textToSubURI.unEscapeURIForUI(this.target.originCharset, escapedFileName);
        } catch (e) {}
        return "";
    },

    
    setTitle: function() {
        
        
        var title = this.saving
            ? ( this.percent != -1 ? this.getString( "savingTitle" ) : this.getString( "unknownSavingTitle" ) )
            : ( this.percent != -1 ? this.getString( "openingTitle" ) : this.getString( "unknownOpeningTitle" ) );


        
        title = this.replaceInsert( title, 1, this.fileName() );

        
        if ( this.percent != -1 ) {
            title = this.replaceInsert( title, 2, this.percent );
        }

        
        if ( this.dialog ) {
            this.dialog.document.title = title;
        }
    },

    
    setPercent: function( percent ) {
        
        if ( percent > 100 ) {
            percent = 100;
        }
        
        if ( this.percent != percent ) {
            this.mPercent = percent;

            
            if ( !this.loaded ) {
                return this.mPercent;
            }

            if ( percent == -1 ) {
                
                this.mode = "undetermined";

                
                this.setValue( "progressText", "" );
            } else {
                
                this.mode = "normal";

                
                this.setValue( "progress", percent );

                
                this.setValue( "progressText", this.replaceInsert( this.getString( "percentMsg" ), 1, percent ) );
            }

            
            this.setTitle();
        }
        return this.mPercent;
    },

    
    setRate: function( rate ) {
        this.mRate = rate;
        return this.mRate;
    },

    
    setCompleted: function() {
        
        if ( !this.loaded ) {
            this.dialog.setTimeout( function(obj){obj.setCompleted()}, 100, this );
            return false;
        }
        if ( !this.mCompleted ) {
            this.mCompleted = true;

            
            if ( this.dialog && this.dialogElement( "keep" ).checked ) {
                
                var string = this.getString( "completeMsg" );
                string = this.replaceInsert( string,
                                             1,
                                             this.formatSeconds( this.elapsed/1000 ) );
                string = this.replaceInsert( string,
                                             2,
                                             this.targetFile.fileSize >> 10 );

                this.setValue( "status", string);
                
                this.percent = 100;

                
                this.setValue( "timeLeft", this.formatSeconds( 0 ) );

                
                var cancelButton = this.dialogElement( "cancel" );
                cancelButton.label = this.getString( "close" );
                cancelButton.focus();

                
                var enableButtons = true;
                try {
                  var prefs = Components.classes[ "@mozilla.org/preferences-service;1" ]
                                  .getService( Components.interfaces.nsIPrefBranch );
                  enableButtons = prefs.getBoolPref( "browser.download.progressDnldDialog.enable_launch_reveal_buttons" );
                } catch ( e ) {
                }

                if ( enableButtons ) {
                    this.enable( "reveal" );
                    try {
                        if ( this.targetFile != null ) {
                            this.enable( "launch" );
                        }
                    } catch(e) {
                    }
                }

                
                this.dialogElement( "pauseResume" ).disabled = true;

                
                this.dialog.sizeToContent();

                
               this.dialog.getAttention();
            } else if ( this.dialog ) {
                this.dialog.close();
            }
        }
        return this.mCompleted;
    },

    
    setMode: function( newMode ) {
        if ( this.mode != newMode ) {
            
            this.dialogElement( "progress" ).setAttribute( "mode", newMode );
        }
        return this.mMode = newMode;
    },

    
    setPaused: function( pausing ) {
        
        if ( this.paused != pausing ) {
            var string = pausing ? "resume" : "pause";
            this.dialogElement( "pauseResume" ).label = this.getString(string);

            
            if ( this.observer ) {
                this.observer.observe( this, pausing ? "onpause" : "onresume" , "" );
            }
        }
        return this.mPaused = pausing;
    },

    
    formatSeconds: function( secs ) {
        
        secs = parseInt( secs + .5 );
        var hours = parseInt( secs/3600 );
        secs -= hours*3600;
        var mins = parseInt( secs/60 );
        secs -= mins*60;
        var result;
        if ( hours )
            result = this.getString( "longTimeFormat" );
        else
            result = this.getString( "shortTimeFormat" );

        if ( hours < 10 )
            hours = "0" + hours;
        if ( mins < 10 )
            mins = "0" + mins;
        if ( secs < 10 )
            secs = "0" + secs;

        
        result = this.replaceInsert( result, 1, hours );
        result = this.replaceInsert( result, 2, mins );
        result = this.replaceInsert( result, 3, secs );

        return result;
    },

    
    dialogElement: function( id ) {
        
        if ( !( id in this.fields ) ) {
            
            try {
                this.fields[ id ] = this.dialog.document.getElementById( id );
            } catch(e) {
                this.fields[ id ] = {
                    value: "",
                    setAttribute: function(id,val) {},
                    removeAttribute: function(id) {}
                    }
            }
        }
        return this.fields[ id ];
    },

    
    setValue: function( id, val ) {
        this.dialogElement( id ).value = val;
    },

    
    enable: function( field ) {
        this.dialogElement( field ).removeAttribute( "disabled" );
    },

    
    getProperty: function( propertyId, strings, len ) {
        if ( !this.mBundle ) {
            this.mBundle = Components.classes[ "@mozilla.org/intl/stringbundle;1" ]
                             .getService( Components.interfaces.nsIStringBundleService )
                               .createBundle( "chrome://global/locale/nsProgressDialog.properties");
        }
        return len ? this.mBundle.formatStringFromName( propertyId, strings, len )
                   : this.mBundle.getStringFromName( propertyId );
    },

    
    getString: function ( stringId ) {
        
        if ( !( this.strings && stringId in this.strings ) ) {
            
            this.strings[ stringId ] = "";
            
            try {
                this.strings[ stringId ] = this.dialog.document.getElementById( "string."+stringId ).childNodes[0].nodeValue;
            } catch (e) {}
       }
       return this.strings[ stringId ];
    },

    
    replaceInsert: function( text, index, value ) {
        var result = text;
        var regExp = new RegExp( "#"+index );
        result = result.replace( regExp, value );
        return result;
    },

    
    hide: function( field ) {
        this.dialogElement( field ).hidden = true;

        
        var sep = this.dialogElement( field+"Separator" );
        if (sep)
            sep.hidden = true;
    },

    
    hex: function( x ) {
        return "0x" + ("0000000" + Number(x).toString(16)).slice(-8);
    },

    
    dump: function( text ) {
        if ( this.debug ) {
            dump( text );
        }
    }
}



var module = {
    
    registerSelf: function (compMgr, fileSpec, location, type) {
        var compReg = compMgr.QueryInterface( Components.interfaces.nsIComponentRegistrar );
        compReg.registerFactoryLocation( this.cid,
                                         "Mozilla Download Progress Dialog",
                                         this.contractId,
                                         fileSpec,
                                         location,
                                         type );
    },

    
    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.cid))
            throw Components.results.NS_ERROR_NO_INTERFACE;

        if (!iid.equals(Components.interfaces.nsIFactory))
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;

        return this.factory;
    },

    
    cid: Components.ID("{F5D248FD-024C-4f30-B208-F3003B85BC92}"),

    
    contractId: "@mozilla.org/progressdialog;1",

    
    factory: {
        
        createInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;

            return (new nsProgressDialog()).QueryInterface(iid);
        }
    },

    
    canUnload: function(compMgr) {
        return true;
    }
};


function NSGetModule(compMgr, fileSpec) {
    return module;
}
