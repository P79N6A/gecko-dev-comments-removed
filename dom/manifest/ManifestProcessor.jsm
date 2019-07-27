





















'use strict';
const {
  utils: Cu
} = Components;
Cu.importGlobalProperties(['URL']);
const displayModes = new Set(['fullscreen', 'standalone', 'minimal-ui',
  'browser'
]);
const orientationTypes = new Set(['any', 'natural', 'landscape', 'portrait',
  'portrait-primary', 'portrait-secondary', 'landscape-primary',
  'landscape-secondary'
]);
Cu.import('resource://gre/modules/devtools/Console.jsm');


Cu.import('resource://gre/modules/ValueExtractor.jsm');

Cu.import('resource://gre/modules/ImageObjectProcessor.jsm');

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
    jsonText,
    manifestURL: aManifestURL,
    docURL: aDocURL
  }) {
    const console = new ConsoleAPI({
      prefix: 'Web Manifest: '
    });
    const manifestURL = new URL(aManifestURL);
    const docURL = new URL(aDocURL);
    let rawManifest = {};
    try {
      rawManifest = JSON.parse(jsonText);
    } catch (e) {}
    if (typeof rawManifest !== 'object' || rawManifest === null) {
      let msg = 'Manifest needs to be an object.';
      console.warn(msg);
      rawManifest = {};
    }
    const extractor = new ValueExtractor(console);
    const imgObjProcessor = new ImageObjectProcessor(console, extractor);
    const processedManifest = {
      'lang': processLangMember(),
      'start_url': processStartURLMember(),
      'display': processDisplayMember(),
      'orientation': processOrientationMember(),
      'name': processNameMember(),
      'icons': imgObjProcessor.process(
        rawManifest, manifestURL, 'icons'
      ),
      'splash_screens': imgObjProcessor.process(
        rawManifest, manifestURL, 'splash_screens'
      ),
      'short_name': processShortNameMember(),
      'theme_color': processThemeColorMember(),
    };
    processedManifest.scope = processScopeMember();
    return processedManifest;

    function processNameMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'name',
        expectedType: 'string',
        trim: true
      };
      return extractor.extractValue(spec);
    }

    function processShortNameMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'short_name',
        expectedType: 'string',
        trim: true
      };
      return extractor.extractValue(spec);
    }

    function processOrientationMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'orientation',
        expectedType: 'string',
        trim: true
      };
      const value = extractor.extractValue(spec);
      if (ManifestProcessor.orientationTypes.has(value)) {
        return value;
      }
      
      return '';
    }

    function processDisplayMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'display',
        expectedType: 'string',
        trim: true
      };
      const value = extractor.extractValue(spec);
      if (ManifestProcessor.displayModes.has(value)) {
        return value;
      }
      return ManifestProcessor.defaultDisplayMode;
    }

    function processScopeMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'scope',
        expectedType: 'string',
        trim: false
      };
      let scopeURL;
      const startURL = new URL(processedManifest.start_url);
      const value = extractor.extractValue(spec);
      if (value === undefined || value === '') {
        return undefined;
      }
      try {
        scopeURL = new URL(value, manifestURL);
      } catch (e) {
        let msg = 'The URL of scope is invalid.';
        console.warn(msg);
        return undefined;
      }
      if (scopeURL.origin !== docURL.origin) {
        let msg = 'Scope needs to be same-origin as Document.';
        console.warn(msg);
        return undefined;
      }
      
      let isSameOrigin = startURL && startURL.origin !== scopeURL.origin;
      if (isSameOrigin || !startURL.pathname.startsWith(scopeURL.pathname)) {
        let msg =
          'The start URL is outside the scope, so scope is invalid.';
        console.warn(msg);
        return undefined;
      }
      return scopeURL.href;
    }

    function processStartURLMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'start_url',
        expectedType: 'string',
        trim: false
      };
      let result = new URL(docURL).href;
      const value = extractor.extractValue(spec);
      if (value === undefined || value === '') {
        return result;
      }
      let potentialResult;
      try {
        potentialResult = new URL(value, manifestURL);
      } catch (e) {
        console.warn('Invalid URL.');
        return result;
      }
      if (potentialResult.origin !== docURL.origin) {
        let msg = 'start_url must be same origin as document.';
        console.warn(msg);
      } else {
        result = potentialResult.href;
      }
      return result;
    }

    function processThemeColorMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'theme_color',
        expectedType: 'string',
        trim: true
      };
      return extractor.extractColorValue(spec);
    }

    function processLangMember() {
      const spec = {
        objectName: 'manifest',
        object: rawManifest,
        property: 'lang',
        expectedType: 'string',
        trim: true
      };
      let tag = extractor.extractValue(spec);
      
      
      
      
      
      
      
      
      return tag;
    }
  }
};
this.ManifestProcessor = ManifestProcessor; 
this.EXPORTED_SYMBOLS = ['ManifestProcessor']; 
