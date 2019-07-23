


















































function nsContextMenu( xulMenu ) {
    this.target         = null;
    this.menu           = null;
    this.popupURL       = null;
    this.onTextInput    = false;
    this.onImage        = false;
    this.onLoadedImage  = false;
    this.onLink         = false;
    this.onMailtoLink   = false;
    this.onSaveableLink = false;
    this.onMetaDataItem = false;
    this.onMathML       = false;
    this.link           = false;
    this.inFrame        = false;
    this.hasBGImage     = false;
    this.isTextSelected = false;
    this.inDirList      = false;
    this.shouldDisplay  = true;
    this.autoDownload   = false;

    
    this.initMenu( xulMenu );
}


nsContextMenu.prototype = {
    
    onDestroy : function () {
    },
    
    initMenu : function ( popup ) {
        
        this.menu = popup;

        const xulNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
        if ( document.popupNode.namespaceURI == xulNS ) {
          this.shouldDisplay = false;
          return;
        }
        
        this.setTarget( document.popupNode );
        
        this.isTextSelected = this.isTextSelection();

        this.initPopupURL();

        
        this.initItems();
    },
    initItems : function () {
        this.initOpenItems();
        this.initNavigationItems();
        this.initViewItems();
        this.initMiscItems();
        this.initSaveItems();
        this.initClipboardItems();
        this.initMetadataItems();
    },
    initOpenItems : function () {
        var showOpen = this.onSaveableLink || ( this.inDirList && this.onLink );

        this.showItem( "context-openlink", showOpen );
        this.showItem( "context-openlinkintab", showOpen );

        this.showItem( "context-sep-open", showOpen );
    },
    initNavigationItems : function () {
        
        this.setItemAttrFromNode( "context-back", "disabled", "canGoBack" );

        
        this.setItemAttrFromNode( "context-forward", "disabled", "canGoForward" );

        var showNav = !( this.isTextSelected || this.onLink || this.onImage || this.onTextInput );
        
        this.showItem( "context-back", showNav );
        this.showItem( "context-forward", showNav );

        this.showItem( "context-reload", showNav );
        
        this.showItem( "context-stop", showNav );
        this.showItem( "context-sep-stop", showNav );

        
        
    },
    initSaveItems : function () {
        var showSave = !( this.inDirList || this.isTextSelected || this.onTextInput || this.onStandaloneImage ||
                       ( this.onLink && this.onImage ) );
        if (showSave)
          goSetMenuValue( "context-savepage", this.autoDownload ? "valueSave" : "valueSaveAs" );
        this.showItem( "context-savepage", showSave );

        
        if (this.onSaveableLink)
          goSetMenuValue( "context-savelink", this.autoDownload ? "valueSave" : "valueSaveAs" );
        this.showItem( "context-savelink", this.onSaveableLink );

        
        showSave = this.onLoadedImage || this.onStandaloneImage;
        if (showSave)
          goSetMenuValue( "context-saveimage", this.autoDownload ? "valueSave" : "valueSaveAs" );
        this.showItem( "context-saveimage", showSave );
        this.showItem( "context-sendimage", showSave );
    },
    initViewItems : function () {
        
        this.showItem( "context-viewpartialsource-selection", this.isTextSelected && !this.onTextInput );
        this.showItem( "context-viewpartialsource-mathml", this.onMathML && !this.isTextSelected );

        var showView = !( this.inDirList || this.onImage || this.isTextSelected || this.onLink || this.onTextInput );

        this.showItem( "context-viewsource", showView );
        this.showItem( "context-viewinfo", showView );

        this.showItem( "context-sep-properties", !( this.inDirList || this.isTextSelected || this.onTextInput ) );
        
        var isWin = navigator.appVersion.indexOf("Windows") != -1;
        this.showItem( "context-setWallpaper", isWin && (this.onLoadedImage || this.onStandaloneImage));

        this.showItem( "context-sep-image", this.onLoadedImage || this.onStandaloneImage);

        if( isWin && this.onLoadedImage )
            
          this.setItemAttr( "context-setWallpaper", "disabled", (("complete" in this.target) && !this.target.complete) ? "true" : null );

        this.showItem( "context-fitimage", this.onStandaloneImage && content.document.imageResizingEnabled );
        if ( this.onStandaloneImage && content.document.imageResizingEnabled ) {
          this.setItemAttr( "context-fitimage", "disabled", content.document.imageIsOverflowing ? null : "true");
          this.setItemAttr( "context-fitimage", "checked", content.document.imageIsResized ? "true" : null);
        }

        this.showItem( "context-reloadimage", this.onImage);

        
        this.showItem( "context-viewimage", this.onImage && !this.onStandaloneImage);

        
        this.showItem( "context-viewbgimage", showView && !this.onStandaloneImage);
        this.showItem( "context-sep-viewbgimage", showView && !this.onStandaloneImage);
        this.setItemAttr( "context-viewbgimage", "disabled", this.hasBGImage ? null : "true");
    },
    initMiscItems : function () {
        
        this.showItem( "context-bookmarkpage", !( this.isTextSelected || this.onTextInput || this.onStandaloneImage ) );
        this.showItem( "context-bookmarklink", this.onLink && !this.onMailtoLink );
        this.showItem( "context-searchselect", this.isTextSelected && !this.onTextInput );
        this.showItem( "frame", this.inFrame );
        this.showItem( "frame-sep", this.inFrame );
        if (this.inFrame)
          goSetMenuValue( "saveframeas", this.autoDownload ? "valueSave" : "valueSaveAs" );
        var blocking = true;
        if (this.popupURL)
          try {
            const PM = Components.classes["@mozilla.org/PopupWindowManager;1"]
                       .getService(Components.interfaces.nsIPopupWindowManager);
            blocking = PM.testPermission(this.popupURL) ==
                       Components.interfaces.nsIPopupWindowManager.DENY_POPUP;
          } catch (e) {
          }

        this.showItem( "popupwindow-reject", this.popupURL && !blocking);
        this.showItem( "popupwindow-allow", this.popupURL && blocking);
        this.showItem( "context-sep-popup", this.popupURL);

        
        this.showItem( "context-sep-bidi", gShowBiDi);
        this.showItem( "context-bidi-text-direction-toggle", this.onTextInput && gShowBiDi);
        this.showItem( "context-bidi-page-direction-toggle", !this.onTextInput && gShowBiDi);
    },
    initClipboardItems : function () {

        
        
        
        

        goUpdateGlobalEditMenuItems();

        this.showItem( "context-undo", this.onTextInput );
        this.showItem( "context-redo", this.onTextInput );
        this.showItem( "context-sep-undo", this.onTextInput );
        this.showItem( "context-cut", this.onTextInput );
        this.showItem( "context-copy", this.isTextSelected || this.onTextInput);
        this.showItem( "context-paste", this.onTextInput );
        this.showItem( "context-delete", this.onTextInput );
        this.showItem( "context-sep-paste", this.onTextInput );
        this.showItem( "context-selectall", true );
        this.showItem( "context-sep-selectall", this.isTextSelected && !this.onTextInput );
        
        
        

        
        
        
        

        
        this.showItem( "context-copyemail", this.onMailtoLink );

        
        this.showItem( "context-copylink", this.onLink );
        this.showItem( "context-sep-copylink", this.onLink );

        
        this.showItem( "context-copyimage", this.onImage );
        this.showItem( "context-sep-copyimage", this.onImage );
    },
    initMetadataItems : function () {
        
        this.showItem( "context-metadata", this.onMetaDataItem );
    },
    
    setTarget : function ( node ) {
        
        this.onImage    = false;
        this.onLoadedImage = false;
        this.onStandaloneImage = false;
        this.onMetaDataItem = false;
        this.onTextInput = false;
        this.imageURL   = "";
        this.onLink     = false;
        this.onMathML   = false;
        this.inFrame    = false;
        this.hasBGImage = false;
        this.bgImageURL = "";

        
        this.target = node;

        this.autoDownload = Components.classes["@mozilla.org/preferences-service;1"]
                                      .getService(Components.interfaces.nsIPrefBranch)
                                      .getBoolPref("browser.download.autoDownload");

        
        if ( this.target.nodeType == Node.ELEMENT_NODE ) {
            if ( this.target instanceof Components.interfaces.nsIImageLoadingContent && this.target.currentURI  ) {
                this.onImage = true;
                var request = this.target.getRequest( Components.interfaces.nsIImageLoadingContent.CURRENT_REQUEST );
                if (request && (request.imageStatus & request.STATUS_SIZE_AVAILABLE))
                    this.onLoadedImage = true;
                this.imageURL = this.target.currentURI.spec;

                if ( this.target.ownerDocument instanceof ImageDocument )
                   this.onStandaloneImage = true;
            } else if ( this.target instanceof HTMLInputElement ) {
               this.onTextInput = this.isTargetATextBox(this.target);
            } else if ( this.target instanceof HTMLTextAreaElement ) {
                 this.onTextInput = true;
            } else if ( this.target instanceof HTMLHtmlElement ) {
               
               var bodyElt = this.target.ownerDocument.getElementsByTagName("body")[0];
               if ( bodyElt ) {
                 var computedURL = this.getComputedURL( bodyElt, "background-image" );
                 if ( computedURL ) {
                   this.hasBGImage = true;
                   this.bgImageURL = this.makeURLAbsolute( bodyElt.baseURI,
                                                           computedURL );
                 }
               }
            } else if ( "HTTPIndex" in content &&
                        content.HTTPIndex instanceof Components.interfaces.nsIHTTPIndex ) {
                this.inDirList = true;
                
                
                var root = this.target;
                while ( root && !this.link ) {
                    if ( root.tagName == "tree" ) {
                        
                        
                        break;
                    }
                    if ( root.getAttribute( "URL" ) ) {
                        
                        this.onLink = true;
                        this.link = { href : root.getAttribute("URL"),
                                      getAttribute: function (attr) {
                                          if (attr == "title") {
                                              return root.firstChild.firstChild.getAttribute("label");
                                          } else {
                                              return "";
                                          }
                                      }
                                    };
                        
                        if ( root.getAttribute( "container" ) == "true" ) {
                            this.onSaveableLink = false;
                        } else {
                            this.onSaveableLink = true;
                        }
                    } else {
                        root = root.parentNode;
                    }
                }
            }
        }

        
        this.onMetaDataItem = this.onImage;
        
        
        const NS_MathML = "http://www.w3.org/1998/Math/MathML";
        if ((this.target.nodeType == Node.TEXT_NODE &&
             this.target.parentNode.namespaceURI == NS_MathML)
             || (this.target.namespaceURI == NS_MathML))
          this.onMathML = true;

        
        if ( this.target.ownerDocument != window.content.document ) {
            this.inFrame = true;
        }
        
        
        const XMLNS = "http://www.w3.org/XML/1998/namespace";
        var elem = this.target;
        while ( elem ) {
            if ( elem.nodeType == Node.ELEMENT_NODE ) {
                
                if ( !this.onLink && 
                    ( (elem instanceof HTMLAnchorElement && elem.href) ||
                      elem instanceof HTMLAreaElement ||
                      elem instanceof HTMLLinkElement ||
                      elem.getAttributeNS( "http://www.w3.org/1999/xlink", "type") == "simple" ) ) {
                    
                    this.onLink = true;
                    this.onMetaDataItem = true;
                    
                    this.link = elem;
                    this.onMailtoLink = this.isLinkType( "mailto:", this.link );
                    
                    this.onSaveableLink = this.isLinkSaveable( this.link );
                }
                
                
                if ( !this.onTextInput ) {
                    
                    this.onTextInput = this.isTargetATextBox(elem);
                }
                
                
                if ( !this.onMetaDataItem ) {
                    
                    
                    if ( ( elem instanceof HTMLQuoteElement && elem.cite)    ||
                         ( elem instanceof HTMLTableElement && elem.summary) ||
                         ( elem instanceof HTMLModElement &&
                             ( elem.cite || elem.dateTime ) )                ||
                         ( elem instanceof HTMLElement &&
                             ( elem.title || elem.lang ) )                   ||
                         elem.getAttributeNS(XMLNS, "lang") ) {
                        dump("On metadata item.\n");
                        this.onMetaDataItem = true;
                    }
                }

                
                
                
                if ( !this.hasBGImage ) {
                    var bgImgUrl = this.getComputedURL( elem, "background-image" );
                    if ( bgImgUrl ) {
                        this.hasBGImage = true;
                        this.bgImageURL = this.makeURLAbsolute( elem.baseURI,
                                                                bgImgUrl );
                    }
                }
            }
            elem = elem.parentNode;    
        }
    },
    initPopupURL: function() {
      
      if (!window.content.opener)
        return;
      try {
        var show = false;
        
        const CI = Components.interfaces;
        var xulwin = window
                    .QueryInterface(CI.nsIInterfaceRequestor)
                    .getInterface(CI.nsIWebNavigation)
                    .QueryInterface(CI.nsIDocShellTreeItem)
                    .treeOwner
                    .QueryInterface(CI.nsIInterfaceRequestor)
                    .getInterface(CI.nsIXULWindow);
        if (xulwin.contextFlags &
            CI.nsIWindowCreator2.PARENT_IS_LOADING_OR_RUNNING_TIMEOUT) {
          
          const PB = Components.classes["@mozilla.org/preferences-service;1"]
                     .getService(CI.nsIPrefBranch);
          show = !PB.getBoolPref("dom.disable_open_during_load");
        }
        if (show) {
          
          const IOS = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(CI.nsIIOService);
          this.popupURL = IOS.newURI(window.content.opener.location.href, null, null);

          
          const PM = Components.classes["@mozilla.org/PopupWindowManager;1"]
                     .getService(CI.nsIPopupWindowManager);
        }
      } catch(e) {
      }
    },
    
    getComputedStyle: function( elem, prop ) {
         return elem.ownerDocument.defaultView.getComputedStyle( elem, '' ).getPropertyValue( prop );
    },
    
    getComputedURL: function( elem, prop ) {
         var url = elem.ownerDocument.defaultView.getComputedStyle( elem, '' ).getPropertyCSSValue( prop );
         return ( url.primitiveType == CSSPrimitiveValue.CSS_URI ) ? url.getStringValue() : null;
    },
    
    isLinkSaveable : function ( link ) {
        
        
        return !(this.isLinkType( "mailto:" , link )     ||
                 this.isLinkType( "javascript:" , link ) ||
                 this.isLinkType( "news:", link )        || 
                 this.isLinkType( "snews:", link ) ); 
    },
    
    isLinkType : function ( linktype, link ) {        
        try {
            
            if ( !link.protocol ) {
                
                var protocol;
                if ( link.href ) {
                    protocol = link.href.substr( 0, linktype.length );
                } else {
                    protocol = link.getAttributeNS("http://www.w3.org/1999/xlink","href");
                    if ( protocol ) {
                        protocol = protocol.substr( 0, linktype.length );
                    }
                }
                return protocol.toLowerCase() === linktype;        
            } else {
                
                return link.protocol.toLowerCase() === linktype;
            }
        } catch (e) {
            
            
            return false;
        }
    },
    
    rejectPopupWindows: function(andClose) {
      const PM = Components.classes["@mozilla.org/PopupWindowManager;1"]
                 .getService(Components.interfaces.nsIPopupWindowManager);
      PM.add(this.popupURL, false);
      if (andClose) {
        const OS = Components.classes["@mozilla.org/observer-service;1"]
                   .getService(Components.interfaces.nsIObserverService);
        OS.notifyObservers(window, "popup-perm-close", this.popupURL.spec);
      }
    },
    
    allowPopupWindows: function() {
      const PM = Components.classes["@mozilla.org/PopupWindowManager;1"]
                 .getService(Components.interfaces.nsIPopupWindowManager);
      PM.add(this.popupURL, true);
    },
    
    openLink : function () {
        
        openNewWindowWith( this.linkURL(), true );
    },
    
    openLinkInTab : function ( reverseBackgroundPref ) {
        
        openNewTabWith( this.linkURL(), true, reverseBackgroundPref );
    },
    
    openFrameInTab : function ( reverseBackgroundPref ) {
        
        openNewTabWith( this.target.ownerDocument.location.href, true, reverseBackgroundPref );
    },
    
    reloadFrame : function () {
        this.target.ownerDocument.location.reload();
    },
    
    openFrame : function () {
        openNewWindowWith( this.target.ownerDocument.location.href );
    },
    
    showOnlyThisFrame : function () {
        window.loadURI(this.target.ownerDocument.location.href);
    },
    
    viewPartialSource : function ( context ) {
        var focusedWindow = document.commandDispatcher.focusedWindow;
        if (focusedWindow == window)
          focusedWindow = content;
        var docCharset = null;
        if (focusedWindow)
          docCharset = "charset=" + focusedWindow.document.characterSet;

        
        
        
        var reference = null;
        if (context == "selection")
          reference = focusedWindow.getSelection();
        else if (context == "mathml")
          reference = this.target;
        else
          throw "not reached";

        var docUrl = null; 
        window.openDialog("chrome://navigator/content/viewPartialSource.xul",
                          "_blank", "scrollbars,resizable,chrome,dialog=no",
                          docUrl, docCharset, reference, context);
    },
    
    viewFrameSource : function () {
        BrowserViewSourceOfDocument(this.target.ownerDocument);
    },
    viewInfo : function () {
        BrowserPageInfo();
    },
    viewFrameInfo : function () {
        BrowserPageInfo(this.target.ownerDocument);
    },
    toggleImageSize : function () {
        content.document.toggleImageSize();
    },
    
    reloadImage : function () {
        urlSecurityCheck( this.imageURL, document );
        if (this.target instanceof Components.interfaces.nsIImageLoadingContent)
          this.target.forceReload();
    },
    
    viewImage : function () {
        urlSecurityCheck( this.imageURL, document );
        openTopWin( this.imageURL );
    },
    
    viewBGImage : function () {
        urlSecurityCheck( this.bgImageURL, document );
        openTopWin( this.bgImageURL );
    },
    setWallpaper: function() {
      
      var promptService       = Components.classes["@mozilla.org/embedcomp/prompt-service;1"].getService(Components.interfaces.nsIPromptService);
      var gNavigatorBundle    = document.getElementById("bundle_navigator");
      var promptTitle         = gNavigatorBundle.getString("wallpaperConfirmTitle");
      var promptMsg           = gNavigatorBundle.getString("wallpaperConfirmMsg");
      var promptConfirmButton = gNavigatorBundle.getString("wallpaperConfirmButton");

      var buttonPressed = promptService.confirmEx(window, promptTitle, promptMsg,
                                                   (promptService.BUTTON_TITLE_IS_STRING * promptService.BUTTON_POS_0) +
                                                   (promptService.BUTTON_TITLE_CANCEL    * promptService.BUTTON_POS_1),
                                                   promptConfirmButton, null, null, null, {value:0});
 
      if (buttonPressed != 0)
        return;

      var winhooks = Components.classes[ "@mozilla.org/winhooks;1" ].
                       getService(Components.interfaces.nsIWindowsHooks);
      
      winhooks.setImageAsWallpaper(this.target, false);
    },    
    
    saveFrame : function () {
        saveDocument( this.target.ownerDocument );
    },
    
    saveLink : function () {
        saveURL( this.linkURL(), this.linkText(), null, true,
                 getReferrer(document) );
    },
    
    saveImage : function () {
        
        
        saveImageURL( this.imageURL, null, "SaveImageTitle", false,
                      getReferrer(document) );
    },
    
    getEmail : function () {
        
        
        
        var addresses;
        try {
          
          var characterSet = this.target.ownerDocument.characterSet;
          const textToSubURI = Components.classes["@mozilla.org/intl/texttosuburi;1"]
                                         .getService(Components.interfaces.nsITextToSubURI);
          addresses = this.linkURL().match(/^mailto:([^?]+)/)[1];
          addresses = textToSubURI.unEscapeURIForUI(characterSet, addresses);
        }
        catch(ex) {
          
        }
        return addresses;
    },
    
    copyEmail : function () {
        var clipboard = this.getService( "@mozilla.org/widget/clipboardhelper;1",
                                         Components.interfaces.nsIClipboardHelper );
        clipboard.copyString(this.getEmail());
    },    
    addBookmark : function() {
      var docshell = document.getElementById( "content" ).webNavigation;
      BookmarksUtils.addBookmark( docshell.currentURI.spec,
                                  docshell.document.title,
                                  docshell.document.characterSet,
                                  false );
    },
    addBookmarkForFrame : function() {
      var doc = this.target.ownerDocument;
      var uri = doc.location.href;
      var title = doc.title;
      if ( !title )
        title = uri;
      BookmarksUtils.addBookmark( uri,
                                  title,
                                  doc.characterSet,
                                  false );
    },
    
    showMetadata : function () {
        window.openDialog(  "chrome://navigator/content/metadata.xul",
                            "_blank",
                            "scrollbars,resizable,chrome,dialog=no",
                            this.target);
    },

    
    
    

    
    createInstance : function ( contractId, iidName ) {
        var iid = Components.interfaces[ iidName ];
        return Components.classes[ contractId ].createInstance( iid );
    },
    
    getService : function ( contractId, iidName ) {
        var iid = Components.interfaces[ iidName ];
        return Components.classes[ contractId ].getService( iid );
    },
    
    showItem : function ( itemOrId, show ) {
        var item = itemOrId.constructor == String ? document.getElementById(itemOrId) : itemOrId;
        if (item) 
          item.hidden = !show;
    },
    
    
    
    setItemAttr : function ( id, attr, val ) {
        var elem = document.getElementById( id );
        if ( elem ) {
            if ( val == null ) {
                
                elem.removeAttribute( attr );
            } else {
                
                elem.setAttribute( attr, val );
            }
        }
    },
    
    
    setItemAttrFromNode : function ( item_id, attr, other_id ) {
        var elem = document.getElementById( other_id );
        if ( elem && elem.getAttribute( attr ) == "true" ) {
            this.setItemAttr( item_id, attr, "true" );
        } else {
            this.setItemAttr( item_id, attr, null );
        }
    },
    
    cloneNode : function ( item ) {
        
        var node = document.createElement( item.tagName );

        
        var attrs = item.attributes;
        for ( var i = 0; i < attrs.length; i++ ) {
            var attr = attrs.item( i );
            node.setAttribute( attr.nodeName, attr.nodeValue );
        }

        
        return node;
    },
    
    linkURL : function () {
        if (this.link.href) {
          return this.link.href;
        }
        var href = this.link.getAttributeNS("http://www.w3.org/1999/xlink","href");
        if (!href || !href.match(/\S/)) {
          throw "Empty href"; 
        }
        href = this.makeURLAbsolute(this.link.baseURI,href);
        return href;
    },
    
    linkText : function () {
        var text = gatherTextUnder( this.link );
        if (!text || !text.match(/\S/)) {
          text = this.link.getAttribute("title");
          if (!text || !text.match(/\S/)) {
            text = this.link.getAttribute("alt");
            if (!text || !text.match(/\S/)) {
              if (this.link.href) {                
                text = this.link.href;
              } else {
                text = getAttributeNS("http://www.w3.org/1999/xlink", "href");
                if (text && text.match(/\S/)) {
                  text = this.makeURLAbsolute(this.link.baseURI, text);
                }
              }
            }
          }
        }

        return text;
    },

    
    
    isTextSelection : function() {
        var result = false;
        var selection = this.searchSelected(16);

        var bundle = srGetStrBundle("chrome://communicator/locale/contentAreaCommands.properties");

        var searchSelectText;
        if (selection != "") {
            searchSelectText = selection.toString();
            if (searchSelectText.length > 15)
                searchSelectText = searchSelectText.substr(0,15) + "...";
            result = true;

          
          searchSelectText = bundle.formatStringFromName("searchText",
                                                         [searchSelectText], 1);
          this.setItemAttr("context-searchselect", "label", searchSelectText);
        } 
        return result;
    },
    
    searchSelected : function( charlen ) {
        var focusedWindow = document.commandDispatcher.focusedWindow;
        var searchStr = focusedWindow.getSelection();
        searchStr = searchStr.toString();
        
        if (!charlen)
            charlen = 150;
        if (charlen < searchStr.length) {
            
            var pattern = new RegExp("^(?:\\s*.){0," + charlen + "}");
            pattern.test(searchStr);
            searchStr = RegExp.lastMatch;
        }
        searchStr = searchStr.replace(/^\s+/, "");
        searchStr = searchStr.replace(/\s+$/, "");
        searchStr = searchStr.replace(/\s+/g, " ");
        return searchStr;
    },
    
    
    makeURLAbsolute : function ( base, url ) {
        
        var ioService = Components.classes["@mozilla.org/network/io-service;1"]
                      .getService(Components.interfaces.nsIIOService);
        var baseURI  = ioService.newURI(base, null, null);
        
        return ioService.newURI(baseURI.resolve(url), null, null).spec;
    },
    toString : function () {
        return "contextMenu.target     = " + this.target + "\n" +
               "contextMenu.onImage    = " + this.onImage + "\n" +
               "contextMenu.onLink     = " + this.onLink + "\n" +
               "contextMenu.link       = " + this.link + "\n" +
               "contextMenu.inFrame    = " + this.inFrame + "\n" +
               "contextMenu.hasBGImage = " + this.hasBGImage + "\n";
    },
    isTargetATextBox : function ( node )
    {
      if (node instanceof HTMLInputElement)
        return (node.type == "text" || node.type == "password")

      return (node instanceof HTMLTextAreaElement);
    },

    
    
    
    shouldShowSeparator : function ( aSeparatorID )
    {
      var separator = document.getElementById(aSeparatorID);
      if (separator) {
        var sibling = separator.previousSibling;
        while (sibling && sibling.localName != "menuseparator") {
          if (sibling.getAttribute("hidden") != "true")
            return true;
          sibling = sibling.previousSibling;
        }
      }
      return false;  
    }
};






function nsDefaultEngine()
{
    try
    {
        var pb = Components.classes["@mozilla.org/preferences-service;1"].
                   getService(Components.interfaces.nsIPrefBranch);
        var pbi = pb.QueryInterface(
                    Components.interfaces.nsIPrefBranch2);
        pbi.addObserver(this.domain, this, false);

        
        
        this.observe(pb, "", this.domain);
    }
    catch (ex)
    {
    }
}

nsDefaultEngine.prototype = 
{
    name: "",
    icon: "",
    domain: "browser.search.defaultengine",

    
    observe: function(aPrefBranch, aTopic, aPrefName)
    {
        try
        {
            var rdf = Components.
                        classes["@mozilla.org/rdf/rdf-service;1"].
                        getService(Components.interfaces.nsIRDFService);
            var ds = rdf.GetDataSource("rdf:internetsearch");
            var defaultEngine = aPrefBranch.getCharPref(aPrefName);
            var res = rdf.GetResource(defaultEngine);

            
            const kNC_Name = rdf.GetResource(
                               "http://home.netscape.com/NC-rdf#Name");
            var engineName = ds.GetTarget(res, kNC_Name, true);
            if (engineName)
            {
                this.name = engineName.QueryInterface(
                              Components.interfaces.nsIRDFLiteral).Value;
            }

            
            const kNC_Icon = rdf.GetResource(
                               "http://home.netscape.com/NC-rdf#Icon");
            var iconURL = ds.GetTarget(res, kNC_Icon, true);
            if (iconURL)
            {
                this.icon = iconURL.QueryInterface(
                  Components.interfaces.nsIRDFLiteral).Value;
            }
        }
        catch (ex)
        {
        }
    }
}
