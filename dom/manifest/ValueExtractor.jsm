







'use strict';
const {
  classes: Cc,
  interfaces: Ci
} = Components;

function ValueExtractor(aConsole) {
  this.console = aConsole;
}

ValueExtractor.prototype = {
  
  
  
  
  
  
  
  
  extractValue({expectedType, object, objectName, property, trim}) {
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
    const DOMUtils = Cc['@mozilla.org/inspector/dom-utils;1']
      .getService(Ci.inIDOMUtils);
    let color;
    if (DOMUtils.isValidCSSColor(value)) {
      color = value;
    } else if (value) {
      const msg = `background_color: ${value} is not a valid CSS color.`;
      this.console.warn(msg);
    }
    return color;
  }
};
this.ValueExtractor = ValueExtractor; 
this.EXPORTED_SYMBOLS = ['ValueExtractor']; 
