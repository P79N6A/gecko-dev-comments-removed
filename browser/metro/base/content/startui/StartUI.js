



"use strict";

Cu.import("resource://gre/modules/Services.jsm");

var StartUI = {
  get startUI() { return document.getElementById("start-container"); },

  get maxResultsPerSection() {
    return Services.prefs.getIntPref("browser.display.startUI.maxresults");
  },

  init: function init() {
    this.startUI.addEventListener("contextmenu", this, false);
    this.startUI.addEventListener("click", this, false);
    this.startUI.addEventListener("MozMousePixelScroll", this, false);

    TopSitesStartView.init();
    BookmarksStartView.init();
    HistoryStartView.init();
    RemoteTabsStartView.init();

    TopSitesStartView.show();
    BookmarksStartView.show();
    HistoryStartView.show();
    RemoteTabsStartView.show();
  },

  uninit: function() {
    if (TopSitesStartView)
      TopSitesStartView.uninit();
    if (BookmarksStartView)
      BookmarksStartView.uninit();
    if (HistoryStartView)
      HistoryStartView.uninit();
    if (RemoteTabsStartView)
      RemoteTabsStartView.uninit();
  },

  get chromeWin() {
    
    return Services.wm.getMostRecentWindow('navigator:browser');
  },

  goToURI: function (aURI) {
    this.chromeWin.BrowserUI.goToURI(aURI);
  },

  onClick: function onClick(aEvent) {
    
    
    if (this.chromeWin.BrowserUI.blurNavBar()) {
      
      
      this.chromeWin.ContentAreaObserver.navBarWillBlur();
    }
    if (aEvent.button == 0) {
      this.chromeWin.ContextUI.dismissTabs();
    }
  },

  onNarrowTitleClick: function onNarrowTitleClick(sectionId) {
    let section = document.getElementById(sectionId);

    if (section.hasAttribute("expanded"))
      return;

    for (let expandedSection of this.startUI.querySelectorAll(".meta-section[expanded]"))
      expandedSection.removeAttribute("expanded")

    section.setAttribute("expanded", "true");
  },

  getScrollBoxObject: function () {
    let startBox = document.getElementById("start-scrollbox");
    if (!startBox._cachedSBO) {
      startBox._cachedSBO = startBox.boxObject.QueryInterface(Ci.nsIScrollBoxObject);
    }
    return startBox._cachedSBO;
  },

  handleEvent: function handleEvent(aEvent) {
    switch (aEvent.type) {
      case "contextmenu":
        let event = document.createEvent("Events");
        event.initEvent("MozEdgeUICompleted", true, false);
        window.dispatchEvent(event);
        break;

      case "click":
        this.onClick(aEvent);
        break;

      case "MozMousePixelScroll":
        let scroller = this.getScrollBoxObject();
        if (this.startUI.getAttribute("viewstate") == "snapped") {
          scroller.scrollBy(0, aEvent.detail);
        } else {
          scroller.scrollBy(aEvent.detail, 0);
        }

        aEvent.preventDefault();
        aEvent.stopPropagation();
        break;
    }
  }
};
