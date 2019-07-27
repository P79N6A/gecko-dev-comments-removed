





let ReadingListUI = {
  


  init() {
    Preferences.observe("browser.readinglist.enabled", this.updateUI, this);
    this.updateUI();
  },

  


  uninit() {
    Preferences.ignore("browser.readinglist.enabled", this.updateUI, this);
  },

  



  get enabled() {
    return Preferences.get("browser.readinglist.enabled", false);
  },

  



  get isSidebarOpen() {
    return SidebarUI.isOpen && SidebarUI.currentID == "readingListSidebar";
  },

  



  updateUI() {
    let enabled = this.enabled;
    if (!enabled) {
      this.hideSidebar();
    }

    document.getElementById("readingListSidebar").setAttribute("hidden", !enabled);
  },

  



  showSidebar() {
    return SidebarUI.show("readingListSidebar");
  },

  


  hideSidebar() {
    if (this.isSidebarOpen) {
      SidebarUI.hide();
    }
  },
};
