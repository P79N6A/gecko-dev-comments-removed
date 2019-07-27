# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:


















let SidebarUI = {
  browser: null,

  _box: null,
  _title: null,
  _splitter: null,

  init() {
    this._box = document.getElementById("sidebar-box");
    this.browser = document.getElementById("sidebar");
    this._title = document.getElementById("sidebar-title");
    this._splitter = document.getElementById("sidebar-splitter");

    if (window.opener && !window.opener.closed &&
        window.opener.document.documentURIObject.schemeIs("chrome") &&
        PrivateBrowsingUtils.isWindowPrivate(window) == PrivateBrowsingUtils.isWindowPrivate(window.opener)) {
      this.adoptFromWindow(window.opener);
    } else {
      let commandID = this._box.getAttribute("sidebarcommand");
      if (commandID) {
        let command = document.getElementById(commandID);
        if (command) {
          this._delayedLoad = true;
          this._box.hidden = false;
          this._splitter.hidden = false;
          command.setAttribute("checked", "true");
        } else {
          
          
          
          this._box.removeAttribute("sidebarcommand");
        }
      }
    }
  },

  uninit() {
    let enumerator = Services.wm.getEnumerator(null);
    enumerator.getNext();
    if (!enumerator.hasMoreElements()) {
      document.persist("sidebar-box", "sidebarcommand");
      document.persist("sidebar-box", "width");
      document.persist("sidebar-box", "src");
      document.persist("sidebar-title", "value");
    }
  },

  



  adoptFromWindow(sourceWindow) {
    
    
    
    let sourceUI = sourceWindow.SidebarUI;
    if (!sourceUI || sourceUI._box.hidden) {
      return;
    }

    let commandID = sourceUI._box.getAttribute("sidebarcommand");
    let commandElem = document.getElementById(commandID);

    
    if (!commandElem) {
      return;
    }

    this._title.setAttribute("value",
                             sourceUI._title.getAttribute("value"));
    this._box.setAttribute("width", sourceUI._box.boxObject.width);

    this._box.setAttribute("sidebarcommand", commandID);
    
    
    
    this._box.setAttribute("src", sourceUI.browser.getAttribute("src"));
    this._delayedLoad = true;

    this._box.hidden = false;
    this._splitter.hidden = false;
    commandElem.setAttribute("checked", "true");
  },

  


  startDelayedLoad() {
    if (!this._delayedLoad) {
      return;
    }

    this.browser.setAttribute("src", this._box.getAttribute("src"));
  },

  





  _fireFocusedEvent() {
    let event = new CustomEvent("SidebarFocused", {bubbles: true});
    this.browser.contentWindow.dispatchEvent(event);

    
    fireSidebarFocusedEvent();
  },

  


  get isOpen() {
    return !this._box.hidden;
  },

  



  get currentID() {
    return this._box.getAttribute("sidebarcommand");
  },

  get title() {
    return this._title.value;
  },

  set title(value) {
    this._title.value = value;
  },

  







  toggle(commandID = this.currentID) {
    if (this.isOpen && commandID == this.currentID) {
      this.hide();
      return Promise.resolve();
    } else {
      return this.show(commandID);
    }
  },

  





  show(commandID) {
    return new Promise((resolve, reject) => {
      let sidebarBroadcaster = document.getElementById(commandID);
      if (!sidebarBroadcaster || sidebarBroadcaster.localName != "broadcaster") {
        reject(new Error("Invalid sidebar broadcaster specified"));
        return;
      }

      let broadcasters = document.getElementsByAttribute("group", "sidebar");
      for (let broadcaster of broadcasters) {
        
        
        if (broadcaster.localName != "broadcaster") {
          continue;
        }

        if (broadcaster != sidebarBroadcaster) {
          broadcaster.removeAttribute("checked");
        } else {
          sidebarBroadcaster.setAttribute("checked", "true");
        }
      }

      this._box.hidden = false;
      this._splitter.hidden = false;

      this._box.setAttribute("sidebarcommand", sidebarBroadcaster.id);

      let title = sidebarBroadcaster.getAttribute("sidebartitle");
      if (!title) {
        title = sidebarBroadcaster.getAttribute("label");
      }
      this._title.value = title;

      let url = sidebarBroadcaster.getAttribute("sidebarurl");
      this.browser.setAttribute("src", url); 

      
      
      
      
      
      this._box.setAttribute("src", url);

      if (this.browser.contentDocument.location.href != url) {
        let onLoad = event => {
          this.browser.removeEventListener("load", onLoad, true);

          
          
          
          setTimeout(() => this._fireFocusedEvent(), 0);

          
          sidebarOnLoad(event);

          resolve();
        };

        this.browser.addEventListener("load", onLoad, true);
      } else {
        
        this._fireFocusedEvent();
        resolve();
      }
    });
  },

  


  hide() {
    if (!this.isOpen) {
      return;
    }

    let commandID = this._box.getAttribute("sidebarcommand");
    let sidebarBroadcaster = document.getElementById(commandID);

    if (sidebarBroadcaster.getAttribute("checked") != "true") {
      return;
    }

    
    
    
    
    
    this.browser.setAttribute("src", "about:blank");
    this.browser.docShell.createAboutBlankContentViewer(null);

    sidebarBroadcaster.removeAttribute("checked");
    this._box.setAttribute("sidebarcommand", "");
    this._title.value = "";
    this._box.hidden = true;
    this._splitter.hidden = true;
    gBrowser.selectedBrowser.focus();
  },
};







function fireSidebarFocusedEvent() {}







function sidebarOnLoad(event) {}








function toggleSidebar(commandID, forceOpen = false) {
  Deprecated.warning("toggleSidebar() is deprecated, please use SidebarUI.toggle() or SidebarUI.show() instead",
                     "https://developer.mozilla.org/en-US/Add-ons/Code_snippets/Sidebar");

  if (forceOpen) {
    SidebarUI.show(commandID);
  } else {
    SidebarUI.toggle(commandID);
  }
}
