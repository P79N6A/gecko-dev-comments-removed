


"use strict";

var FeedHandler = {
  PREF_CONTENTHANDLERS_BRANCH: "browser.contentHandlers.types.",
  TYPE_MAYBE_FEED: "application/vnd.mozilla.maybe.feed",

  _contentTypes: null,

  getContentHandlers: function fh_getContentHandlers(contentType) {
    if (!this._contentTypes)
      this.loadContentHandlers();

    if (!(contentType in this._contentTypes))
      return [];

    return this._contentTypes[contentType];
  },
  
  loadContentHandlers: function fh_loadContentHandlers() {
    this._contentTypes = {};

    let kids = Services.prefs.getBranch(this.PREF_CONTENTHANDLERS_BRANCH).getChildList("");

    
    let nums = [];
    for (let i = 0; i < kids.length; i++) {
      let match = /^(\d+)\.uri$/.exec(kids[i]);
      if (!match)
        continue;
      else
        nums.push(match[1]);
    }

    
    nums.sort(function(a, b) { return a - b; });

    
    for (let i = 0; i < nums.length; i++) {
      let branch = Services.prefs.getBranch(this.PREF_CONTENTHANDLERS_BRANCH + nums[i] + ".");
      let vals = branch.getChildList("");
      if (vals.length == 0)
        return;

      try {
        let type = branch.getCharPref("type");
        let uri = branch.getComplexValue("uri", Ci.nsIPrefLocalizedString).data;
        let title = branch.getComplexValue("title", Ci.nsIPrefLocalizedString).data;

        if (!(type in this._contentTypes))
          this._contentTypes[type] = [];
        this._contentTypes[type].push({ contentType: type, uri: uri, name: title });
      }
      catch(ex) {}
    }
  },

  observe: function fh_observe(aSubject, aTopic, aData) {
    if (aTopic === "Feeds:Subscribe") {
      let args = JSON.parse(aData);
      let tab = BrowserApp.getTabForId(args.tabId);
      if (!tab)
        return;

      let browser = tab.browser;
      let feeds = browser.feeds;
      if (feeds == null)
        return;

      
      let feedIndex = -1;
      if (feeds.length > 1) {
        
        let feedResult = {
          type: "Prompt:Show",
          multiple: false,
          selected: [],
          listitems: []
        };

        
        for (let i = 0; i < feeds.length; i++) {
          let item = {
            label: feeds[i].title || feeds[i].href,
            isGroup: false,
            inGroup: false,
            disabled: false,
            id: i
          };
          feedResult.listitems.push(item);
        }
        feedIndex = JSON.parse(sendMessageToJava(feedResult)).button;
      } else {
        
        feedIndex = 0;
      }

      if (feedIndex == -1)
        return;
      let feedURL = feeds[feedIndex].href;

      
      let handlers = this.getContentHandlers(this.TYPE_MAYBE_FEED);
      if (handlers.length == 0)
        return;

      
      let handlerResult = {
        type: "Prompt:Show",
        multiple: false,
        selected: [],
        listitems: []
      };

      
      for (let i = 0; i < handlers.length; ++i) {
        let item = {
          label: handlers[i].name,
          isGroup: false,
          inGroup: false,
          disabled: false,
          id: i
        };
        handlerResult.listitems.push(item);
      }
      let handlerIndex = JSON.parse(sendMessageToJava(handlerResult)).button;
      if (handlerIndex == -1)
        return;

      
      let readerURL = handlers[handlerIndex].uri;
      readerURL = readerURL.replace(/%s/gi, encodeURIComponent(feedURL));

      
      BrowserApp.addTab(readerURL, { parentId: BrowserApp.selectedTab.id });
    }
  }
};
