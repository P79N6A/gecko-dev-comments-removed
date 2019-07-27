



Components.utils.import("resource://gre/modules/PrivateBrowsingUtils.jsm");

let ContentAreaDownloadsView = {
  init() {
    let view = new DownloadsPlacesView(document.getElementById("downloadsRichListBox"));
    
    if (!PrivateBrowsingUtils.isWindowPrivate(window)) {
      view.place = "place:transition=7&sort=4";
    }
  },
};
