



"use strict";

this.EXPORTED_SYMBOLS = ["PageMetadata"];

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "UnescapeService",
                                   "@mozilla.org/feed-unescapehtml;1",
                                   "nsIScriptableUnescapeHTML");







const DISCOVER_IMAGES_MAX  = 5;






this.PageMetadata = {
  













  getData(document) {
    let result = {
      url: this._validateURL(document, document.documentURI),
      title: document.title,
      previews: [],
    };

    
    
    
    
    if (document.defaultView) {
      let docshell = document.defaultView.QueryInterface(Ci.nsIInterfaceRequestor)
                                         .getInterface(Ci.nsIWebNavigation)
                                         .QueryInterface(Ci.nsIDocShell);
      let shentry = {};
      if (docshell.getCurrentSHEntry(shentry) &&
          shentry.value && shentry.value.URIWasModified) {
        return result;
      }
    }

    this._getMetaData(document, result);
    this._getLinkData(document, result);
    this._getPageData(document, result);
    result.microdata = this.getMicrodata(document);

    return result;
  },

  























  getMicrodata(document, target = null) {
    function getObject(item) {
      let result = {};

      if (item.itemType.length) {
        result.types = [...item.itemType];
      }

      if (item.itemId) {
        result.itemId = item.itemId;
      }

      if (item.properties.length) {
        result.properties = {};
      }

      for (let elem of item.properties) {
        let value;
        if (elem.itemScope) {
          value = getObject(elem);
        } else if (elem.itemValue) {
          value = elem.itemValue;
        } else if (elem.hasAttribute("content")) {
          
          value = elem.getAttribute("content");
        }

        for (let prop of elem.itemProp) {
          if (!result.properties[prop]) {
            result.properties[prop] = [];
          }

          result.properties[prop].push(value);
        }
      }

      return result;
    }

    let result = { items: [] };
    let elements = target ? [target] : document.getItems();

    for (let element of elements) {
      if (element.itemScope) {
        result.items.push(getObject(element));
      }
    }

    return result;
  },

  






  _getMetaData(document, result) {
    
    let elements = document.querySelectorAll("head > meta[property], head > meta[name]");
    if (elements.length < 1) {
      return;
    }

    for (let element of elements) {
      let value = element.getAttribute("content")
      if (!value) {
        continue;
      }
      value = UnescapeService.unescape(value.trim());

      let key = element.getAttribute("property") || element.getAttribute("name");
      if (!key) {
        continue;
      }

      
      
      
      result[key] = value;

      switch (key) {
        case "title":
        case "og:title": {
          result.title = value;
          break;
        }

        case "description":
        case "og:description": {
          result.description = value;
          break;
        }

        case "og:site_name": {
          result.siteName = value;
          break;
        }

        case "medium":
        case "og:type": {
          result.medium = value;
          break;
        }

        case "og:video": {
          let url = this._validateURL(document, value);
          if (url) {
            result.source = url;
          }
          break;
        }

        case "og:url": {
          let url = this._validateURL(document, value);
          if (url) {
            result.url = url;
          }
          break;
        }

        case "og:image": {
          let url = this._validateURL(document, value);
          if (url) {
            result.previews.push(url);
          }
          break;
        }
      }
    }
  },

  






  _getLinkData: function(document, result) {
    let elements = document.querySelectorAll("head > link[rel], head > link[id]");

    for (let element of elements) {
      let url = element.getAttribute("href");
      if (!url) {
        continue;
      }
      url = this._validateURL(document, UnescapeService.unescape(url.trim()));

      let key = element.getAttribute("rel") || element.getAttribute("id");
      if (!key) {
        continue;
      }

      switch (key) {
        case "shorturl":
        case "shortlink": {
          result.shortUrl = url;
          break;
        }

        case "canonicalurl":
        case "canonical": {
          result.url = url;
          break;
        }

        case "image_src": {
          result.previews.push(url);
          break;
        }

        case "alternate": {
          
          
          
          
          
          if (!result.alternate) {
            result.alternate = [];
          }

          result.alternate.push({
            type: element.getAttribute("type"),
            href: element.getAttribute("href"),
            title: element.getAttribute("title")
          });
        }
      }
    }
  },

  









  _getPageData(document, result) {
    if (result.previews.length < 1) {
      result.previews = this._getImageUrls(document);
    }
  },

  









  _getImageUrls(document) {
    let result = [];
    let elements = document.querySelectorAll("img");

    for (let element of elements) {
      let src = element.getAttribute("src");
      if (src) {
        result.push(this._validateURL(document, UnescapeService.unescape(src)));

        
        
        if (result.length > DISCOVER_IMAGES_MAX) {
          break;
        }
      }
    }

    return result;
  },

  








  _validateURL(document, url) {
    let docURI = Services.io.newURI(document.documentURI, null, null);
    let uri = Services.io.newURI(docURI.resolve(url), null, null);

    if (["http", "https"].indexOf(uri.scheme) < 0) {
      return null;
    }

    uri.userPass = "";

    return uri.spec;
  },
};
