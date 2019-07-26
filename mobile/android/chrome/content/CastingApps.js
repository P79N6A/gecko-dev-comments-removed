


"use strict";

var CastingApps = {
  _castMenuId: -1,

  init: function ca_init() {
    if (!this.isEnabled()) {
      return;
    }

    
    SimpleServiceDiscovery.registerTarget("roku:ecp", function(aService, aApp) {
      Cu.import("resource://gre/modules/RokuApp.jsm");
      return new RokuApp(aService, "FirefoxTest");
    });

    
    SimpleServiceDiscovery.search(120 * 1000);

    this._castMenuId = NativeWindow.contextmenus.add(
      Strings.browser.GetStringFromName("contextmenu.castToScreen"),
      this.filterCast,
      this.openExternal.bind(this)
    );

    Services.obs.addObserver(this, "Casting:Play", false);
    Services.obs.addObserver(this, "Casting:Pause", false);
    Services.obs.addObserver(this, "Casting:Stop", false);

    BrowserApp.deck.addEventListener("TabSelect", this, true);
    BrowserApp.deck.addEventListener("pageshow", this, true);
    BrowserApp.deck.addEventListener("playing", this, true);
    BrowserApp.deck.addEventListener("ended", this, true);
  },

  uninit: function ca_uninit() {
    BrowserApp.deck.removeEventListener("TabSelect", this, true);
    BrowserApp.deck.removeEventListener("pageshow", this, true);
    BrowserApp.deck.removeEventListener("playing", this, true);
    BrowserApp.deck.removeEventListener("ended", this, true);

    Services.obs.removeObserver(this, "Casting:Play");
    Services.obs.removeObserver(this, "Casting:Pause");
    Services.obs.removeObserver(this, "Casting:Stop");

    NativeWindow.contextmenus.remove(this._castMenuId);
  },

  isEnabled: function isEnabled() {
    return Services.prefs.getBoolPref("browser.casting.enabled");
  },

  observe: function (aSubject, aTopic, aData) {
    switch (aTopic) {
      case "Casting:Play":
        if (this.session && this.session.remoteMedia.status == "paused") {
          this.session.remoteMedia.play();
        }
        break;
      case "Casting:Pause":
        if (this.session && this.session.remoteMedia.status == "started") {
          this.session.remoteMedia.pause();
        }
        break;
      case "Casting:Stop":
        if (this.session) {
          this.closeExternal();
        }
        break;
    }
  },

  handleEvent: function(aEvent) {
    switch (aEvent.type) {
      case "TabSelect": {
        let tab = BrowserApp.getTabForBrowser(aEvent.target);
        this._updatePageActionForTab(tab, aEvent);
        break;
      }
      case "pageshow": {
        let tab = BrowserApp.getTabForWindow(aEvent.originalTarget.defaultView);
        this._updatePageActionForTab(tab, aEvent);
        break;
      }
      case "playing":
      case "ended": {
        let video = aEvent.target;
        if (video instanceof HTMLVideoElement) {
          
          this._updatePageActionForVideo(aEvent.type === "playing" ? video : null);
        }
        break;
      }
    }
  },

  _sendEventToVideo: function _sendEventToVideo(aElement, aData) {
    let event = aElement.ownerDocument.createEvent("CustomEvent");
    event.initCustomEvent("media-videoCasting", false, true, JSON.stringify(aData));
    aElement.dispatchEvent(event);
  },

  handleVideoBindingAttached: function handleVideoBindingAttached(aTab, aEvent) {
    
    
    let video = aEvent.target;
    if (!video instanceof HTMLVideoElement) {
      return;
    }

    if (SimpleServiceDiscovery.services.length == 0) {
      return;
    }

    if (!this.getVideo(video, 0, 0)) {
      return;
    }

    
    this._sendEventToVideo(video, { allow: true });
  },

  handleVideoBindingCast: function handleVideoBindingCast(aTab, aEvent) {
    
    let video = aEvent.target;
    if (!video instanceof HTMLVideoElement) {
      return;
    }

    
    
    this.closeExternal();

    
    this.openExternal(video, 0, 0);
  },

  makeURI: function makeURI(aURL, aOriginCharset, aBaseURI) {
    return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
  },

  getVideo: function(aElement, aX, aY) {
    
    let video = this._getVideo(aElement);
    if (video) {
      return video;
    }

    
    
    try {
      
      
      let elements = aElement.ownerDocument.querySelectorAll("video");
      for (let element of elements) {
        
        let rect = element.getBoundingClientRect();
        if (aY >= rect.top && aX >= rect.left && aY <= rect.bottom && aX <= rect.right) {
          video = this._getVideo(element);
          if (video) {
            break;
          }
        }
      }
    } catch(e) {}

    
    return video;
  },

  _getVideo: function(aElement) {
    if (!aElement instanceof HTMLVideoElement) {
      return null;
    }

    
    
    if (aElement.getAttribute("x-webkit-airplay") === "deny") {
      return null;
    }

    
    function allowableExtension(aURI) {
      if (aURI && aURI instanceof Ci.nsIURL) {
        return (aURI.fileExtension == "mp4");
      }
      return false;
    }

    
    let posterURL = aElement.poster;

    
    let sourceURL = aElement.src;

    
    if (!sourceURL) {
      sourceURL = aElement.currentSrc;
    }

    if (sourceURL) {
      
      let sourceURI = this.makeURI(sourceURL, null, this.makeURI(aElement.baseURI));
      if (allowableExtension(sourceURI)) {
        return { element: aElement, source: sourceURI.spec, poster: posterURL };
      }
    }

    
    
    let sourceNodes = aElement.getElementsByTagName("source");
    for (let sourceNode of sourceNodes) {
      let sourceURI = this.makeURI(sourceNode.src, null, this.makeURI(sourceNode.baseURI));

      
      
      if (sourceNode.type == "video/mp4" || allowableExtension(sourceURI)) {
        return { element: aElement, source: sourceURI.spec, poster: posterURL };
      }
    }

    return null;
  },

  filterCast: {
    matches: function(aElement, aX, aY) {
      if (SimpleServiceDiscovery.services.length == 0)
        return false;
      let video = CastingApps.getVideo(aElement, aX, aY);
      if (CastingApps.session) {
        return (video && CastingApps.session.data.source != video.source);
      }
      return (video != null);
    }
  },

  pageAction: {
    click: function() {
      
      let browser = BrowserApp.selectedBrowser;
      if (!browser) {
        return;
      }

      
      let videos = browser.contentDocument.querySelectorAll("video");
      for (let video of videos) {
        let unwrappedVideo = XPCNativeWrapper.unwrap(video);
        if (!video.paused && unwrappedVideo.mozAllowCasting) {
          CastingApps.openExternal(video, 0, 0);
          return;
        }
      }
    }
  },

  _findCastableVideo: function _findCastableVideo(aBrowser) {
      
      
      let castableVideo = null;
      let videos = aBrowser.contentDocument.querySelectorAll("video");
      for (let video of videos) {
        let unwrappedVideo = XPCNativeWrapper.unwrap(video);
        if (unwrappedVideo.mozIsCasting) {
          
          return video;
        }

        if (!video.paused && unwrappedVideo.mozAllowCasting) {
          
          castableVideo = video;
        }
      }

      
      return castableVideo;
  },

  _updatePageActionForTab: function _updatePageActionForTab(aTab, aEvent) {
    
    if (aTab != BrowserApp.selectedTab) {
      return;
    }

    
    this._updatePageAction();
  },

  _updatePageActionForVideo: function _updatePageActionForVideo(aVideo) {
    this._updatePageAction(aVideo);
  },

  _updatePageAction: function _updatePageAction(aVideo) {
    
    
    if (this.pageAction.id) {
      NativeWindow.pageactions.remove(this.pageAction.id);
      delete this.pageAction.id;
    }

    if (!aVideo) {
      aVideo = this._findCastableVideo(BrowserApp.selectedBrowser);
      if (!aVideo) {
        return;
      }
    }

    
    if (BrowserApp.selectedTab != BrowserApp.getTabForWindow(aVideo.ownerDocument.defaultView.top)) {
      return;
    }

    
    
    
    
    let unwrappedVideo = XPCNativeWrapper.unwrap(aVideo);
    if (unwrappedVideo.mozIsCasting) {
      this.pageAction.id = NativeWindow.pageactions.add({
        title: Strings.browser.GetStringFromName("contextmenu.castToScreen"),
        icon: "drawable://casting_active",
        clickCallback: this.pageAction.click,
        important: true
      });
    } else if (unwrappedVideo.mozAllowCasting) {
      this.pageAction.id = NativeWindow.pageactions.add({
        title: Strings.browser.GetStringFromName("contextmenu.castToScreen"),
        icon: "drawable://casting",
        clickCallback: this.pageAction.click,
        important: true
      });
    }
  },

  prompt: function(aCallback) {
    let items = [];
    SimpleServiceDiscovery.services.forEach(function(aService) {
      let item = {
        label: aService.friendlyName,
        selected: false
      };
      items.push(item);
    });

    let prompt = new Prompt({
      title: Strings.browser.GetStringFromName("casting.prompt")
    }).setSingleChoiceItems(items).show(function(data) {
      let selected = data.button;
      let service = selected == -1 ? null : SimpleServiceDiscovery.services[selected];
      if (aCallback)
        aCallback(service);
    });
  },

  openExternal: function(aElement, aX, aY) {
    
    let video = this.getVideo(aElement, aX, aY);
    if (!video) {
      return;
    }

    this.prompt(function(aService) {
      if (!aService)
        return;

      
      let app = SimpleServiceDiscovery.findAppForService(aService, "video-sharing");
      if (!app)
        return;

      video.title = aElement.ownerDocument.defaultView.top.document.title;
      if (video.element) {
        
        if (!video.element.paused) {
          video.element.pause();
        }
      }

      app.stop(function() {
        app.start(function(aStarted) {
          if (!aStarted) {
            dump("CastingApps: Unable to start app");
            return;
          }

          app.remoteMedia(function(aRemoteMedia) {
            if (!aRemoteMedia) {
              dump("CastingApps: Failed to create remotemedia");
              return;
            }

            this.session = {
              service: aService,
              app: app,
              remoteMedia: aRemoteMedia,
              data: {
                title: video.title,
                source: video.source,
                poster: video.poster
              },
              videoRef: Cu.getWeakReference(video.element)
            };
          }.bind(this), this);
        }.bind(this));
      }.bind(this));
    }.bind(this));
  },

  closeExternal: function() {
    if (!this.session) {
      return;
    }

    this.session.remoteMedia.shutdown();
    this.session.app.stop();

    let video = this.session.videoRef.get();
    if (video) {
      this._sendEventToVideo(video, { active: false });
      this._updatePageAction();
    }

    delete this.session;
  },

  
  onRemoteMediaStart: function(aRemoteMedia) {
    if (!this.session) {
      return;
    }

    aRemoteMedia.load(this.session.data);
    sendMessageToJava({ type: "Casting:Started", device: this.session.service.friendlyName });

    let video = this.session.videoRef.get();
    if (video) {
      this._sendEventToVideo(video, { active: true });
      this._updatePageAction(video);
    }
  },

  onRemoteMediaStop: function(aRemoteMedia) {
    sendMessageToJava({ type: "Casting:Stopped" });
  },

  onRemoteMediaStatus: function(aRemoteMedia) {
    if (!this.session) {
      return;
    }

    let status = aRemoteMedia.status;
    if (status == "completed") {
      this.closeExternal();
    }
  }
};

