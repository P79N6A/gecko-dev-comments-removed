



"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "PageActions",
                                  "resource://gre/modules/PageActions.jsm");



var rokuTarget = {
  target: "roku:ecp",
  factory: function(aService) {
    Cu.import("resource://gre/modules/RokuApp.jsm");
    return new RokuApp(aService);
  },
  types: ["video/mp4"],
  extensions: ["mp4"]
};

var fireflyTarget = {
  target: "urn:dial-multiscreen-org:service:dial:1",
  filters: {
    server: null,
    modelName: "Eureka Dongle"
  },
  factory: function(aService) {
    Cu.import("resource://gre/modules/FireflyApp.jsm");
    return new FireflyApp(aService);
  },
  types: ["video/mp4", "video/webm"],
  extensions: ["mp4", "webm"]
};

var mediaPlayerTarget = {
  target: "media:router",
  factory: function(aService) {
    Cu.import("resource://gre/modules/MediaPlayerApp.jsm");
    return new MediaPlayerApp(aService);
  },
  types: ["video/mp4", "video/webm", "application/x-mpegurl"],
  extensions: ["mp4", "webm", "m3u", "m3u8"]
};

var CastingApps = {
  _castMenuId: -1,
  mirrorStartMenuId: -1,
  mirrorStopMenuId: -1,

  init: function ca_init() {
    if (!this.isEnabled()) {
      return;
    }

    
    SimpleServiceDiscovery.registerTarget(rokuTarget);
    SimpleServiceDiscovery.registerTarget(fireflyTarget);
    SimpleServiceDiscovery.registerTarget(mediaPlayerTarget);

    
    SimpleServiceDiscovery.search(120 * 1000);

    this._castMenuId = NativeWindow.contextmenus.add(
      Strings.browser.GetStringFromName("contextmenu.sendToDevice"),
      this.filterCast,
      this.handleContextMenu.bind(this)
    );

    Services.obs.addObserver(this, "Casting:Play", false);
    Services.obs.addObserver(this, "Casting:Pause", false);
    Services.obs.addObserver(this, "Casting:Stop", false);
    Services.obs.addObserver(this, "Casting:Mirror", false);
    Services.obs.addObserver(this, "ssdp-service-found", false);
    Services.obs.addObserver(this, "ssdp-service-lost", false);

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
    Services.obs.removeObserver(this, "Casting:Mirror");
    Services.obs.removeObserver(this, "ssdp-service-found");
    Services.obs.removeObserver(this, "ssdp-service-lost");

    NativeWindow.contextmenus.remove(this._castMenuId);
  },

  serviceAdded: function(aService) {
    if (aService.mirror && this.mirrorStartMenuId == -1) {
      this.mirrorStartMenuId = NativeWindow.menu.add({
        name: Strings.browser.GetStringFromName("casting.mirrorTab"),
        callback: function() {
          function callbackFunc(aService) {
            let app = SimpleServiceDiscovery.findAppForService(aService);
            if (app)
              app.mirror(function() {
              });
          }

          function filterFunc(aService) {
            return aService.mirror == true;
          }
          this.prompt(callbackFunc, filterFunc);
        }.bind(this),
        parent: NativeWindow.menu.toolsMenuID
      });

      this.mirrorStopMenuId = NativeWindow.menu.add({
        name: Strings.browser.GetStringFromName("casting.mirrorTabStop"),
        callback: function() {
          if (this.tabMirror) {
            this.tabMirror.stop();
            this.tabMirror = null;
          }
          NativeWindow.menu.update(this.mirrorStartMenuId, { visible: true });
          NativeWindow.menu.update(this.mirrorStopMenuId, { visible: false });
        }.bind(this),
        parent: NativeWindow.menu.toolsMenuID
      });
    }
    NativeWindow.menu.update(this.mirrorStopMenuId, { visible: false });
  },

  serviceLost: function(aService) {
    if (aService.mirror && this.mirrorStartMenuId != -1) {
      let haveMirror = false;
      SimpleServiceDiscovery.services.forEach(function(service) {
        if (service.mirror) {
          haveMirror = true;
        }
      });
      if (!haveMirror) {
        NativeWindow.menu.remove(this.mirrorStartMenuId);
        this.mirrorStartMenuId = -1;
      }
    }
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
      case "Casting:Mirror":
        {
          Cu.import("resource://gre/modules/TabMirror.jsm");
          this.tabMirror = new TabMirror(aData, window);
          NativeWindow.menu.update(this.mirrorStartMenuId, { visible: false });
          NativeWindow.menu.update(this.mirrorStopMenuId, { visible: true });
        }
        break;
      case "ssdp-service-found":
        {
          this.serviceAdded(SimpleServiceDiscovery.findServiceForID(aData));
          break;
        }
      case "ssdp-service-lost":
        {
          this.serviceLost(SimpleServiceDiscovery.findServiceForID(aData));
          break;
        }
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
    if (!(video instanceof HTMLVideoElement)) {
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
    if (!(video instanceof HTMLVideoElement)) {
      return;
    }

    
    
    this.closeExternal();

    
    UITelemetry.addEvent("cast.1", "button", null);
    this.openExternal(video, 0, 0);
  },

  makeURI: function makeURI(aURL, aOriginCharset, aBaseURI) {
    return Services.io.newURI(aURL, aOriginCharset, aBaseURI);
  },

  getVideo: function(aElement, aX, aY) {
    let extensions = SimpleServiceDiscovery.getSupportedExtensions();
    let types = SimpleServiceDiscovery.getSupportedMimeTypes();

    
    let video = this._getVideo(aElement, types, extensions);
    if (video) {
      return video;
    }

    
    
    try {
      
      
      let elements = aElement.ownerDocument.querySelectorAll("video");
      for (let element of elements) {
        
        let rect = element.getBoundingClientRect();
        if (aY >= rect.top && aX >= rect.left && aY <= rect.bottom && aX <= rect.right) {
          video = this._getVideo(element, types, extensions);
          if (video) {
            break;
          }
        }
      }
    } catch(e) {}

    
    return video;
  },

  _getVideo: function(aElement, aTypes, aExtensions) {
    if (!(aElement instanceof HTMLVideoElement)) {
      return null;
    }


    
    let posterURL = aElement.poster;

    
    let sourceURL = aElement.src;

    
    if (!sourceURL) {
      sourceURL = aElement.currentSrc;
    }

    if (sourceURL) {
      
      let sourceURI = this.makeURI(sourceURL, null, this.makeURI(aElement.baseURI));
      if (this.allowableExtension(sourceURI, aExtensions)) {
        return { element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI};
      }
    }

    
    
    let sourceNodes = aElement.getElementsByTagName("source");
    for (let sourceNode of sourceNodes) {
      let sourceURI = this.makeURI(sourceNode.src, null, this.makeURI(sourceNode.baseURI));

      
      
      if (this.allowableMimeType(sourceNode.type, aTypes) || this.allowableExtension(sourceURI, aExtensions)) {
        return { element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI, type: sourceNode.type };
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
        if (!video.paused && video.mozAllowCasting) {
          UITelemetry.addEvent("cast.1", "pageaction", null);
          CastingApps.openExternal(video, 0, 0);
          return;
        }
      }
    }
  },

  _findCastableVideo: function _findCastableVideo(aBrowser) {
      if (!aBrowser) {
        return null;
      }

      
      
      let castableVideo = null;
      let videos = aBrowser.contentDocument.querySelectorAll("video");
      for (let video of videos) {
        if (video.mozIsCasting) {
          
          return video;
        }

        if (!video.paused && video.mozAllowCasting) {
          
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
      PageActions.remove(this.pageAction.id);
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

    
    
    
    
    if (aVideo.mozIsCasting) {
      this.pageAction.id = PageActions.add({
        title: Strings.browser.GetStringFromName("contextmenu.sendToDevice"),
        icon: "drawable://casting_active",
        clickCallback: this.pageAction.click,
        important: true
      });
    } else if (aVideo.mozAllowCasting) {
      this.pageAction.id = PageActions.add({
        title: Strings.browser.GetStringFromName("contextmenu.sendToDevice"),
        icon: "drawable://casting",
        clickCallback: this.pageAction.click,
        important: true
      });
    }
  },

  prompt: function(aCallback, aFilterFunc) {
    let items = [];
    let filteredServices = [];
    SimpleServiceDiscovery.services.forEach(function(aService) {
      let item = {
        label: aService.friendlyName,
        selected: false
      };
      if (!aFilterFunc || aFilterFunc(aService)) {
        filteredServices.push(aService);
        items.push(item);
      }
    });

    if (items.length == 0) {
      return;
    }

    let prompt = new Prompt({
      title: Strings.browser.GetStringFromName("casting.sendToDevice")
    }).setSingleChoiceItems(items).show(function(data) {
      let selected = data.button;
      let service = selected == -1 ? null : filteredServices[selected];
      if (aCallback)
        aCallback(service);
    });
  },

  handleContextMenu: function(aElement, aX, aY) {
    UITelemetry.addEvent("action.1", "contextmenu", null, "web_cast");
    UITelemetry.addEvent("cast.1", "contextmenu", null);
    this.openExternal(aElement, aX, aY);
  },

  openExternal: function(aElement, aX, aY) {
    
    let video = this.getVideo(aElement, aX, aY);
    if (!video) {
      return;
    }

    function filterFunc(service) {
      return this.allowableExtension(video.sourceURI, service.extensions) || this.allowableMimeType(video.type, service.types);
    }

    this.prompt(function(aService) {
      if (!aService)
        return;

      
      let app = SimpleServiceDiscovery.findAppForService(aService);
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
    }.bind(this), filterFunc.bind(this));
  },

  closeExternal: function() {
    if (!this.session) {
      return;
    }

    this.session.remoteMedia.shutdown();
    this._shutdown();
  },

  _shutdown: function() {
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
    Messaging.sendRequest({ type: "Casting:Started", device: this.session.service.friendlyName });

    let video = this.session.videoRef.get();
    if (video) {
      this._sendEventToVideo(video, { active: true });
      this._updatePageAction(video);
    }
  },

  onRemoteMediaStop: function(aRemoteMedia) {
    Messaging.sendRequest({ type: "Casting:Stopped" });
    this._shutdown();
  },

  onRemoteMediaStatus: function(aRemoteMedia) {
    if (!this.session) {
      return;
    }

    let status = aRemoteMedia.status;
    if (status == "completed") {
      this.closeExternal();
    }
  },

  allowableExtension: function(aURI, aExtensions) {
    if (aURI && aURI instanceof Ci.nsIURL) {
      for (let x in aExtensions) {
        if (aURI.fileExtension == aExtensions[x]) return true;
      }
    }
    return false;
  },

  allowableMimeType: function(aType, aTypes) {
    for (let x in aTypes) {
      if (aType == aTypes[x]) return true;
    }
    return false;
  }
};
