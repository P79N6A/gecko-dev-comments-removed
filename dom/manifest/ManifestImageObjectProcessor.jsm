




















'use strict';
this.EXPORTED_SYMBOLS = ['ManifestImageObjectProcessor']; 
const imports = {};
const {
  utils: Cu,
  classes: Cc,
  interfaces: Ci
} = Components;
const scriptLoader = Cc['@mozilla.org/moz/jssubscript-loader;1']
  .getService(Ci.mozIJSSubScriptLoader);
scriptLoader.loadSubScript(
  'resource://gre/modules/manifestValueExtractor.js',
  this); 
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.importGlobalProperties(['URL']);
imports.netutil = Cc['@mozilla.org/network/util;1']
  .getService(Ci.nsINetUtil);
imports.DOMUtils = Cc['@mozilla.org/inspector/dom-utils;1']
  .getService(Ci.inIDOMUtils);

function ManifestImageObjectProcessor() {}


Object.defineProperties(ManifestImageObjectProcessor, {
  'decimals': {
    get: function() {
      return /^\d+$/;
    }
  },
  'anyRegEx': {
    get: function() {
      return new RegExp('any', 'i');
    }
  }
});

ManifestImageObjectProcessor.process = function(
  aManifest, aBaseURL, aMemberName, console
) {
  const spec = {
    objectName: 'manifest',
    object: aManifest,
    property: aMemberName,
    expectedType: 'array',
    trim: false
  };
  const images = [];
  const value = extractValue(spec, console);
  if (Array.isArray(value)) {
    
    value.filter(item => !!processSrcMember(item, aBaseURL))
      .map(toImageObject)
      .forEach(image => images.push(image));
  }
  return images;

  function toImageObject(aImageSpec) {
    return {
      'src': processSrcMember(aImageSpec, aBaseURL),
      'type': processTypeMember(aImageSpec),
      'sizes': processSizesMember(aImageSpec),
      'density': processDensityMember(aImageSpec),
      'background_color': processBackgroundColorMember(aImageSpec)
    };
  }

  function processTypeMember(aImage) {
    const charset = {};
    const hadCharset = {};
    const spec = {
      objectName: 'image',
      object: aImage,
      property: 'type',
      expectedType: 'string',
      trim: true
    };
    let value = extractValue(spec, console);
    if (value) {
      value = imports.netutil.parseContentType(value, charset, hadCharset);
    }
    return value || undefined;
  }

  function processDensityMember(aImage) {
    const value = parseFloat(aImage.density);
    const validNum = Number.isNaN(value) || value === +Infinity || value <=
      0;
    return (validNum) ? 1.0 : value;
  }

  function processSrcMember(aImage, aBaseURL) {
    const spec = {
      objectName: 'image',
      object: aImage,
      property: 'src',
      expectedType: 'string',
      trim: false
    };
    const value = extractValue(spec, console);
    let url;
    if (value && value.length) {
      try {
        url = new URL(value, aBaseURL).href;
      } catch (e) {}
    }
    return url;
  }

  function processSizesMember(aImage) {
    const sizes = new Set();
    const spec = {
      objectName: 'image',
      object: aImage,
      property: 'sizes',
      expectedType: 'string',
      trim: true
    };
    const value = extractValue(spec, console);
    if (value) {
      
      value.split(/\s+/)
        .filter(isValidSizeValue)
        .forEach(size => sizes.add(size));
    }
    return sizes;
    
    function isValidSizeValue(aSize) {
      const size = aSize.toLowerCase();
      if (ManifestImageObjectProcessor.anyRegEx.test(aSize)) {
        return true;
      }
      if (!size.includes('x') || size.indexOf('x') !== size.lastIndexOf('x')) {
        return false;
      }
      
      const widthAndHeight = size.split('x');
      const w = widthAndHeight.shift();
      const h = widthAndHeight.join('x');
      const validStarts = !w.startsWith('0') && !h.startsWith('0');
      const validDecimals = ManifestImageObjectProcessor.decimals.test(w + h);
      return (validStarts && validDecimals);
    }
  }

  function processBackgroundColorMember(aImage) {
    const spec = {
      objectName: 'image',
      object: aImage,
      property: 'background_color',
      expectedType: 'string',
      trim: true
    };
    const value = extractValue(spec, console);
    let color;
    if (imports.DOMUtils.isValidCSSColor(value)) {
      color = value;
    } else {
      const msg = `background_color: ${value} is not a valid CSS color.`;
      console.warn(msg);
    }
    return color;
  }
};
this.ManifestImageObjectProcessor = ManifestImageObjectProcessor; 
