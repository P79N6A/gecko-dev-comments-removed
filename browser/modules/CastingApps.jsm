



"use strict";
this.EXPORTED_SYMBOLS = ["CastingApps"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/SimpleServiceDiscovery.jsm");


var CastingApps = {
  _sendEventToVideo: function (element, data) {
    let event = element.ownerDocument.createEvent("CustomEvent");
    event.initCustomEvent("media-videoCasting", false, true, JSON.stringify(data));
    element.dispatchEvent(event);
  },

  makeURI: function (url, charset, baseURI) {
    return Services.io.newURI(url, charset, baseURI);
  },

  getVideo: function (element) {
    if (!element) {
      return null;
    }

    let extensions = SimpleServiceDiscovery.getSupportedExtensions();
    let types = SimpleServiceDiscovery.getSupportedMimeTypes();

    
    let posterURL = element.poster;

    
    let sourceURL = element.src;

    
    if (!sourceURL) {
      sourceURL = element.currentSrc;
    }

    if (sourceURL) {
      
      let sourceURI = this.makeURI(sourceURL, null, this.makeURI(element.baseURI));
      if (this.allowableExtension(sourceURI, extensions)) {
        return { element: element, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI};
      }
    }

    
    
    let sourceNodes = element.getElementsByTagName("source");
    for (let sourceNode of sourceNodes) {
      let sourceURI = this.makeURI(sourceNode.src, null, this.makeURI(sourceNode.baseURI));

      
      
      if (this.allowableMimeType(sourceNode.type, types) || this.allowableExtension(sourceURI, extensions)) {
        return { element: element, source: sourceURI.spec, poster: posterURL, sourceURI: sourceURI, type: sourceNode.type };
      }
    }

    return null;
  },

  sendVideoToService: function (videoElement, service) {
    if (!service)
      return;

    let video = this.getVideo(videoElement);
    if (!video) {
      return;
    }

    
    let app = SimpleServiceDiscovery.findAppForService(service);
    if (!app)
      return;

    video.title = videoElement.ownerDocument.defaultView.top.document.title;
    if (video.element) {
      
      if (!video.element.paused) {
        video.element.pause();
      }
    }

    app.stop(() => {
      app.start(started => {
        if (!started) {
          Cu.reportError("CastingApps: Unable to start app");
          return;
        }

        app.remoteMedia(remoteMedia => {
          if (!remoteMedia) {
            Cu.reportError("CastingApps: Failed to create remotemedia");
            return;
          }

          this.session = {
            service: service,
            app: app,
            remoteMedia: remoteMedia,
            data: {
              title: video.title,
              source: video.source,
              poster: video.poster
            },
            videoRef: Cu.getWeakReference(video.element)
          };
        }, this);
      });
    });
  },

  getServicesForVideo: function (videoElement) {
    let video = this.getVideo(videoElement);
    if (!video) {
      return {};
    }

    let filteredServices = SimpleServiceDiscovery.services.filter(service => {
      return this.allowableExtension(video.sourceURI, service.extensions) ||
             this.allowableMimeType(video.type, service.types);
    });

    return filteredServices;
  },

  getServicesForMirroring: function () {
    return SimpleServiceDiscovery.services.filter(service => service.mirror);
  },

  
  onRemoteMediaStart: function (remoteMedia) {
    if (!this.session) {
      return;
    }

    remoteMedia.load(this.session.data);

    let video = this.session.videoRef.get();
    if (video) {
      this._sendEventToVideo(video, { active: true });
    }
  },

  onRemoteMediaStop: function (remoteMedia) {
  },

  onRemoteMediaStatus: function (remoteMedia) {
  },

  allowableExtension: function (uri, extensions) {
    return (uri instanceof Ci.nsIURL) && extensions.indexOf(uri.fileExtension) != -1;
  },

  allowableMimeType: function (type, types) {
    return types.indexOf(type) != -1;
  }
};
