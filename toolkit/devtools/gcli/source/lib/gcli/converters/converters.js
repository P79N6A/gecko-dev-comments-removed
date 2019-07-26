















'use strict';

var promise = require('../util/promise');
var util = require('../util/util');
var host = require('../util/host');






var fallbackDomConverter = {
  from: '*',
  to: 'dom',
  exec: function(data, conversionContext) {
    return conversionContext.document.createTextNode(data || '');
  }
};




var fallbackStringConverter = {
  from: '*',
  to: 'string',
  exec: function(data, conversionContext) {
    return data == null ? '' : data.toString();
  }
};




var viewDomConverter = {
  item: 'converter',
  from: 'view',
  to: 'dom',
  exec: function(view, conversionContext) {
    if (!view.isView) {
      view = conversionContext.createView(view);
    }
    return view.toDom(conversionContext.document);
  }
};




var viewStringConverter = {
  item: 'converter',
  from: 'view',
  to: 'string',
  exec: function(view, conversionContext) {
    if (!view.isView) {
      view = conversionContext.createView(view);
    }
    return view.toDom(conversionContext.document).textContent;
  }
};




var stringViewStringConverter = {
  item: 'converter',
  from: 'stringView',
  to: 'string',
  exec: function(view, conversionContext) {
    if (!view.isView) {
      view = conversionContext.createView(view);
    }
    return view.toDom(conversionContext.document).textContent;
  }
};




var errorDomConverter = {
  item: 'converter',
  from: 'error',
  to: 'dom',
  exec: function(ex, conversionContext) {
    var node = util.createElement(conversionContext.document, 'p');
    node.className = 'gcli-error';
    node.textContent = ex;
    return node;
  }
};




var errorStringConverter = {
  item: 'converter',
  from: 'error',
  to: 'string',
  exec: function(ex, conversionContext) {
    return '' + ex;
  }
};




function getChainConverter(first, second) {
  if (first.to !== second.from) {
    throw new Error('Chain convert impossible: ' + first.to + '!=' + second.from);
  }
  return {
    from: first.from,
    to: second.to,
    exec: function(data, conversionContext) {
      var intermediate = first.exec(data, conversionContext);
      return second.exec(intermediate, conversionContext);
    }
  };
}




var converters = {
  from: {}
};




exports.addConverter = function(converter) {
  var fromMatch = converters.from[converter.from];
  if (fromMatch == null) {
    fromMatch = {};
    converters.from[converter.from] = fromMatch;
  }

  fromMatch[converter.to] = converter;
};




exports.removeConverter = function(converter) {
  var fromMatch = converters.from[converter.from];
  if (fromMatch == null) {
    return;
  }

  if (fromMatch[converter.to] === converter) {
    fromMatch[converter.to] = null;
  }
};




function getConverter(from, to) {
  var fromMatch = converters.from[from];
  if (fromMatch == null) {
    return getFallbackConverter(from, to);
  }

  var converter = fromMatch[to];
  if (converter == null) {
    
    
    
    
    if (to === 'dom') {
      converter = fromMatch.view;
      if (converter != null) {
        return getChainConverter(converter, viewDomConverter);
      }
    }

    if (to === 'string') {
      converter = fromMatch.stringView;
      if (converter != null) {
        return getChainConverter(converter, stringViewStringConverter);
      }
      converter = fromMatch.view;
      if (converter != null) {
        return getChainConverter(converter, viewStringConverter);
      }
    }

    return getFallbackConverter(from, to);
  }
  return converter;
}




function getFallbackConverter(from, to) {
  console.error('No converter from ' + from + ' to ' + to + '. Using fallback');

  if (to === 'dom') {
    return fallbackDomConverter;
  }

  if (to === 'string') {
    return fallbackStringConverter;
  }

  throw new Error('No conversion possible from ' + from + ' to ' + to + '.');
}









exports.convert = function(data, from, to, conversionContext) {
  try {
    if (from === to) {
      return promise.resolve(data);
    }

    var converter = getConverter(from, to);
    return host.exec(function() {
      return converter.exec(data, conversionContext);
    });
  }
  catch (ex) {
    var converter = getConverter('error', to);
    return host.exec(function() {
      return converter.exec(ex, conversionContext);
    });
  }
};




exports.items = [
  viewDomConverter, viewStringConverter, stringViewStringConverter,
  errorDomConverter, errorStringConverter
];
