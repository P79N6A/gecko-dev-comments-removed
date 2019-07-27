





let ReadingListUI = {
  


  init() {
    Preferences.observe("browser.readinglist.enabled", () => this.updateUI());
    this.updateUI();
  },

  



  get enabled() {
    return Preferences.get("browser.readinglist.enabled", false);
  },

  



  get isSidebarOpen() {
    return SidebarUI.isOpen && SidebarUI.currentID == "readingListSidebar";
  },

  



  updateUI() {
    let enabled = this.enabled;
    document.getElementById("readingListSidebar").setAttribute("hidden", !enabled);

    if (!enabled) {
      this.hideSidebar();
    }
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
