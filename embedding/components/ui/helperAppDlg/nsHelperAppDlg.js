

















































const nsIHelperAppLauncherDialog = Components.interfaces.nsIHelperAppLauncherDialog;
const REASON_CANTHANDLE = nsIHelperAppLauncherDialog.REASON_CANTHANDLE;
const REASON_SERVERREQUEST = nsIHelperAppLauncherDialog.REASON_SERVERREQUEST;
const REASON_TYPESNIFFED = nsIHelperAppLauncherDialog.REASON_TYPESNIFFED;




function nsHelperAppDialog() {
    
    this.mLauncher = null;
    this.mContext  = null;
    this.mSourcePath = null;
    this.chosenApp = null;
    this.givenDefaultApp = false;
    this.strings   = new Array;
    this.elements  = new Array;
    this.updateSelf = true;
    this.mTitle    = "";
    this.mIsMac    = false;
}

nsHelperAppDialog.prototype = {
    
    debug: false,

    nsIMIMEInfo  : Components.interfaces.nsIMIMEInfo,

    
    dump: function( text ) {
        if ( this.debug ) {
            dump( text );
        }
    },

    
    QueryInterface: function (iid) {
        if (iid.equals(Components.interfaces.nsIHelperAppLauncherDialog) ||
            iid.equals(Components.interfaces.nsISupports))
            return this;

        Components.returnCode = Components.results.NS_ERROR_NO_INTERFACE;
        return null;
    },

    

    
    
    
    show: function(aLauncher, aContext, aReason)  {
         this.mLauncher = aLauncher;
         this.mContext  = aContext;
         this.mReason   = aReason;
         
         var ww = Components.classes["@mozilla.org/embedcomp/window-watcher;1"]
                    .getService( Components.interfaces.nsIWindowWatcher );
         this.mDialog = ww.openWindow( null, 
                                       "chrome://global/content/nsHelperAppDlg.xul",
                                       null,
                                       "chrome,titlebar,dialog=yes",
                                       null );
         
         this.mDialog.dialog = this;
         
         this.mIsMac = (this.mDialog.navigator.platform.indexOf( "Mac" ) != -1);
         this.progressListener.helperAppDlg = this;
         this.mLauncher.setWebProgressListener( this.progressListener );
    },

    
    promptForSaveToFile: function(aLauncher, aContext, aDefaultFile, aSuggestedFileExtension) {
        var result = "";

        const prefSvcContractID = "@mozilla.org/preferences-service;1";
        const prefSvcIID = Components.interfaces.nsIPrefService;
        var branch = Components.classes[prefSvcContractID].getService(prefSvcIID)
                                                          .getBranch("browser.download.");
        var dir = null;

        const nsILocalFile = Components.interfaces.nsILocalFile;
        const kDownloadDirPref = "dir";

        
        try {
            dir = branch.getComplexValue(kDownloadDirPref, nsILocalFile);
        } catch (e) { }

        var bundle = Components.classes["@mozilla.org/intl/stringbundle;1"]
                               .getService(Components.interfaces.nsIStringBundleService)
                               .createBundle("chrome://global/locale/nsHelperAppDlg.properties");

        var autoDownload = branch.getBoolPref("autoDownload");
        
        if (autoDownload && dir && dir.exists()) {
            if (aDefaultFile == "")
                aDefaultFile = bundle.GetStringFromName("noDefaultFile") + (aSuggestedFileExtension || "");
            dir.append(aDefaultFile);
            return uniqueFile(dir);
        }

        
        var nsIFilePicker = Components.interfaces.nsIFilePicker;
        var picker = Components.classes[ "@mozilla.org/filepicker;1" ]
                       .createInstance( nsIFilePicker );

        var windowTitle = bundle.GetStringFromName( "saveDialogTitle" );

        var parent = aContext
                        .QueryInterface( Components.interfaces.nsIInterfaceRequestor )
                        .getInterface( Components.interfaces.nsIDOMWindowInternal );
        picker.init( parent, windowTitle, nsIFilePicker.modeSave );
        picker.defaultString = aDefaultFile;
        if (aSuggestedFileExtension) {
            
            picker.defaultExtension = aSuggestedFileExtension.substring(1);
        } else {
            try {
                picker.defaultExtension = this.mLauncher.MIMEInfo.primaryExtension;
            } catch (ex) {
            }
        }

        var wildCardExtension = "*";
        if ( aSuggestedFileExtension ) {
            wildCardExtension += aSuggestedFileExtension;
            picker.appendFilter( wildCardExtension, wildCardExtension );
        }

        picker.appendFilters( nsIFilePicker.filterAll );

        try {
            if (dir.exists())
                picker.displayDirectory = dir;
        } catch (e) { }

        if (picker.show() == nsIFilePicker.returnCancel || !picker.file) {
            
            return null;
        }

        
        if (branch.getBoolPref("lastLocation") || autoDownload) {
            var directory = picker.file.parent.QueryInterface(nsILocalFile);
            branch.setComplexValue(kDownloadDirPref, nsILocalFile, directory);
        }

        return picker.file;
    },

    

    
    
    progressListener: {
        
        helperAppDlg: null,

        
        
        onStatusChange: function( aWebProgress, aRequest, aStatus, aMessage ) {
            if ( aStatus != Components.results.NS_OK ) {
                
                var prompter = Components.classes[ "@mozilla.org/embedcomp/prompt-service;1" ]
                                   .getService( Components.interfaces.nsIPromptService );
                
                prompter.alert( this.dialog, this.helperAppDlg.mTitle, aMessage );

                
                this.helperAppDlg.onCancel();
                if ( this.helperAppDlg.mDialog ) {
                    this.helperAppDlg.mDialog.close();
                }
            }
        },

        
        onProgressChange: function( aWebProgress,
                                    aRequest,
                                    aCurSelfProgress,
                                    aMaxSelfProgress,
                                    aCurTotalProgress,
                                    aMaxTotalProgress ) {
        },

        onProgressChange64: function( aWebProgress,
                                      aRequest,
                                      aCurSelfProgress,
                                      aMaxSelfProgress,
                                      aCurTotalProgress,
                                      aMaxTotalProgress ) {
        },

        onStateChange: function( aWebProgress, aRequest, aStateFlags, aStatus ) {
        },

        onLocationChange: function( aWebProgress, aRequest, aLocation ) {
        },

        onSecurityChange: function( aWebProgress, aRequest, state ) {
        },

        onRefreshAttempted: function( aWebProgress, aURI, aDelay, aSameURI ) {
            return true;
        }
    },

    
    initDialog : function() {
         
         var prompt = this.dialogElement( "prompt" );
         var modified = this.replaceInsert( prompt.firstChild.nodeValue, 1, this.getString( "brandShortName" ) );
         prompt.firstChild.nodeValue = modified;

         
         var suggestedFileName = this.mLauncher.suggestedFileName;

         
         var url = this.mLauncher.source.clone();
         try {
           url.userPass = "";
         } catch (ex) {}
         var fname = "";
         this.mSourcePath = url.prePath;
         try {
             url = url.QueryInterface( Components.interfaces.nsIURL );
             
             fname = url.fileName;
             this.mSourcePath += url.directory;
         } catch (ex) {
             
             fname = url.path;
             this.mSourcePath += url.path;
         }

         if (suggestedFileName)
           fname = suggestedFileName;

         this.mTitle = this.replaceInsert( this.mDialog.document.title, 1, fname);
         this.mDialog.document.title = this.mTitle;

         
         this.initIntro(url, fname);

         var iconString = "moz-icon://" + fname + "?size=32&contentType=" + this.mLauncher.MIMEInfo.MIMEType;

         this.dialogElement("contentTypeImage").setAttribute("src", iconString);

         this.initAppAndSaveToDiskValues();

         
         
         
         
         var alwaysHandleCheckbox = this.dialogElement( "alwaysHandle" );
         if (this.mReason != REASON_CANTHANDLE ||
             this.mLauncher.MIMEInfo.MIMEType == "application/octet-stream"){
            alwaysHandleCheckbox.checked = false;
            alwaysHandleCheckbox.disabled = true;
         }
         else {
            alwaysHandleCheckbox.checked = !this.mLauncher.MIMEInfo.alwaysAskBeforeHandling;
         }

         
         if ( this.mDialog.opener ) {
             this.mDialog.moveToAlertPosition();
         } else {
             this.mDialog.sizeToContent();
             this.mDialog.centerWindowOnScreen();
         }

         
         this.dialogElement( "mode" ).focus();

         this.mDialog.document.documentElement.getButton("accept").disabled = true;
         const nsITimer = Components.interfaces.nsITimer;
         this._timer = Components.classes["@mozilla.org/timer;1"]
                                 .createInstance(nsITimer);
         this._timer.initWithCallback(this, 250, nsITimer.TYPE_ONE_SHOT);
    },

    
    initIntro: function(url, filename) {
        var intro = this.dialogElement( "intro" );
        var desc = this.mLauncher.MIMEInfo.description;

        var text;
        if ( this.mReason == REASON_CANTHANDLE )
          text = "intro.";
        else if (this.mReason == REASON_SERVERREQUEST )
          text = "intro.attachment.";
        else if (this.mReason == REASON_TYPESNIFFED )
          text = "intro.sniffed.";

        var modified;
        if (desc)
          modified = this.replaceInsert( this.getString( text + "label" ), 1, desc );
        else
          modified = this.getString( text + "noDesc.label" );

        modified = this.replaceInsert( modified, 2, this.mLauncher.MIMEInfo.MIMEType );
        modified = this.replaceInsert( modified, 3, filename);
        modified = this.replaceInsert( modified, 4, this.getString( "brandShortName" ));

        
        
        var pathString = url.prePath;
        try
        {
          var fileURL = url.QueryInterface(Components.interfaces.nsIFileURL);
          if (fileURL)
          {
            var fileObject = fileURL.file;
            if (fileObject)
            {
              var parentObject = fileObject.parent;
              if (parentObject)
              {
                pathString = parentObject.path;
              }
            }
          }
        } catch(ex) {}


        intro.firstChild.nodeValue = "";
        intro.firstChild.nodeValue = modified;

        
        var location = this.dialogElement( "location" );
        location.value = pathString;
        location.setAttribute( "tooltiptext", this.mSourcePath );
    },

    _timer: null,
    notify: function (aTimer) {
        try { 
          if (!this._blurred)
            this.mDialog.document.documentElement.getButton('accept').disabled = false;
        } catch (ex) {}
        this._timer = null;
    },

    _blurred: false,
    onBlur: function(aEvent) {
        if (aEvent.target != this.mDialog.document)
          return;

        this._blurred = true;
        this.mDialog.document.documentElement.getButton("accept").disabled = true;
    },

    onFocus: function(aEvent) {
        if (aEvent.target != this.mDialog.document)
          return;

        this._blurred = false;
        if (!this._timer) {
          
          var script = "document.documentElement.getButton('accept').disabled = false";
          this.mDialog.setTimeout(script, 250);
        }
    },

    
    openWithDefaultOK: function() {
        var result;

        
        if ( this.mDialog.navigator.platform.indexOf( "Win" ) != -1 ) {
            
            
            
            
            
            

            
            var tmpFile = this.mLauncher.targetFile;

            
            result = !tmpFile.isExecutable();
        } else {
            
            
            
            result = this.mLauncher.MIMEInfo.hasDefaultHandler;
        }
        return result;
    },

    
    initDefaultApp: function() {
        
        var desc = this.mLauncher.MIMEInfo.defaultDescription;
        if ( desc ) {
            this.dialogElement( "useSystemDefault" ).label = this.replaceInsert( this.getString( "defaultApp" ), 1, desc );
        }
    },

    
    getPath: function(file) {
        if (this.mIsMac) {
            return file.leafName || file.path;
        }

        return file.path;
    },

    
    initAppAndSaveToDiskValues: function() {
        
        try {
            this.chosenApp =
              this.mLauncher.MIMEInfo.preferredApplicationHandler
                  .QueryInterface(Components.interfaces.nsILocalHandlerApp);
        } catch (e) {
            this.chosenApp = null;
        }
        
        this.initDefaultApp();

        
        if (this.chosenApp && this.chosenApp.executable &&
            this.chosenApp.executable.path) {
            this.dialogElement( "appPath" ).value = 
              this.getPath(this.chosenApp.executable);
        }

        var useDefault = this.dialogElement( "useSystemDefault" );;
        if (this.mLauncher.MIMEInfo.preferredAction == this.nsIMIMEInfo.useSystemDefault &&
            this.mReason != REASON_SERVERREQUEST) {
            
            useDefault.radioGroup.selectedItem = useDefault;
        } else if (this.mLauncher.MIMEInfo.preferredAction == this.nsIMIMEInfo.useHelperApp &&
                   this.mReason != REASON_SERVERREQUEST) {
            
            var openUsing = this.dialogElement( "openUsing" );
            openUsing.radioGroup.selectedItem = openUsing;
        } else {
            
            var saveToDisk = this.dialogElement( "saveToDisk" );
            saveToDisk.radioGroup.selectedItem = saveToDisk;
        }
        
        if ( !this.openWithDefaultOK() ) {
            
            useDefault.hidden = true;
            
            if ( useDefault.selected ) {
                useDefault.radioGroup.selectedItem = this.dialogElement( "saveToDisk" );
            }
        }

        
        this.toggleChoice();
    },

    
    toggleChoice : function () {
        
        var openUsing = this.dialogElement( "openUsing" ).selected;
        this.dialogElement( "chooseApp" ).disabled = !openUsing;
        
        this.dialogElement( "appPath" ).disabled = !openUsing || this.mIsMac;
        this.updateOKButton();
    },

    
    helperAppChoice: function() {
        var result = this.chosenApp;
        if (!this.mIsMac) {
            var typed  = this.dialogElement( "appPath" ).value;
            
            if ( result ) {
                
                if ( typed != result.executable.path ) {
                    
                    try {
                        result.executable.QueryInterface( Components.interfaces.nsILocalFile ).initWithPath( typed );
                    } catch( e ) {
                        
                        result = null;
                    }
                }
            } else {
                
                var localFile = Components.classes[ "@mozilla.org/file/local;1" ]
                    .createInstance( Components.interfaces.nsILocalFile );
                try {
                    localFile.initWithPath( typed );
                    result = Components.classes[
                      "@mozilla.org/uriloader/local-handler-app;1"].
                      createInstance(Components.interfaces.nsILocalHandlerApp);
                    result.executable = localFile;

                } catch( e ) {
                    result = null;
                }
            }
            
            this.chosenApp = result;
        }
        return result;
    },

    updateOKButton: function() {
        var ok = false;
        if ( this.dialogElement( "saveToDisk" ).selected )
        {
            
            ok = true;
        }
        else if ( this.dialogElement( "useSystemDefault" ).selected )
        {
            
            ok = true;
        }
        else
        {
            
            
            ok = this.chosenApp || /\S/.test( this.dialogElement( "appPath" ).value );
        }

        
        this.mDialog.document.documentElement.getButton( "accept" ).disabled = !ok;
    },

    
    appChanged: function() {
        return this.helperAppChoice() != this.mLauncher.MIMEInfo.preferredApplicationHandler;
    },

    updateMIMEInfo: function() {
        var needUpdate = false;
        
        
        
        
        if ( this.dialogElement( "saveToDisk" ).selected &&
             this.mReason != REASON_SERVERREQUEST ) {
            needUpdate = this.mLauncher.MIMEInfo.preferredAction != this.nsIMIMEInfo.saveToDisk;
            if ( needUpdate )
                this.mLauncher.MIMEInfo.preferredAction = this.nsIMIMEInfo.saveToDisk;
        } else if ( this.dialogElement( "useSystemDefault" ).selected ) {
            needUpdate = this.mLauncher.MIMEInfo.preferredAction != this.nsIMIMEInfo.useSystemDefault;
            if ( needUpdate )
                this.mLauncher.MIMEInfo.preferredAction = this.nsIMIMEInfo.useSystemDefault;
        } else if ( this.dialogElement( "openUsing" ).selected ) {
            
            
            needUpdate = this.mLauncher.MIMEInfo.preferredAction != this.nsIMIMEInfo.useHelperApp || this.appChanged();
            if ( needUpdate ) {
                this.mLauncher.MIMEInfo.preferredAction = this.nsIMIMEInfo.useHelperApp;
                
                var app = this.helperAppChoice();
                this.mLauncher.MIMEInfo.preferredApplicationHandler = app;
            }
        }
        
        if ( this.mReason == REASON_CANTHANDLE )
        {
          
          needUpdate = needUpdate || this.mLauncher.MIMEInfo.alwaysAskBeforeHandling == this.dialogElement( "alwaysHandle" ).checked;

          
          
          
          
          
          
          needUpdate = needUpdate || !this.mLauncher.MIMEInfo.alwaysAskBeforeHandling;

          
          this.mLauncher.MIMEInfo.alwaysAskBeforeHandling = !this.dialogElement( "alwaysHandle" ).checked;
        }

        return needUpdate;
    },

    
    
    updateHelperAppPref: function() {
        
        
        
        this.mDialog.openDialog( "chrome://communicator/content/pref/pref-applications-edit.xul",
                                 "_blank",
                                 "chrome,modal=yes,resizable=no",
                                 this );
    },

    
    onOK: function() {
        
        if ( this.dialogElement( "openUsing" ).selected ) {
            var helperApp = this.helperAppChoice();
            if ( !helperApp || !helperApp.executable ||
                 !helperApp.executable.exists() ) {
                
                var msg = this.replaceInsert( this.getString( "badApp" ), 1, this.dialogElement( "appPath" ).value );
                var svc = Components.classes[ "@mozilla.org/embedcomp/prompt-service;1" ]
                            .getService( Components.interfaces.nsIPromptService );
                svc.alert( this.mDialog, this.getString( "badApp.title" ), msg );
                
                this.mDialog.document.documentElement.getButton( "accept" ).disabled = true;
                
                var path = this.dialogElement( "appPath" );
                if ( !path.disabled ) {
                    path.select();
                    path.focus();
                }
                
                this.chosenApp = null;
                
                return false;
            }
        }

        
        
        this.mLauncher.setWebProgressListener( null );

        
        
        
        
        try {
            var needUpdate = this.updateMIMEInfo();
            if ( this.dialogElement( "saveToDisk" ).selected )
                this.mLauncher.saveToDisk( null, false );
            else
                this.mLauncher.launchWithApplication( null, false );

            
            
            
            if ( needUpdate &&
                 this.mLauncher.MIMEInfo.MIMEType != "application/octet-stream" )
            {
                this.updateHelperAppPref();
            }

        } catch(e) { }

        
        this.mDialog.dialog = null;

        
        return true;
    },

    
    onCancel: function() {
        
        this.mLauncher.setWebProgressListener( null );

        
        try {
            const NS_BINDING_ABORTED = 0x804b0002;
            this.mLauncher.cancel(NS_BINDING_ABORTED);
        } catch( exception ) {
        }

        
        this.mDialog.dialog = null;

        
        return true;
    },

    
    dialogElement: function( id ) {
         
         if ( !( id in this.elements ) ) {
             
             this.elements[ id ] = this.mDialog.document.getElementById( id );
         }
         return this.elements[ id ];
    },

    
    chooseApp: function() {
        var nsIFilePicker = Components.interfaces.nsIFilePicker;
        var fp = Components.classes["@mozilla.org/filepicker;1"].createInstance( nsIFilePicker );
        fp.init( this.mDialog,
                 this.getString( "chooseAppFilePickerTitle" ),
                 nsIFilePicker.modeOpen );

        
        fp.appendFilters( nsIFilePicker.filterAll );

        if ( fp.show() == nsIFilePicker.returnOK && fp.file ) {
            

            var localHandler = Components.classes[
              "@mozilla.org/uriloader/local-handler-app;1"].
                createInstance(Components.interfaces.nsILocalHandlerApp);
            localHandler.executable = fp.file;
            this.chosenApp = localHandler;
            
            this.dialogElement( "appPath" ).value = 
              this.getPath(this.chosenApp.executable);
        }
    },

    
    doDebug: function() {
        const nsIProgressDialog = Components.interfaces.nsIProgressDialog;
        
        var progress = Components.classes[ "@mozilla.org/progressdialog;1" ]
                         .createInstance( nsIProgressDialog );
        
        progress.open( this.mDialog );
    },

    
    dumpObj: function( spec ) {
         var val = "<undefined>";
         try {
             val = eval( "this."+spec ).toString();
         } catch( exception ) {
         }
         this.dump( spec + "=" + val + "\n" );
    },

    
    dumpObjectProperties: function( desc, obj ) {
         for( prop in obj ) {
             this.dump( desc + "." + prop + "=" );
             var val = "<undefined>";
             try {
                 val = obj[ prop ];
             } catch ( exception ) {
             }
             this.dump( val + "\n" );
         }
    },

    
    getString: function( id ) {
        
        if ( !( id in this.strings ) ) {
            
            var elem = this.mDialog.document.getElementById( id );
            if ( elem
                 &&
                 elem.firstChild
                 &&
                 elem.firstChild.nodeValue ) {
                this.strings[ id ] = elem.firstChild.nodeValue;
            } else {
                
                this.strings[ id ] = "";
            }
        }
        return this.strings[ id ];
    },

    
    replaceInsert: function( text, insertNo, replacementText ) {
        var result = text;
        var regExp = new RegExp("#"+insertNo);
        result = result.replace( regExp, replacementText );
        return result;
    }
}



var module = {
    firstTime: true,

    
    registerSelf: function (compMgr, fileSpec, location, type) {
        if (this.firstTime) {
            this.firstTime = false;
            throw Components.results.NS_ERROR_FACTORY_REGISTER_AGAIN;
        }
        compMgr = compMgr.QueryInterface(Components.interfaces.nsIComponentRegistrar);

        compMgr.registerFactoryLocation( this.cid,
                                         "Mozilla Helper App Launcher Dialog",
                                         this.contractId,
                                         fileSpec,
                                         location,
                                         type );
    },

    
    getClassObject: function (compMgr, cid, iid) {
        if (!cid.equals(this.cid)) {
            throw Components.results.NS_ERROR_NO_INTERFACE;
        }

        if (!iid.equals(Components.interfaces.nsIFactory)) {
            throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
        }

        return this.factory;
    },

    
    cid: Components.ID("{F68578EB-6EC2-4169-AE19-8C6243F0ABE1}"),

    
    contractId: "@mozilla.org/helperapplauncherdialog;1",

    
    factory: {
        
        createInstance: function (outer, iid) {
            if (outer != null)
                throw Components.results.NS_ERROR_NO_AGGREGATION;

            return (new nsHelperAppDialog()).QueryInterface(iid);
        }
    },

    
    canUnload: function(compMgr) {
        return true;
    }
};


function NSGetModule(compMgr, fileSpec) {
    return module;
}







function uniqueFile(aLocalFile) {
    while (aLocalFile.exists()) {
        parts = /(-\d+)?(\.[^.]+)?$/.test(aLocalFile.leafName);
        aLocalFile.leafName = RegExp.leftContext + (RegExp.$1 - 1) + RegExp.$2;
    }
    return aLocalFile;
}
