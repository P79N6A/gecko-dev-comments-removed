



"use strict";
const {classes: Cc, interfaces: Ci, utils: Cu} = Components;

this.EXPORTED_SYMBOLS = ["Pocket"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI",
  "resource:///modules/CustomizableUI.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "ReaderMode",
  "resource://gre/modules/ReaderMode.jsm");

let Pocket = {
  get site() Services.prefs.getCharPref("browser.pocket.site"),
  get listURL() { return "https://" + Pocket.site + "/?src=ff_ext"; },

  


  onPanelViewShowing(event) {
    let document = event.target.ownerDocument;
    let window = document.defaultView;
    let iframe = window.pktUI.getPanelFrame();

    let urlToSave = Pocket._urlToSave;
    let titleToSave = Pocket._titleToSave;
    Pocket._urlToSave = null;
    Pocket._titleToSave = null;
    
    
    window.setTimeout(function() {
      if (urlToSave) {
        window.pktUI.tryToSaveUrl(urlToSave, titleToSave);
      } else {
        window.pktUI.pocketButtonOnCommand();
      }

      if (iframe.contentDocument &&
          iframe.contentDocument.readyState == "complete") {
        window.pktUI.pocketPanelDidShow();
      } else {
        
        
        
        iframe.addEventListener("load", Pocket.onFrameLoaded, true);
      }
    }, 0);
  },

  onFrameLoaded(event) {
    let document = event.currentTarget.ownerDocument;
    let window = document.defaultView;
    let iframe = window.pktUI.getPanelFrame();

    iframe.removeEventListener("load", Pocket.onFrameLoaded, true);
    window.pktUI.pocketPanelDidShow();
  },

  onPanelViewHiding(event) {
    let window = event.target.ownerDocument.defaultView;
    window.pktUI.pocketPanelDidHide(event);
  },

  _urlToSave: null,
  _titleToSave: null,
  savePage(browser, url, title) {
    let document = browser.ownerDocument;
    let pocketWidget = document.getElementById("pocket-button");
    let placement = CustomizableUI.getPlacementOfWidget("pocket-button");
    if (!placement)
      return;

    this._urlToSave = url;
    this._titleToSave = title;
    if (placement.area == CustomizableUI.AREA_PANEL) {
      let win = document.defaultView;
      win.PanelUI.show().then(function() {
        pocketWidget = document.getElementById("pocket-button");
        pocketWidget.doCommand();
      });
    } else {
      pocketWidget.doCommand();
    }
  },
};
