















'use strict';

var host = require('../util/host');
var prism = require('../util/prism').Prism;

function isMultiline(text) {
  return typeof text === 'string' && text.indexOf('\n') > -1;
}

exports.items = [
  {
    
    item: 'language',
    name: 'javascript',
    prompt: '>',

    constructor: function(terminal) {
      this.document = terminal.document;
      this.focusManager = terminal.focusManager;

      this.updateHints();
    },

    destroy: function() {
      this.document = undefined;
    },

    exec: function(input) {
      return this.eval(input).then(function(response) {
        
        var output = (response.exception != null) ?
                      response.exception.class :
                      response.output;

        var isSameString = typeof output === 'string' &&
                           input.substr(1, input.length - 2) === output;
        var isSameOther = typeof output !== 'string' &&
                          input === '' + output;

        
        if (typeof output === 'string' && response.exception == null) {
          if (output.indexOf('\'') === -1) {
            output = '\'' + output + '\'';
          }
          else {
            output = output.replace(/\\/, '\\').replace(/"/, '"').replace(/'/, '\'');
            output = '"' + output + '"';
          }
        }

        var line;
        if (isSameString || isSameOther || output === undefined) {
          line = input;
        }
        else if (isMultiline(output)) {
          line = input + '\n/*\n' + output + '\n*/';
        }
        else {
          line = input + ' // ' + output;
        }

        var grammar = prism.languages[this.name];
        return prism.highlight(line, grammar, this.name);
      }.bind(this));
    },

    eval: function(input) {
      return host.script.eval(input);
    }
  }
];
