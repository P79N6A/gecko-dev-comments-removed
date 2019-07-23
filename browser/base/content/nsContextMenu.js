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
# The Original Code is mozilla.org code.
#
# The Initial Developer of the Original Code is
# Netscape Communications Corporation.
# Portions created by the Initial Developer are Copyright (C) 1998
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Blake Ross <blake@cs.stanford.edu>
#   David Hyatt <hyatt@mozilla.org>
#   Peter Annema <disttsc@bart.nl>
#   Dean Tessman <dean_tessman@hotmail.com>
#   Kevin Puetz <puetzk@iastate.edu>
#   Ben Goodger <ben@netscape.com>
#   Pierre Chanial <chanial@noos.fr>
#   Jason Eager <jce2@po.cwru.edu>
#   Joe Hewitt <hewitt@netscape.com>
#   Alec Flett <alecf@netscape.com>
#   Asaf Romano <mozilla.mano@sent.com>
#   Jason Barnabe <jason_barnabe@fastmail.fm>
#   Peter Parente <parente@cs.unc.edu>
#   Giorgio Maone <g.maone@informaction.com>
#   Tom Germeau <tom.germeau@epigoon.com>
#   Jesse Ruderman <jruderman@gmail.com>
#   Joe Hughes <joe@retrovirus.com>
#   Pamela Greene <pamg.bugs@gmail.com>
#   Michael Ventnor <ventnors_dogs234@yahoo.com.au>
#   Simon BÃ¼nzli <zeniko@gmail.com>
#   Gijs Kruitbosch <gijskruitbosch@gmail.com>
#   Ehsan Akhgari <ehsan.akhgari@gmail.com>
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

function nsContextMenu(aXulMenu, aBrowser) {
  this.target            = null;
  this.browser           = null;
  this.menu              = null;
  this.isFrameImage      = false;
  this.onTextInput       = false;
  this.onKeywordField    = false;
  this.onImage           = false;
  this.onLoadedImage     = false;
  this.onCanvas          = false;
  this.onLink            = false;
  this.onMailtoLink      = false;
  this.onSaveableLink    = false;
  this.onMetaDataItem    = false;
  this.onMathML          = false;
  this.link              = false;
  this.linkURL           = "";
  this.linkURI           = null;
  this.linkProtocol      = null;
  this.inFrame           = false;
  this.hasBGImage        = false;
  this.isTextSelected    = false;
  this.isContentSelected = false;
  this.inDirList         = false;
  this.shouldDisplay     = true;
  this.isDesignMode      = false;
  this.possibleSpellChecking = false;
  this.ellipsis = "\u2026";
  try {
    this.ellipsis = gPrefService.getComplexValue("intl.ellipsis",
                                                 Ci.nsIPrefLocalizedString).data;
  } catch (e) { }

  
  this.initMenu(aXulMenu, aBrowser);
}


nsContextMenu.prototype = {
  
  onDestroy: function () {
  },

  
  initMenu: function CM_initMenu(aPopup, aBrowser) {
    this.menu = aPopup;
    this.browser = aBrowser;

    this.isFrameImage = document.getElementById("isFrameImage");

    
    this.setTarget(document.popupNode, document.popupRangeParent,
                   document.popupRangeOffset);

    this.isTextSelected = this.isTextSelection();
    this.isContentSelected = this.isContentSelection();

    
    this.initItems();
  },

  initItems: function CM_initItems() {
    this.initOpenItems();
    this.initNavigationItems();
    this.initViewItems();
    this.initMiscItems();
    this.initSpellingItems();
    this.initSaveItems();
    this.initClipboardItems();
    this.initMetadataItems();
  },

  initOpenItems: function CM_initOpenItems() {
    var shouldShow = this.onSaveableLink ||
                     (this.inDirList && this.onLink);
    this.showItem("context-openlink", shouldShow);
    this.showItem("context-openlinkintab", shouldShow);
    this.showItem("context-sep-open", shouldShow);
  },

  initNavigationItems: function CM_initNavigationItems() {
    var shouldShow = !(this.isContentSelected || this.onLink || this.onImage ||
                       this.onCanvas || this.onTextInput);
    this.showItem("context-back", shouldShow);
    this.showItem("context-forward", shouldShow);
    this.showItem("context-reload", shouldShow);
    this.showItem("context-stop", shouldShow);
    this.showItem("context-sep-stop", shouldShow);

    
    
  },

  initSaveItems: function CM_initSaveItems() {
    var shouldShow = !(this.inDirList || this.onTextInput || this.onLink ||
                       this.isContentSelected || this.onImage || this.onCanvas);
    this.showItem("context-savepage", shouldShow);
    this.showItem("context-sendpage", shouldShow);

    
    this.showItem("context-savelink", this.onSaveableLink);
    this.showItem("context-sendlink", this.onSaveableLink);

    
    this.showItem("context-saveimage", this.onLoadedImage || this.onCanvas);
    
    this.showItem("context-sendimage", this.onImage);
  },

  initViewItems: function CM_initViewItems() {
    
    this.showItem("context-viewpartialsource-selection",
                  this.isContentSelected);
    this.showItem("context-viewpartialsource-mathml",
                  this.onMathML && !this.isContentSelected);

    var shouldShow = !(this.inDirList || this.isContentSelected ||
                       this.onImage || this.onLink || this.onTextInput);
    this.showItem("context-viewsource", shouldShow);
    this.showItem("context-viewinfo", shouldShow);

    this.showItem("context-sep-properties",
                  !(this.inDirList || this.isContentSelected ||
                    this.onTextInput));

    
    
    var haveSetDesktopBackground = false;
#ifdef HAVE_SHELL_SERVICE
    
    var shell = getShellService();
    if (shell)
      haveSetDesktopBackground = true;
#endif
    this.showItem("context-setDesktopBackground",
                  haveSetDesktopBackground && this.onLoadedImage);

    if (haveSetDesktopBackground && this.onLoadedImage) {
      document.getElementById("context-setDesktopBackground")
              .disabled = this.disableSetDesktopBackground();
    }

    
    
    this.showItem("context-viewimage", (this.onImage &&
                  (!this.onStandaloneImage || this.inFrame)) || this.onCanvas);

    
    this.showItem("context-viewbgimage", shouldShow);
    this.showItem("context-sep-viewbgimage", shouldShow);
    document.getElementById("context-viewbgimage")
            .disabled = !this.hasBGImage;
  },

  initMiscItems: function CM_initMiscItems() {
    
    this.showItem("context-bookmarkpage",
                  !(this.isContentSelected || this.onTextInput ||
                    this.onLink || this.onImage));
    this.showItem("context-bookmarklink", this.onLink && !this.onMailtoLink);
    this.showItem("context-searchselect", this.isTextSelected);
    this.showItem("context-keywordfield",
                  this.onTextInput && this.onKeywordField);
    this.showItem("frame", this.inFrame);
    this.showItem("frame-sep", this.inFrame);

    
    if (this.inFrame) {
      if (mimeTypeIsTextBased(this.target.ownerDocument.contentType))
        this.isFrameImage.removeAttribute('hidden');
      else
        this.isFrameImage.setAttribute('hidden', 'true');
    }

    
    this.showItem("context-sep-bidi", top.gBidiUI);
    this.showItem("context-bidi-text-direction-toggle",
                  this.onTextInput && top.gBidiUI);
    this.showItem("context-bidi-page-direction-toggle",
                  !this.onTextInput && top.gBidiUI);

    if (this.onImage) {
      var blockImage = document.getElementById("context-blockimage");

      var uri = this.target
                    .QueryInterface(Ci.nsIImageLoadingContent)
                    .currentURI;

      
      
      var hostLabel = "";
      try {
        hostLabel = uri.host;
      } catch (ex) { }

      if (hostLabel) {
        var shortenedUriHost = hostLabel.replace(/^www\./i,"");
        if (shortenedUriHost.length > 15)
          shortenedUriHost = shortenedUriHost.substr(0,15) + this.ellipsis;
        blockImage.label = gNavigatorBundle.getFormattedString("blockImages", [shortenedUriHost]);

        if (this.isImageBlocked())
          blockImage.setAttribute("checked", "true");
        else
          blockImage.removeAttribute("checked");
      }
    }

    
    this.showItem("context-blockimage", this.onImage && hostLabel);
  },

  initSpellingItems: function() {
    var canSpell = InlineSpellCheckerUI.canSpellCheck;
    var onMisspelling = InlineSpellCheckerUI.overMisspelling;
    this.showItem("spell-check-enabled", canSpell);
    this.showItem("spell-separator", canSpell || this.possibleSpellChecking);
    if (canSpell) {
      document.getElementById("spell-check-enabled")
              .setAttribute("checked", InlineSpellCheckerUI.enabled);
    }

    this.showItem("spell-add-to-dictionary", onMisspelling);

    
    this.showItem("spell-suggestions-separator", onMisspelling);
    if (onMisspelling) {
      var menu = document.getElementById("contentAreaContextMenu");
      var suggestionsSeparator =
        document.getElementById("spell-add-to-dictionary");
      var numsug = InlineSpellCheckerUI.addSuggestionsToMenu(menu, suggestionsSeparator, 5);
      this.showItem("spell-no-suggestions", numsug == 0);
    }
    else
      this.showItem("spell-no-suggestions", false);

    
    this.showItem("spell-dictionaries", InlineSpellCheckerUI.enabled);
    if (canSpell) {
      var dictMenu = document.getElementById("spell-dictionaries-menu");
      var dictSep = document.getElementById("spell-language-separator");
      InlineSpellCheckerUI.addDictionaryListToMenu(dictMenu, dictSep);
      this.showItem("spell-add-dictionaries-main", false);
    }
    else if (this.possibleSpellChecking) {
      
      
      
      this.showItem("spell-add-dictionaries-main", true);
    }
    else
      this.showItem("spell-add-dictionaries-main", false);
  },

  initClipboardItems: function() {
    
    
    
    
    goUpdateGlobalEditMenuItems();

    this.showItem("context-undo", this.onTextInput);
    this.showItem("context-sep-undo", this.onTextInput);
    this.showItem("context-cut", this.onTextInput);
    this.showItem("context-copy",
                  this.isContentSelected || this.onTextInput);
    this.showItem("context-paste", this.onTextInput);
    this.showItem("context-delete", this.onTextInput);
    this.showItem("context-sep-paste", this.onTextInput);
    this.showItem("context-selectall",
                  !(this.onLink || this.onImage) || this.isDesignMode);
    this.showItem("context-sep-selectall", this.isContentSelected );

    
    
    
    

    
    this.showItem("context-copyemail", this.onMailtoLink);

    
    this.showItem("context-copylink", this.onLink);
    this.showItem("context-sep-copylink", this.onLink && this.onImage);

#ifdef CONTEXT_COPY_IMAGE_CONTENTS
    
    this.showItem("context-copyimage-contents", this.onImage);
#endif
    
    this.showItem("context-copyimage", this.onImage);
    this.showItem("context-sep-copyimage", this.onImage);
  },

  initMetadataItems: function() {
    
    this.showItem("context-metadata", this.onMetaDataItem);
  },

  
  setTarget: function (aNode, aRangeParent, aRangeOffset) {
    const xulNS = "http://www.mozilla.org/keymaster/gatekeeper/there.is.only.xul";
    if (aNode.namespaceURI == xulNS) {
      this.shouldDisplay = false;
      return;
    }

    
    this.onImage           = false;
    this.onLoadedImage     = false;
    this.onStandaloneImage = false;
    this.onCanvas          = false;
    this.onMetaDataItem    = false;
    this.onTextInput       = false;
    this.onKeywordField    = false;
    this.imageURL          = "";
    this.onLink            = false;
    this.linkURL           = "";
    this.linkURI           = null;
    this.linkProtocol      = "";
    this.onMathML          = false;
    this.inFrame           = false;
    this.hasBGImage        = false;
    this.bgImageURL        = "";
    this.possibleSpellChecking = false;

    
    
    
    
    
    InlineSpellCheckerUI.clearSuggestionsFromMenu();
    InlineSpellCheckerUI.clearDictionaryListFromMenu();

    InlineSpellCheckerUI.uninit();

    
    this.target = aNode;

    
    if (this.target.nodeType == Node.ELEMENT_NODE) {
      
      if (this.target instanceof Ci.nsIImageLoadingContent &&
          this.target.currentURI) {
        this.onImage = true;
        this.onMetaDataItem = true;
                
        var request =
          this.target.getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
        if (request && (request.imageStatus & request.STATUS_SIZE_AVAILABLE))
          this.onLoadedImage = true;

        this.imageURL = this.target.currentURI.spec;
        if (this.target.ownerDocument instanceof ImageDocument)
          this.onStandaloneImage = true;
      }
      else if (this.target instanceof HTMLCanvasElement) {
        this.onCanvas = true;
      }
      else if (this.target instanceof HTMLInputElement ) {
        this.onTextInput = this.isTargetATextBox(this.target);
        
        if (this.onTextInput && ! this.target.readOnly &&
            this.target.type != "password") {
          this.possibleSpellChecking = true;
          InlineSpellCheckerUI.init(this.target.QueryInterface(Ci.nsIDOMNSEditableElement).editor);
          InlineSpellCheckerUI.initFromEvent(aRangeParent, aRangeOffset);
        }
        this.onKeywordField = this.isTargetAKeywordField(this.target);
      }
      else if (this.target instanceof HTMLTextAreaElement) {
        this.onTextInput = true;
        if (!this.target.readOnly) {
          this.possibleSpellChecking = true;
          InlineSpellCheckerUI.init(this.target.QueryInterface(Ci.nsIDOMNSEditableElement).editor);
          InlineSpellCheckerUI.initFromEvent(aRangeParent, aRangeOffset);
        }
      }
      else if (this.target instanceof HTMLHtmlElement) {
        var bodyElt = this.target.ownerDocument.body;
        if (bodyElt) {
          var computedURL = this.getComputedURL(bodyElt, "background-image");
          if (computedURL) {
            this.hasBGImage = true;
            this.bgImageURL = makeURLAbsolute(bodyElt.baseURI,
                                              computedURL);
          }
        }
      }
      else if ("HTTPIndex" in content &&
               content.HTTPIndex instanceof Ci.nsIHTTPIndex) {
        this.inDirList = true;
        
        
        var root = this.target;
        while (root && !this.link) {
          if (root.tagName == "tree") {
            
            
            break;
          }

          if (root.getAttribute("URL")) {
            
            this.onLink = true;
            this.link = { href : root.getAttribute("URL"),
                          getAttribute: function (aAttr) {
                            if (aAttr == "title") {
                              return root.firstChild.firstChild
                                         .getAttribute("label");
                            }
                            else
                              return "";
                           }
                         };

            
            this.onSaveableLink = root.getAttribute("container") != "true";
          }
          else
            root = root.parentNode;
        }
      }
    }

    
    
    const XMLNS = "http://www.w3.org/XML/1998/namespace";
    var elem = this.target;
    while (elem) {
      if (elem.nodeType == Node.ELEMENT_NODE) {
        
        if (!this.onLink &&
             ((elem instanceof HTMLAnchorElement && elem.href) ||
              (elem instanceof HTMLAreaElement && elem.href) ||
              elem instanceof HTMLLinkElement ||
              elem.getAttributeNS("http://www.w3.org/1999/xlink", "type") == "simple")) {
            
          
          this.onLink = true;
          this.onMetaDataItem = true;

          
          
          
          var realLink = elem;
          var parent = elem.parentNode;
          while (parent) {
            try {
              if ((parent instanceof HTMLAnchorElement && parent.href) ||
                  (parent instanceof HTMLAreaElement && parent.href) ||
                  parent instanceof HTMLLinkElement ||
                  parent.getAttributeNS("http://www.w3.org/1999/xlink", "type") == "simple")
                realLink = parent;
            } catch (e) { }
            parent = parent.parentNode;
          }
          
          
          this.link = realLink;
          this.linkURL = this.getLinkURL();
          this.linkURI = this.getLinkURI();
          this.linkProtocol = this.getLinkProtocol();
          this.onMailtoLink = (this.linkProtocol == "mailto");
          this.onSaveableLink = this.isLinkSaveable( this.link );
        }

        
        if (!this.onMetaDataItem) {
          
          
          
          if ((elem instanceof HTMLQuoteElement && elem.cite) ||
              (elem instanceof HTMLTableElement && elem.summary) ||
              (elem instanceof HTMLModElement &&
               (elem.cite || elem.dateTime)) ||
              (elem instanceof HTMLElement &&
               (elem.title || elem.lang)) ||
              elem.getAttributeNS(XMLNS, "lang")) {
            this.onMetaDataItem = true;
          }
        }

        
        
        
        if (!this.hasBGImage) {
          var bgImgUrl = this.getComputedURL( elem, "background-image" );
          if (bgImgUrl) {
            this.hasBGImage = true;
            this.bgImageURL = makeURLAbsolute(elem.baseURI,
                                              bgImgUrl);
          }
        }
      }

      elem = elem.parentNode;
    }
    
    
    const NS_MathML = "http://www.w3.org/1998/Math/MathML";
    if ((this.target.nodeType == Node.TEXT_NODE &&
         this.target.parentNode.namespaceURI == NS_MathML)
         || (this.target.namespaceURI == NS_MathML))
      this.onMathML = true;

    
    var docDefaultView = this.target.ownerDocument.defaultView;
    if (docDefaultView != docDefaultView.top)
      this.inFrame = true;

    
    var win = this.target.ownerDocument.defaultView;
    if (win) {
      var isEditable = false;
      try {
        var editingSession = win.QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIWebNavigation)
                                .QueryInterface(Ci.nsIInterfaceRequestor)
                                .getInterface(Ci.nsIEditingSession);
        if (editingSession.windowIsEditable(win) &&
            this.getComputedStyle(this.target, "-moz-user-modify") == "read-write") {
          isEditable = true;
        }
      }
      catch(ex) {
        
      }

      if (isEditable) {
        this.onTextInput       = true;
        this.onKeywordField    = false;
        this.onImage           = false;
        this.onLoadedImage     = false;
        this.onMetaDataItem    = false;
        this.onMathML          = false;
        this.inFrame           = false;
        this.hasBGImage        = false;
        this.isDesignMode      = true;
        this.possibleSpellChecking = true;
        InlineSpellCheckerUI.init(editingSession.getEditorForWindow(win));
        var canSpell = InlineSpellCheckerUI.canSpellCheck;
        InlineSpellCheckerUI.initFromEvent(aRangeParent, aRangeOffset);
        this.showItem("spell-check-enabled", canSpell);
        this.showItem("spell-separator", canSpell);
      }
    }
  },

  
  getComputedStyle: function(aElem, aProp) {
    return aElem.ownerDocument
                .defaultView
                .getComputedStyle(aElem, "").getPropertyValue(aProp);
  },

  
  getComputedURL: function(aElem, aProp) {
    var url = aElem.ownerDocument
                   .defaultView.getComputedStyle(aElem, "")
                   .getPropertyCSSValue(aProp);
    return url.primitiveType == CSSPrimitiveValue.CSS_URI ?
           url.getStringValue() : null;
  },

  
  isLinkSaveable: function(aLink) {
    
    
    return this.linkProtocol && !(
             this.linkProtocol == "mailto"     ||
             this.linkProtocol == "javascript" ||
             this.linkProtocol == "news"       ||
             this.linkProtocol == "snews"      );
  },

  
  openLink : function () {
    openNewWindowWith(this.linkURL, this.target.ownerDocument, null, false);
  },

  
  openLinkInTab: function() {
    openNewTabWith(this.linkURL, this.target.ownerDocument, null, null, false);
  },

  
  openFrameInTab: function() {
    var doc = this.target.ownerDocument;
    var frameURL = doc.documentURIObject.spec;
    var referrer = doc.referrer;

    openNewTabWith(frameURL, null, null, null, false,
                   referrer ? makeURI(referrer) : null);
  },

  
  reloadFrame: function() {
    this.target.ownerDocument.location.reload();
  },

  
  openFrame: function() {
    var doc = this.target.ownerDocument;
    var frameURL = doc.documentURIObject.spec;
    var referrer = doc.referrer;

    openNewWindowWith(frameURL, null, null, false,
                      referrer ? makeURI(referrer) : null);
  },

  
  showOnlyThisFrame: function() {
    var doc = this.target.ownerDocument;
    var frameURL = doc.documentURIObject.spec;

    urlSecurityCheck(frameURL, this.browser.contentPrincipal,
                     Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
    var referrer = doc.referrer;
    this.browser.loadURI(frameURL, referrer ? makeURI(referrer) : null);
  },

  
  viewPartialSource: function(aContext) {
    var focusedWindow = document.commandDispatcher.focusedWindow;
    if (focusedWindow == window)
      focusedWindow = content;

    var docCharset = null;
    if (focusedWindow)
      docCharset = "charset=" + focusedWindow.document.characterSet;

    
    
    
    var reference = null;
    if (aContext == "selection")
      reference = focusedWindow.getSelection();
    else if (aContext == "mathml")
      reference = this.target;
    else
      throw "not reached";

    
    var docUrl = null;
    window.openDialog("chrome://global/content/viewPartialSource.xul",
                      "_blank", "scrollbars,resizable,chrome,dialog=no",
                      docUrl, docCharset, reference, aContext);
  },

  
  viewFrameSource: function() {
    BrowserViewSourceOfDocument(this.target.ownerDocument);
  },

  viewInfo: function() {
    BrowserPageInfo(this.target.ownerDocument.defaultView.top.document);
  },

  viewFrameInfo: function() {
    BrowserPageInfo(this.target.ownerDocument);
  },

  
  viewImage: function(e) {
    var viewURL;

    if (this.onCanvas)
      viewURL = this.target.toDataURL();
    else {
      viewURL = this.imageURL;
      urlSecurityCheck(viewURL,
                       this.browser.contentPrincipal,
                       Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
    }

    var doc = this.target.ownerDocument;
    openUILink(viewURL, e, null, null, null, null, doc.documentURIObject );
  },

  
  viewBGImage: function(e) {
    urlSecurityCheck(this.bgImageURL,
                     this.browser.contentPrincipal,
                     Ci.nsIScriptSecurityManager.DISALLOW_SCRIPT);
    var doc = this.target.ownerDocument;
    openUILink(this.bgImageURL, e, null, null, null, null, doc.documentURIObject );
  },

  disableSetDesktopBackground: function() {
    
    
    if (!(this.target instanceof Ci.nsIImageLoadingContent))
      return true;

    if (("complete" in this.target) && !this.target.complete)
      return true;

    if (this.target.currentURI.schemeIs("javascript"))
      return true;

    var request = this.target
                      .QueryInterface(Ci.nsIImageLoadingContent)
                      .getRequest(Ci.nsIImageLoadingContent.CURRENT_REQUEST);
    if (!request)
      return true;

    return false;
  },

  setDesktopBackground: function() {
    
    
    if (this.disableSetDesktopBackground())
      return;

    urlSecurityCheck(this.target.currentURI.spec,
                     this.target.ownerDocument.nodePrincipal);

    
    const kDesktopBackgroundURL = 
                  "chrome://browser/content/setDesktopBackground.xul";
#ifdef XP_MACOSX
    
    
    var wm = Components.classes["@mozilla.org/appshell/window-mediator;1"]
                       .getService(Components.interfaces.nsIWindowMediator);
    var dbWin = wm.getMostRecentWindow("Shell:SetDesktopBackground");
    if (dbWin) {
      dbWin.gSetBackground.init(this.target);
      dbWin.focus();
    }
    else {
      openDialog(kDesktopBackgroundURL, "",
                 "centerscreen,chrome,dialog=no,dependent,resizable=no",
                 this.target);
    }
#else
    
    openDialog(kDesktopBackgroundURL, "",
               "centerscreen,chrome,dialog,modal,dependent",
               this.target);
#endif
  },

  
  saveFrame: function () {
    saveDocument(this.target.ownerDocument);
  },

  
  saveLink: function() {
    var doc =  this.target.ownerDocument;
    urlSecurityCheck(this.linkURL, doc.nodePrincipal);
    saveURL(this.linkURL, this.linkText(), null, true, false,
            doc.documentURIObject);
  },

  sendLink: function() {
    
    MailIntegration.sendMessage( this.linkURL, "" );
  },

  
  saveImage: function() {
    var doc =  this.target.ownerDocument;
    if (this.onCanvas) {
      
      saveImageURL(this.target.toDataURL(), "canvas.png", "SaveImageTitle",
                   true, false, doc.documentURIObject);
    }
    else {
      urlSecurityCheck(this.imageURL, doc.nodePrincipal);
      saveImageURL(this.imageURL, null, "SaveImageTitle", false,
                   false, doc.documentURIObject);
    }
  },

  sendImage: function() {
    MailIntegration.sendMessage(this.imageURL, "");
  },

  toggleImageBlocking: function(aBlock) {
    var permissionmanager = Cc["@mozilla.org/permissionmanager;1"].
                            getService(Ci.nsIPermissionManager);

    var uri = this.target.QueryInterface(Ci.nsIImageLoadingContent).currentURI;

    if (aBlock)
      permissionmanager.add(uri, "image", Ci.nsIPermissionManager.DENY_ACTION);
    else
      permissionmanager.remove(uri.host, "image");

    var brandBundle = document.getElementById("bundle_brand");
    var app = brandBundle.getString("brandShortName");
    var bundle_browser = document.getElementById("bundle_browser");
    var message = bundle_browser.getFormattedString(aBlock ?
     "imageBlockedWarning" : "imageAllowedWarning", [app, uri.host]);

    var notificationBox = this.browser.getNotificationBox();
    var notification = notificationBox.getNotificationWithValue("images-blocked");

    if (notification)
      notification.label = message;
    else {
      var self = this;
      var buttons = [{
        label: bundle_browser.getString("undo"),
        accessKey: bundle_browser.getString("undo.accessKey"),
        callback: function() { self.toggleImageBlocking(!aBlock); }
      }];
      const priority = notificationBox.PRIORITY_WARNING_MEDIUM;
      notificationBox.appendNotification(message, "images-blocked",
                                         "chrome://browser/skin/Info.png",
                                         priority, buttons);
    }

    
    BrowserReload();
  },

  isImageBlocked: function() {
    var permissionmanager = Cc["@mozilla.org/permissionmanager;1"].
                            getService(Ci.nsIPermissionManager);

    var uri = this.target.QueryInterface(Ci.nsIImageLoadingContent).currentURI;

    return permissionmanager.testPermission(uri, "image") == Ci.nsIPermissionManager.DENY_ACTION;
  },

  
  copyEmail: function() {
    
    
    
    var url = this.linkURL;
    var qmark = url.indexOf("?");
    var addresses;

    
    addresses = qmark > 7 ? url.substring(7, qmark) : url.substr(7);

    
    
    try {
      var characterSet = this.target.ownerDocument.characterSet;
      const textToSubURI = Cc["@mozilla.org/intl/texttosuburi;1"].
                           getService(Ci.nsITextToSubURI);
      addresses = textToSubURI.unEscapeURIForUI(characterSet, addresses);
    }
    catch(ex) {
      
    }

    var clipboard = Cc["@mozilla.org/widget/clipboardhelper;1"].
                    getService(Ci.nsIClipboardHelper);
    clipboard.copyString(addresses);
  },

  
  showMetadata: function () {
    window.openDialog("chrome://browser/content/metaData.xul",
                      "_blank",
                      "scrollbars,resizable,chrome,dialog=no",
                      this.target);
  },

  
  
  

  
  showItem: function(aItemOrId, aShow) {
    var item = aItemOrId.constructor == String ?
      document.getElementById(aItemOrId) : aItemOrId;
    if (item)
      item.hidden = !aShow;
  },

  
  
  
  setItemAttr: function(aID, aAttr, aVal ) {
    var elem = document.getElementById(aID);
    if (elem) {
      if (aVal == null) {
        
        elem.removeAttribute(aAttr);
      }
      else {
        
        elem.setAttribute(aAttr, aVal);
      }
    }
  },

  
  
  setItemAttrFromNode: function(aItem_id, aAttr, aOther_id) {
    var elem = document.getElementById(aOther_id);
    if (elem && elem.getAttribute(aAttr) == "true")
      this.setItemAttr(aItem_id, aAttr, "true");
    else
      this.setItemAttr(aItem_id, aAttr, null);
  },

  
  cloneNode: function(aItem) {
    
    var node = document.createElement(aItem.tagName);

    
    var attrs = aItem.attributes;
    for (var i = 0; i < attrs.length; i++) {
      var attr = attrs.item(i);
      node.setAttribute(attr.nodeName, attr.nodeValue);
    }

    
    return node;
  },

  
  getLinkURL: function() {
    var href = this.link.href;  
    if (href)
      return href;

    href = this.link.getAttributeNS("http://www.w3.org/1999/xlink",
                                    "href");

    if (!href || !href.match(/\S/)) {
      
      
      throw "Empty href";
    }

    return makeURLAbsolute(this.link.baseURI, href);
  },
  
  getLinkURI: function() {
    var ioService = Cc["@mozilla.org/network/io-service;1"].
                    getService(Ci.nsIIOService);
    try {
      return ioService.newURI(this.linkURL, null, null);
    }
    catch (ex) {
     
    }

    return null;
  },
  
  getLinkProtocol: function() {
    if (this.linkURI)
      return this.linkURI.scheme; 

    return null;
  },

  
  linkText: function() {
    var text = gatherTextUnder(this.link);
    if (!text || !text.match(/\S/)) {
      text = this.link.getAttribute("title");
      if (!text || !text.match(/\S/)) {
        text = this.link.getAttribute("alt");
        if (!text || !text.match(/\S/))
          text = this.linkURL;
      }
    }

    return text;
  },

  
  isTextSelection: function() {
    
    
    var selectedText = getBrowserSelection(16);

    if (!selectedText)
      return false;

    if (selectedText.length > 15)
      selectedText = selectedText.substr(0,15) + this.ellipsis;

    
    
    var engineName = "";
    var ss = Cc["@mozilla.org/browser/search-service;1"].
             getService(Ci.nsIBrowserSearchService);
    if (isElementVisible(BrowserSearch.searchBar))
      engineName = ss.currentEngine.name;
    else
      engineName = ss.defaultEngine.name;

    
    var menuLabel = gNavigatorBundle.getFormattedString("contextMenuSearchText",
                                                        [engineName,
                                                         selectedText]);
    document.getElementById("context-searchselect").label = menuLabel;

    return true;
  },

  
  isContentSelection: function() {
    return !document.commandDispatcher.focusedWindow.getSelection().isCollapsed;
  },

  toString: function () {
    return "contextMenu.target     = " + this.target + "\n" +
           "contextMenu.onImage    = " + this.onImage + "\n" +
           "contextMenu.onLink     = " + this.onLink + "\n" +
           "contextMenu.link       = " + this.link + "\n" +
           "contextMenu.inFrame    = " + this.inFrame + "\n" +
           "contextMenu.hasBGImage = " + this.hasBGImage + "\n";
  },

  isTargetATextBox: function(node) {
    if (node instanceof HTMLInputElement)
      return (node.type == "text" || node.type == "password")

    return (node instanceof HTMLTextAreaElement);
  },

  isTargetAKeywordField: function(aNode) {
    var form = aNode.form;
    if (!form)
      return false;

    var method = form.method.toUpperCase();

    
    
    
    
    
    
    
    
    
    
    return (method == "GET" || method == "") ||
           (form.enctype != "text/plain") && (form.enctype != "multipart/form-data");
  },

  
  
  
  shouldShowSeparator: function (aSeparatorID) {
    var separator = document.getElementById(aSeparatorID);
    if (separator) {
      var sibling = separator.previousSibling;
      while (sibling && sibling.localName != "menuseparator") {
        if (!sibling.hidden)
          return true;
        sibling = sibling.previousSibling;
      }
    }
    return false;
  },

  addDictionaries: function() {
    var uri = formatURL("browser.dictionaries.download.url", true);

    var locale = "-";
    try {
      locale = gPrefService.getComplexValue("intl.accept_languages",
                                            Ci.nsIPrefLocalizedString).data;
    }
    catch (e) { }

    var version = "-";
    try {
      version = Cc["@mozilla.org/xre/app-info;1"].
                getService(Ci.nsIXULAppInfo).version;
    }
    catch (e) { }

    uri = uri.replace(/%LOCALE%/, escape(locale)).replace(/%VERSION%/, version);

    var newWindowPref = gPrefService.getIntPref("browser.link.open_newwindow");
    var where = newWindowPref == 3 ? "tab" : "window";

    openUILinkIn(uri, where);
  },

  bookmarkThisPage: function CM_bookmarkThisPage() {
    window.top.PlacesCommandHook.bookmarkPage(this.browser, PlacesUtils.bookmarksMenuFolderId, true);
  },

  bookmarkLink: function CM_bookmarkLink() {
    window.top.PlacesCommandHook.bookmarkLink(PlacesUtils.bookmarksMenuFolderId, this.linkURL,
                                              this.linkText());
  },

  addBookmarkForFrame: function CM_addBookmarkForFrame() {
    var doc = this.target.ownerDocument;
    var uri = doc.documentURIObject;

    var itemId = PlacesUtils.getMostRecentBookmarkForURI(uri);
    if (itemId == -1) {
      var title = doc.title;
      var description = PlacesUtils.getDescriptionFromDocument(doc);

      var descAnno = { name: DESCRIPTION_ANNO, value: description };
      var txn = PlacesUtils.ptm.createItem(uri, 
                                           PlacesUtils.bookmarksMenuFolderId,
                                           -1, title, null, [descAnno]);
      PlacesUtils.ptm.doTransaction(txn);
      itemId = PlacesUtils.getMostRecentBookmarkForURI(uri);
      StarUI.beginBatch();
    }

    window.top.StarUI.showEditBookmarkPopup(itemId, this.browser, "overlap");
  },

  savePageAs: function CM_savePageAs() {
    saveDocument(this.browser.contentDocument);
  },

  sendPage: function CM_sendPage() {
    MailIntegration.sendLinkForWindow(this.browser.contentWindow);  
  },

  printFrame: function CM_printFrame() {
    PrintUtils.print(this.target.ownerDocument.defaultView);
  },

  switchPageDirection: function CM_switchPageDirection() {
    SwitchDocumentDirection(this.browser.contentWindow);
  }
};
