



"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "PageActions",
                                  "resource://gre/modules/PageActions.jsm");



var rokuDevice = {
  id: "roku:ecp",
  target: "roku:ecp",
  factory: function(aService) {
    Cu.import("resource://gre/modules/RokuApp.jsm");
    return new RokuApp(aService);
  },
  mirror: Services.prefs.getBoolPref("browser.mirroring.enabled.roku"),
  types: ["video/mp4"],
  extensions: ["mp4"]
};

var matchstickDevice = {
  id: "matchstick:dial",
  target: "urn:dial-multiscreen-org:service:dial:1",
  filters: {
    server: null,
    modelName: "Eureka Dongle"
  },
  factory: function(aService) {
    Cu.import("resource://gre/modules/MatchstickApp.jsm");
    return new MatchstickApp(aService);
  },
  types: ["video/mp4", "video/webm"],
  extensions: ["mp4", "webm"]
};

var mediaPlayerDevice = {
  id: "media:router",
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
    if (!this.isCastingEnabled()) {
      return;
    }

    
    SimpleServiceDiscovery.registerDevice(rokuDevice);
    SimpleServiceDiscovery.registerDevice(matchstickDevice);
    SimpleServiceDiscovery.registerDevice(mediaPlayerDevice);

    
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

  _mirrorStarted: function(stopMirrorCallback) {
    this.stopMirrorCallback = stopMirrorCallback;
    NativeWindow.menu.update(this.mirrorStartMenuId, { visible: false });
    NativeWindow.menu.update(this.mirrorStopMenuId, { visible: true });
  },

  serviceAdded: function(aService) {
    if (this.isMirroringEnabled() && aService.mirror && this.mirrorStartMenuId == -1) {
      this.mirrorStartMenuId = NativeWindow.menu.add({
        name: Strings.browser.GetStringFromName("casting.mirrorTab"),
        callback: function() {
          let callbackFunc = function(aService) {
            let app = SimpleServiceDiscovery.findAppForService(aService);
            if (app) {
              app.mirror(function() {}, window, BrowserApp.selectedTab.getViewport(), this._mirrorStarted.bind(this));
            }
          }.bind(this);

          this.prompt(callbackFunc, aService => aService.mirror);
        }.bind(this),
        parent: NativeWindow.menu.toolsMenuID
      });

      this.mirrorStopMenuId = NativeWindow.menu.add({
        name: Strings.browser.GetStringFromName("casting.mirrorTabStop"),
        callback: function() {
          if (this.tabMirror) {
            this.tabMirror.stop();
            this.tabMirror = null;
          } else if (this.stopMirrorCallback) {
            this.stopMirrorCallback();
            this.stopMirrorCallback = null;
          }
          NativeWindow.menu.update(this.mirrorStartMenuId, { visible: true });
          NativeWindow.menu.update(this.mirrorStopMenuId, { visible: false });
        }.bind(this),
      });
    }
    if (this.mirrorStartMenuId != -1) {
      NativeWindow.menu.update(this.mirrorStopMenuId, { visible: false });
    }
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

  isCastingEnabled: function isCastingEnabled() {
    return Services.prefs.getBoolPref("browser.casting.enabled");
  },

  isMirroringEnabled: function isMirroringEnabled() {
    return Services.prefs.getBoolPref("browser.mirroring.enabled");
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

    this.getVideo(video, 0, 0, (aBundle) => {
      
      if (aBundle) {
        this._sendEventToVideo(aBundle.element, { allow: true });
      }
    });
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

  allowableExtension: function(aURI, aExtensions) {
    return (aURI instanceof Ci.nsIURL) && aExtensions.indexOf(aURI.fileExtension) != -1;
  },

  allowableMimeType: function(aType, aTypes) {
    return aTypes.indexOf(aType) != -1;
  },

  
  
  
  getVideo: function(aElement, aX, aY, aCallback) {
    let extensions = SimpleServiceDiscovery.getSupportedExtensions();
    let types = SimpleServiceDiscovery.getSupportedMimeTypes();

    
    if (aElement instanceof HTMLVideoElement) {
      
      
      this._getVideo(aElement, types, extensions, aCallback);
      return;
    }

    
    

    
    
    try {
      let elements = aElement.ownerDocument.querySelectorAll("video");
      for (let element of elements) {
        
        let rect = element.getBoundingClientRect();
        if (aY >= rect.top && aX >= rect.left && aY <= rect.bottom && aX <= rect.right) {
          
          this._getVideo(element, types, extensions, aCallback);
          return;
        }
      }
    } catch(e) {}
  },

  _getContentTypeForURI: function(aURI, aCallback) {
    let channel = Services.io.newChannelFromURI(aURI);
    let listener = {
      onStartRequest: function(request, context) {
        switch (channel.responseStatus) {
          case 301:
          case 302:
          case 303:
            request.cancel(0);
            let location = channel.getResponseHeader("Location");
            CastingApps._getContentTypeForURI(CastingApps.makeURI(location), aCallback);
            break;
          default:
            aCallback(channel.contentType);
            request.cancel(0);
            break;
        }
      },
      onStopRequest: function(request, context, statusCode)  {},
      onDataAvailable: function(request, context, stream, offset, count) {}
    };
    channel.asyncOpen(listener, null)
  },

  
  
  _getVideo: function(aElement, aTypes, aExtensions, aCallback) {
    
    let asyncURIs = [];

    
    let posterURL = aElement.poster;

    
    let sourceURL = aElement.src;

    
    if (!sourceURL) {
      sourceURL = aElement.currentSrc;
    }

    if (sourceURL) {
      
      let sourceURI = this.makeURI(sourceURL, null, this.makeURI(aElement.baseURI));
      if (this.allowableExtension(sourceURI, aExtensions)) {
        aCallback({ element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI});
        return;
      }

      if (aElement.type) {
        
        if (this.allowableMimeType(aElement.type, aTypes)) {
          aCallback({ element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI, type: aElement.type });
          return;
        }
      }

      
      asyncURIs.push(sourceURI);
    }

    
    
    let sourceNodes = aElement.getElementsByTagName("source");
    for (let sourceNode of sourceNodes) {
      let sourceURI = this.makeURI(sourceNode.src, null, this.makeURI(sourceNode.baseURI));

      
      
      if (this.allowableExtension(sourceURI, aExtensions)) {
        aCallback({ element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI, type: sourceNode.type });
        return;
      }

      if (sourceNode.type) {
        
        if (this.allowableMimeType(sourceNode.type, aTypes)) {
          aCallback({ element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI, type: sourceNode.type });
          return;
        }
      }

      
      asyncURIs.push(sourceURI);
    }

    
    
    aCallback.fired = false;
    for (let sourceURI of asyncURIs) {
      
      this._getContentTypeForURI(sourceURI, (aType) => {
        if (!aCallback.fired && this.allowableMimeType(aType, aTypes)) {
          aCallback.fired = true;
          aCallback({ element: aElement, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI, type: aType });
        }
      });
    }

    
    if (!aCallback.fired) {
      aCallback(null);
    }
  },

  
  
  isVideoCastable: function(aElement, aX, aY) {
    
    if (aElement instanceof HTMLVideoElement) {
      return aElement.mozAllowCasting;
    }

    
    
    
    try {
      
      
      let elements = aElement.ownerDocument.querySelectorAll("video");
      for (let element of elements) {
        
        let rect = element.getBoundingClientRect();
        if (aY >= rect.top && aX >= rect.left && aY <= rect.bottom && aX <= rect.right) {
          
          return element.mozAllowCasting;
        }
      }
    } catch(e) {}

    return false;
  },

  filterCast: {
    matches: function(aElement, aX, aY) {
      
      
      if (SimpleServiceDiscovery.services.length == 0)
        return false;
      return CastingApps.isVideoCastable(aElement, aX, aY);
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
    
    this.getVideo(aElement, aX, aY, this._openExternal.bind(this));
  },

  _openExternal: function(aVideo) {
    if (!aVideo) {
      return;
    }

    function filterFunc(aService) {
      return this.allowableExtension(aVideo.sourceURI, aService.extensions) || this.allowableMimeType(aVideo.type, aService.types);
    }

    this.prompt(function(aService) {
      if (!aService)
        return;

      
      let app = SimpleServiceDiscovery.findAppForService(aService);
      if (!app)
        return;

      if (aVideo.element) {
        aVideo.title = aVideo.element.ownerDocument.defaultView.top.document.title;

        
        if (!aVideo.element.paused) {
          aVideo.element.pause();
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
                title: aVideo.title,
                source: aVideo.source,
                poster: aVideo.poster
              },
              videoRef: Cu.getWeakReference(aVideo.element)
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
  }
};
