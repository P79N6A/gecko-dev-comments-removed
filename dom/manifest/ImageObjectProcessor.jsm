




















'use strict';
const {
  utils: Cu,
  interfaces: Ci,
  classes: Cc
} = Components;

Cu.importGlobalProperties(['URL']);
const netutil = Cc['@mozilla.org/network/util;1']
  .getService(Ci.nsINetUtil);

function ImageObjectProcessor(aConsole, aExtractor) {
  this.console = aConsole;
  this.extractor = aExtractor;
}


Object.defineProperties(ImageObjectProcessor, {
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

ImageObjectProcessor.prototype.process = function(
  aManifest, aBaseURL, aMemberName
) {
  const spec = {
    objectName: 'manifest',
    object: aManifest,
    property: aMemberName,
    expectedType: 'array',
    trim: false
  };
  const extractor = this.extractor;
  const images = [];
  const value = extractor.extractValue(spec);
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
    let value = extractor.extractValue(spec);
    if (value) {
      value = netutil.parseContentType(value, charset, hadCharset);
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
    const value = extractor.extractValue(spec);
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
    const value = extractor.extractValue(spec);
    if (value) {
      
      value.split(/\s+/)
        .filter(isValidSizeValue)
        .forEach(size => sizes.add(size));
    }
    return sizes;
    
    function isValidSizeValue(aSize) {
      const size = aSize.toLowerCase();
      if (ImageObjectProcessor.anyRegEx.test(aSize)) {
        return true;
      }
      if (!size.includes('x') || size.indexOf('x') !== size.lastIndexOf('x')) {
        return false;
      }
      
      const widthAndHeight = size.split('x');
      const w = widthAndHeight.shift();
      const h = widthAndHeight.join('x');
      const validStarts = !w.startsWith('0') && !h.startsWith('0');
      const validDecimals = ImageObjectProcessor.decimals.test(w + h);
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
    return extractor.extractColorValue(spec);
  }
};
this.ImageObjectProcessor = ImageObjectProcessor; 
this.EXPORTED_SYMBOLS = ['ImageObjectProcessor']; 
