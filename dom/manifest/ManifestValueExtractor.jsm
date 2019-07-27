







'use strict';
const imports = {};
const {
  classes: Cc,
  interfaces: Ci
} = Components;
imports.DOMUtils = Cc['@mozilla.org/inspector/dom-utils;1']
  .getService(Ci.inIDOMUtils);

this.EXPORTED_SYMBOLS = ['ManifestValueExtractor']; 

function ManifestValueExtractor(aConsole) {
  this.console = aConsole;
}

ManifestValueExtractor.prototype = {
  
  
  
  
  
  
  
  
  extractValue({
    expectedType, object, objectName, property, trim
  }) {
    const value = object[property];
    const isArray = Array.isArray(value);
    
    const type = (isArray) ? 'array' : typeof value;
    if (type !== expectedType) {
      if (type !== 'undefined') {
        let msg = `Expected the ${objectName}'s ${property} `;
        msg += `member to be a ${expectedType}.`;
        this.console.log(msg);
      }
      return undefined;
    }
    
    const shouldTrim = expectedType === 'string' && value && trim;
    if (shouldTrim) {
      return value.trim() || undefined;
    }
    return value;
  },
  extractColorValue(spec) {
    const value = this.extractValue(spec);
    let color;
    if (imports.DOMUtils.isValidCSSColor(value)) {
      color = value;
    } else {
      const msg = `background_color: ${value} is not a valid CSS color.`;
      this.console.warn(msg);
    }
    return color;
  }
};

this.ManifestValueExtractor = ManifestValueExtractor; 
