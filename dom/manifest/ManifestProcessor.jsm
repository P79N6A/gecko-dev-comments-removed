




















'use strict';
this.EXPORTED_SYMBOLS = ['ManifestProcessor'];
const imports = {};
const {
  utils: Cu,
  classes: Cc,
  interfaces: Ci
} = Components;
Cu.import('resource://gre/modules/XPCOMUtils.jsm');
Cu.importGlobalProperties(['URL']);
XPCOMUtils.defineLazyModuleGetter(imports, 'Services',
  'resource://gre/modules/Services.jsm');
imports.netutil = Cc['@mozilla.org/network/util;1'].getService(Ci.nsINetUtil);


function extractValue({
  objectName,
  object,
  property,
  expectedType,
  trim
}, console) {
  const value = object[property];
  const isArray = Array.isArray(value);
  
  const type = (isArray) ? 'array' : typeof value;
  if (type !== expectedType) {
    if (type !== 'undefined') {
      let msg = `Expected the ${objectName}'s ${property} `;
      msg += `member to a be a ${expectedType}.`;
      console.log(msg);
    }
    return undefined;
  }
  
  const shouldTrim = expectedType === 'string' && value && trim;
  if (shouldTrim) {
    return value.trim() || undefined;
  }
  return value;
}
const displayModes = new Set(['fullscreen', 'standalone', 'minimal-ui',
  'browser'
]);
const orientationTypes = new Set(['any', 'natural', 'landscape', 'portrait',
  'portrait-primary', 'portrait-secondary', 'landscape-primary',
  'landscape-secondary'
]);
const {
  ConsoleAPI
} = Cu.import('resource://gre/modules/devtools/Console.jsm');

function ManifestProcessor() {}


Object.defineProperties(ManifestProcessor, {
  'defaultDisplayMode': {
    get: function() {
      return 'browser';
    }
  },
  'displayModes': {
    get: function() {
      return displayModes;
    }
  },
  'orientationTypes': {
    get: function() {
      return orientationTypes;
    }
  }
});

ManifestProcessor.prototype = {

  
  
  
  
  
  
  process({
    jsonText, manifestURL, docURL
  }) {
    const console = new ConsoleAPI({
      prefix: 'Web Manifest: '
    });
    let rawManifest = {};
    try {
      rawManifest = JSON.parse(jsonText);
    } catch (e) {}
    if (typeof rawManifest !== 'object' || rawManifest === null) {
      let msg = 'Manifest needs to be an object.';
      console.warn(msg);
      rawManifest = {};
    }
    const processedManifest = {
      start_url: processStartURLMember(rawManifest, manifestURL, docURL),
      display: processDisplayMember(rawManifest),
      orientation: processOrientationMember(rawManifest),
      name: processNameMember(rawManifest),
      icons: IconsProcessor.process(rawManifest, manifestURL, console),
      short_name: processShortNameMember(rawManifest),
    };
    processedManifest.scope = processScopeMember(rawManifest, manifestURL,
      docURL, processedManifest.start_url);
    return processedManifest;

    function processNameMember(aManifest) {
      const spec = {
        objectName: 'manifest',
        object: aManifest,
        property: 'name',
        expectedType: 'string',
        trim: true
      };
      return extractValue(spec, console);
    }

    function processShortNameMember(aManifest) {
      const spec = {
        objectName: 'manifest',
        object: aManifest,
        property: 'short_name',
        expectedType: 'string',
        trim: true
      };
      return extractValue(spec, console);
    }

    function processOrientationMember(aManifest) {
      const spec = {
        objectName: 'manifest',
        object: aManifest,
        property: 'orientation',
        expectedType: 'string',
        trim: true
      };
      const value = extractValue(spec, console);
      if (ManifestProcessor.orientationTypes.has(value)) {
        return value;
      }
      
      return '';
    }

    function processDisplayMember(aManifest) {
      const spec = {
        objectName: 'manifest',
        object: aManifest,
        property: 'display',
        expectedType: 'string',
        trim: true
      };
      const value = extractValue(spec, console);
      if (ManifestProcessor.displayModes.has(value)) {
        return value;
      }
      return ManifestProcessor.defaultDisplayMode;
    }

    function processScopeMember(aManifest, aManifestURL, aDocURL, aStartURL) {
      const spec = {
        objectName: 'manifest',
        object: aManifest,
        property: 'scope',
        expectedType: 'string',
        trim: false
      };
      const value = extractValue(spec, console);
      let scopeURL;
      try {
        scopeURL = new URL(value, aManifestURL);
      } catch (e) {
        let msg = 'The URL of scope is invalid.';
        console.warn(msg);
        return undefined;
      }
      if (scopeURL.origin !== aDocURL.origin) {
        let msg = 'Scope needs to be same-origin as Document.';
        console.warn(msg);
        return undefined;
      }
      
      let isSameOrigin = aStartURL && aStartURL.origin !== scopeURL.origin;
      if (isSameOrigin || !aStartURL.pathname.startsWith(scopeURL.pathname)) {
        let msg =
          'The start URL is outside the scope, so scope is invalid.';
        console.warn(msg);
        return undefined;
      }
      return scopeURL;
    }

    function processStartURLMember(aManifest, aManifestURL, aDocURL) {
      const spec = {
        objectName: 'manifest',
        object: aManifest,
        property: 'start_url',
        expectedType: 'string',
        trim: false
      };
      let result = new URL(aDocURL);
      const value = extractValue(spec, console);
      if (value === undefined || value === '') {
        return result;
      }
      let potentialResult;
      try {
        potentialResult = new URL(value, aManifestURL);
      } catch (e) {
        console.warn('Invalid URL.');
        return result;
      }
      if (potentialResult.origin !== aDocURL.origin) {
        let msg = 'start_url must be same origin as document.';
        console.warn(msg);
      } else {
        result = potentialResult;
      }
      return result;
    }
  }
};
this.ManifestProcessor = ManifestProcessor;

function IconsProcessor() {}


Object.defineProperties(IconsProcessor,{
  'onlyDecimals': {
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

IconsProcessor.process = function(aManifest, aBaseURL, console) {
  const spec = {
    objectName: 'manifest',
    object: aManifest,
    property: 'icons',
    expectedType: 'array',
    trim: false
  };
  const icons = [];
  const value = extractValue(spec, console);
  if (Array.isArray(value)) {
    
    value.filter(item => !!processSrcMember(item, aBaseURL))
      .map(toIconObject)
      .forEach(icon => icons.push(icon));
  }
  return icons;

  function toIconObject(aIconData) {
    return {
      src: processSrcMember(aIconData, aBaseURL),
      type: processTypeMember(aIconData),
      sizes: processSizesMember(aIconData),
      density: processDensityMember(aIconData)
    };
  }

  function processTypeMember(aIcon) {
    const charset = {};
    const hadCharset = {};
    const spec = {
      objectName: 'icon',
      object: aIcon,
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

  function processDensityMember(aIcon) {
    const value = parseFloat(aIcon.density);
    const validNum = Number.isNaN(value) || value === +Infinity || value <=
      0;
    return (validNum) ? 1.0 : value;
  }

  function processSrcMember(aIcon, aBaseURL) {
    const spec = {
      objectName: 'icon',
      object: aIcon,
      property: 'src',
      expectedType: 'string',
      trim: false
    };
    const value = extractValue(spec, console);
    let url;
    if (value && value.length) {
      try {
        url = new URL(value, aBaseURL);
      } catch (e) {}
    }
    return url;
  }

  function processSizesMember(aIcon) {
    const sizes = new Set(),
      spec = {
        objectName: 'icon',
        object: aIcon,
        property: 'sizes',
        expectedType: 'string',
        trim: true
      },
      value = extractValue(spec, console);
    if (value) {
      
      value.split(/\s+/)
        .filter(isValidSizeValue)
        .forEach(size => sizes.add(size));
    }
    return sizes;
    
    function isValidSizeValue(aSize) {
      const size = aSize.toLowerCase();
      if (IconsProcessor.anyRegEx.test(aSize)) {
        return true;
      }
      if (!size.contains('x') || size.indexOf('x') !== size.lastIndexOf('x')) {
        return false;
      }
      
      const widthAndHeight = size.split('x');
      const w = widthAndHeight.shift();
      const h = widthAndHeight.join('x');
      const validStarts = !w.startsWith('0') && !h.startsWith('0');
      const validDecimals = IconsProcessor.onlyDecimals.test(w + h);
      return (validStarts && validDecimals);
    }
  }
};
