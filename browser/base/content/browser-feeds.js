# -*- indent-tabs-mode: nil; js-indent-level: 2 -*-
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http:





var FeedHandler = {
  












  buildFeedList: function(container, isSubview) {
    var feeds = gBrowser.selectedBrowser.feeds;
    if (!isSubview && feeds == null) {
      
      
      
      
      
      
      container.parentNode.removeAttribute("open");
      return false;
    }

    for (let i = container.childNodes.length - 1; i >= 0; --i) {
      let node = container.childNodes[i];
      if (isSubview && node.localName == "label")
        continue;
      container.removeChild(node);
    }

    if (!feeds || feeds.length <= 1)
      return false;

    
    var itemNodeType = isSubview ? "toolbarbutton" : "menuitem";
    for (let feedInfo of feeds) {
      var item = document.createElement(itemNodeType);
      var baseTitle = feedInfo.title || feedInfo.href;
      var labelStr = gNavigatorBundle.getFormattedString("feedShowFeedNew", [baseTitle]);
      item.setAttribute("label", labelStr);
      item.setAttribute("feed", feedInfo.href);
      item.setAttribute("tooltiptext", feedInfo.href);
      item.setAttribute("crop", "center");
      let className = "feed-" + itemNodeType;
      if (isSubview) {
        className += " subviewbutton";
      }
      item.setAttribute("class", className);
      container.appendChild(item);
    }
    return true;
  },

  












  subscribeToFeed: function(href, event) {
    
    
    if (!href)
      href = event.target.getAttribute("feed");
    urlSecurityCheck(href, gBrowser.contentPrincipal,
                     Ci.nsIScriptSecurityManager.DISALLOW_INHERIT_PRINCIPAL);
    var feedURI = makeURI(href, document.characterSet);
    
    
    if (/^https?$/.test(feedURI.scheme))
      href = "feed:" + href;
    this.loadFeed(href, event);
  },

  loadFeed: function(href, event) {
    var feeds = gBrowser.selectedBrowser.feeds;
    try {
      openUILink(href, event, { ignoreAlt: true });
    }
    finally {
      
      
      gBrowser.selectedBrowser.feeds = feeds;
    }
  },

  get _feedMenuitem() {
    delete this._feedMenuitem;
    return this._feedMenuitem = document.getElementById("singleFeedMenuitemState");
  },

  get _feedMenupopup() {
    delete this._feedMenupopup;
    return this._feedMenupopup = document.getElementById("multipleFeedsMenuState");
  },

  



  updateFeeds: function() {
    if (this._updateFeedTimeout)
      clearTimeout(this._updateFeedTimeout);

    var feeds = gBrowser.selectedBrowser.feeds;
    var haveFeeds = feeds && feeds.length > 0;

    var feedButton = document.getElementById("feed-button");
    if (feedButton) {
      if (haveFeeds) {
        feedButton.removeAttribute("disabled");
      } else {
        feedButton.setAttribute("disabled", "true");
      }
    }

    if (!haveFeeds) {
      this._feedMenuitem.setAttribute("disabled", "true");
      this._feedMenuitem.removeAttribute("hidden");
      this._feedMenupopup.setAttribute("hidden", "true");
      return;
    }

    if (feeds.length > 1) {
      this._feedMenuitem.setAttribute("hidden", "true");
      this._feedMenupopup.removeAttribute("hidden");
    } else {
      this._feedMenuitem.setAttribute("feed", feeds[0].href);
      this._feedMenuitem.removeAttribute("disabled");
      this._feedMenuitem.removeAttribute("hidden");
      this._feedMenupopup.setAttribute("hidden", "true");
    }
  },

  addFeed: function(link, browserForLink) {
    if (!browserForLink.feeds)
      browserForLink.feeds = [];

    browserForLink.feeds.push({ href: link.href, title: link.title });

    
    
    if (browserForLink == gBrowser.selectedBrowser) {
      
      
      if (this._updateFeedTimeout)
        clearTimeout(this._updateFeedTimeout);
      this._updateFeedTimeout = setTimeout(this.updateFeeds.bind(this), 100);
    }
  }
};
