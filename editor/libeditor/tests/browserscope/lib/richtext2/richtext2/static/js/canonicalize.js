




























function canonicalizeEntities(str) {
  
  
  var match;
  while (match = str.match(/&#x([0-9A-F]+);/i)) {
    str = str.replace('&#x' + match[1] + ';', String.fromCharCode(parseInt(match[1], 16)));
  }
  while (match = str.match(/&#([0-9]+);/)) {
    str = str.replace('&#' + match[1] + ';', String.fromCharCode(Number(match[1])));
  }
  return str;
}





















function canonicalizeStyle(str, emitFlags) {
  
  str = str.replace(/ ?[\{\}] ?/g, '');

  var attributes = str.split(';');
  var count = attributes.length;
  var resultArr = [];
  
  for (var a = 0; a < count; ++a) {
    
    
    var avPair = attributes[a].match(/ ?([^ :]+) ?: ?(.+)/);
    if (!avPair)
      continue;

    var name  = avPair[1];
    var value = avPair[2].replace(/ $/, '');  

    switch (name) {
      case 'color':
      case 'background-color':
      case 'border-color':
        if (emitFlags.canonicalizeUnits) {
          resultArr.push(name + ': #' + new Color(value).toHexString());
        } else {
          resultArr.push(name + ': ' + value);
        }
        break;
      
      case 'font-family':
        if (emitFlags.canonicalizeUnits) {
          resultArr.push(name + ': ' + new FontName(value).toString());
        } else {
          resultArr.push(name + ': ' + value);
        }
        break;
        
      case 'font-size':
        if (emitFlags.canonicalizeUnits) {
          resultArr.push(name + ': ' + new FontSize(value).toString());
        } else {
          resultArr.push(name + ': ' + value);
        }
        break;
        
      default:
        resultArr.push(name + ': ' + value);  
    }
  }
  
  
  resultArr.sort();

  return resultArr.join('; ') || null;
} 













function canonicalizeSingleAttribute(elemName, attrName, attrValue, emitFlags) {
  
  
  attrValue = attrValue.replace(/\x22/, '\x27');

  switch (attrName) {
    case 'class':
      return emitFlags.emitClass ? attrValue : null;

    case 'id':
      if (!emitFlags.emitID) {
        return null;
      }
      if (attrValue && attrValue.substr(0, 7) == 'editor-') {
        return null;
      }
      return attrValue;

    
    
    case 'style':
      return (emitFlags.emitStyle && attrValue) 
                 ? canonicalizeStyle(attrValue, emitFlags)
                 : null;

    
    case 'onload':
        return null;

    
    case 'bgcolor':
    case 'color':
      if (!attrValue) {
        return null;
      }
      return emitFlags.canonicalizeUnits ? new Color(attrValue).toString() : attrValue;

    
    case 'face':
      return emitFlags.canonicalizeUnits ? new FontName(attrValue).toString() : attrValue;
      
    
    case 'size':
      if (!attrValue) {
        return null;
      }
      switch (elemName) {
        case 'basefont':
        case 'font':
          return emitFlags.canonicalizeUnits ? new FontSize(attrValue).toString() : attrValue;
      }
      return attrValue;
      
    
    
    case 'colspan':
    case 'rowspan':
    case 'span':
      return (attrValue == '1' || attrValue === '') ? null : attrValue;
      
    
    
    
    
    
    
    
    case 'async':
    case 'autofocus':
    case 'checked':
    case 'compact':
    case 'declare':
    case 'defer':
    case 'disabled':
    case 'formnovalidate':
    case 'frameborder':
    case 'ismap':
    case 'loop':
    case 'multiple':
    case 'nohref':
    case 'nosize':
    case 'noshade':
    case 'novalidate':
    case 'nowrap':
    case 'open':
    case 'readonly':
    case 'required':
    case 'reversed':
    case 'seamless':
    case 'selected':
      return attrValue ? attrValue : attrName;
      
    default:
      return attrValue;  
  }
}
 













function canonicalizeElementTag(str, emitFlags) {
  
  str = str.toLowerCase();

  var pos = str.search(' ');

  
  if (pos == -1) {
    return str;
  }

  var elemName = str.substr(0, pos);
  str = str.substr(pos + 1);

  
  
  

  
  
  
  
  var attrs = [];
  var selStartInTag = false;
  var selEndInTag = false;

  while (str) {
    var attrName;
    var attrValue = '';
    
    pos = str.search(/[ =]/);
    if (pos >= 0) {
      attrName = str.substr(0, pos);
      if (str.charAt(pos) == ' ') {
        ++pos;
      }
      if (str.charAt(pos) == '=') {
        ++pos;
        if (str.charAt(pos) == ' ') {
          ++pos;
        }
        str = str.substr(pos);
        switch (str.charAt(0)) {
          case '"':
          case "'":
            pos = str.indexOf(str.charAt(0), 1);
            pos = (pos < 0) ? str.length : pos;
            attrValue = str.substring(1, pos);
            ++pos;
            break;

          default:
            pos = str.indexOf(' ', 0);
            pos = (pos < 0) ? str.length : pos;
            attrValue = (pos == -1) ? str : str.substr(0, pos);
            break;
        }
        attrValue = attrValue.replace(/^ /, '');         
        attrValue = attrValue.replace(/ $/, '');
      }
    } else {
      attrName = str;
    }
    str = (pos == -1 || pos >= str.length) ? '' : str.substr(pos + 1);

    
    switch (attrName) {
      case ATTRNAME_SEL_START:
        selStartInTag = true;
        continue;
      
      case ATTRNAME_SEL_END:
        selEndInTag = true;
        continue;
    }

    switch (attrName) {
      case '':
      case 'onload':
      case 'xmlns':
        break;
        
      default:
        if (!emitFlags.emitAttrs) {
          break;
        }
        
        
      case 'contenteditable':
        attrValue = canonicalizeEntities(attrValue);
        attrValue = canonicalizeSingleAttribute(elemName, attrName, attrValue, emitFlags);
        if (attrValue !== null) {
          attrs.push(attrName + '="' + attrValue + '"');
        }
    }
  }

  var result = elemName;

  
  
  if (attrs.length > 0) {
    attrs.sort();
    result += ' ' + attrs.join(' ');
  }

  
  if (selStartInTag && selEndInTag) {
    result += ' |';
  } else if (selStartInTag) {
    result += ' {';
  } else if (selEndInTag) {
    result += ' }';
  }

  return result;
}












function canonicalizeElementsAndAttributes(str, emitFlags) {
  var tagStart = str.indexOf('<');
  var tagEnd   = 0;
  var result   = '';

  while (tagStart >= 0) {
    ++tagStart;
    if (str.charAt(tagStart) == '/') {
      ++tagStart;
    }
    result = result + canonicalizeEntities(str.substring(tagEnd, tagStart));
    tagEnd = str.indexOf('>', tagStart);
    if (tagEnd < 0) {
      tagEnd = str.length - 1;  
    }
    if (str.charAt(tagEnd - 1) == '/') {
      --tagEnd;
    }
    var elemStr = str.substring(tagStart, tagEnd);
    elemStr = canonicalizeElementTag(elemStr, emitFlags);
    result = result + elemStr;
    tagStart = str.indexOf('<', tagEnd);
  }
  return result + canonicalizeEntities(str.substring(tagEnd));
}










function canonicalizeSpaces(str) {
  
  str = str.replace(/\s+/g, ' ');

  
  
  str = str.replace(/\< ?/g, '<');
  str = str.replace(/\<\/ ?/g, '</');
  str = str.replace(/ ?\/?\>/g, '>');
  
  return str;
}












function initialCanonicalizationOf(str) {
  str = canonicalizeSpaces(str);
  str = str.replace(/ ?<!-- ?/g, '');
  str = str.replace(/ ?--> ?/g, '');
  str = str.replace(/<\/[bh]r>/g, '');
  
  return str;
}
