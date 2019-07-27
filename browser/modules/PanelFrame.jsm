


"use strict";



this.EXPORTED_SYMBOLS = ["PanelFrame"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "CustomizableUI", "resource:///modules/CustomizableUI.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SharedFrame", "resource:///modules/SharedFrame.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DynamicResizeWatcher", "resource:///modules/Social.jsm");


const PANEL_MIN_HEIGHT = 100;
const PANEL_MIN_WIDTH = 330;

let PanelFrameInternal = {
  


  get _dynamicResizer() {
    delete this._dynamicResizer;
    this._dynamicResizer = new DynamicResizeWatcher();
    return this._dynamicResizer;
  },

  












  _attachNotificatonPanel: function(aWindow, aParent, aButton, aType, aOrigin, aSrc, aSize) {
    aParent.hidden = false;
    let notificationFrameId = aOrigin ? aType + "-status-" + aOrigin : aType;
    let frame = aWindow.document.getElementById(notificationFrameId);

    
    
    if (frame && frame.parentNode != aParent) {
      SharedFrame.forgetGroup(frame.id);
      frame.parentNode.removeChild(frame);
      frame = null;
    }

    if (!frame) {
      let {width, height} = aSize ? aSize : {width: PANEL_MIN_WIDTH, height: PANEL_MIN_HEIGHT};

      frame = SharedFrame.createFrame(
        notificationFrameId, 
        aParent, 
        {
          "type": "content",
          "mozbrowser": "true",
          
          "class": "social-panel-frame",
          "id": notificationFrameId,
          "tooltip": "aHTMLTooltip",
          "context": "contentAreaContextMenu",
          "flex": "1",

          
          
          "style": "width: " + width + "px; height: " + height + "px;",
          "dynamicresizer": !aSize,

          "origin": aOrigin,
          "src": aSrc
        }
      );
    } else {
      frame.setAttribute("origin", aOrigin);
      SharedFrame.updateURL(notificationFrameId, aSrc);
    }
    aButton.setAttribute("notificationFrameId", notificationFrameId);
  }
};




let PanelFrame = {
  















  showPopup: function(aWindow, aToolbarButton, aType, aOrigin, aSrc, aSize, aCallback) {
    
    let widgetGroup = CustomizableUI.getWidget(aToolbarButton.getAttribute("id"));
    let widget = widgetGroup.forWindow(aWindow);
    
    
    let anchorBtn = widget.anchor;

    let panel = aWindow.document.getElementById(aType + "-notification-panel");
    PanelFrameInternal._attachNotificatonPanel(aWindow, panel, aToolbarButton, aType, aOrigin, aSrc, aSize);

    let notificationFrameId = aToolbarButton.getAttribute("notificationFrameId");
    let notificationFrame = aWindow.document.getElementById(notificationFrameId);


    
    
    let frameIter = panel.firstElementChild;
    while (frameIter) {
      frameIter.collapsed = (frameIter != notificationFrame);
      frameIter = frameIter.nextElementSibling;
    }

    function dispatchPanelEvent(name) {
      let evt = notificationFrame.contentDocument.createEvent("CustomEvent");
      evt.initCustomEvent(name, true, true, {});
      notificationFrame.contentDocument.documentElement.dispatchEvent(evt);
    }

    
    let dynamicResizer;
    if (notificationFrame.getAttribute("dynamicresizer") == "true") {
      dynamicResizer = PanelFrameInternal._dynamicResizer;
    }
    panel.addEventListener("popuphidden", function onpopuphiding() {
      panel.removeEventListener("popuphidden", onpopuphiding);
      anchorBtn.removeAttribute("open");
      if (dynamicResizer)
        dynamicResizer.stop();
      notificationFrame.docShell.isActive = false;
      dispatchPanelEvent(aType + "FrameHide");
    });

    panel.addEventListener("popupshown", function onpopupshown() {
      panel.removeEventListener("popupshown", onpopupshown);
      SharedFrame.setOwner(notificationFrameId, notificationFrame);
      
      
      
      
      
      let initFrameShow = () => {
        notificationFrame.docShell.isActive = true;
        notificationFrame.docShell.isAppTab = true;
        if (dynamicResizer)
          dynamicResizer.start(panel, notificationFrame);
        dispatchPanelEvent(aType + "FrameShow");
      };
      anchorBtn.setAttribute("open", "true");
      if (notificationFrame.contentDocument &&
          notificationFrame.contentDocument.readyState == "complete") {
        initFrameShow();
      } else {
        
        notificationFrame.addEventListener("load", function panelBrowserOnload(e) {
          notificationFrame.removeEventListener("load", panelBrowserOnload, true);
          initFrameShow();
        }, true);
      }
    });

    
    let anchor = aWindow.document.getAnonymousElementByAttribute(anchorBtn, "class", "toolbarbutton-badge-container") ||
                 aWindow.document.getAnonymousElementByAttribute(anchorBtn, "class", "toolbarbutton-icon");
    
    
    Services.tm.mainThread.dispatch(function() {
      panel.openPopup(anchor, "bottomcenter topright", 0, 0, false, false);
    }, Ci.nsIThread.DISPATCH_NORMAL);

    if (aCallback)
      aCallback(notificationFrame);
  }
};
