

























var metadata_generator = {

    currentMetadata: {},
    cachedMetadata: false,
    metadataProperties: ['help', 'assert', 'author'],
    
    error: function(message) {
        var messageElement = document.createElement('p');
        messageElement.setAttribute('class', 'error');
        this.appendText(messageElement, message);
        
        var summary = document.getElementById('summary');
        if (summary) {
            summary.parentNode.insertBefore(messageElement, summary);
        }
        else {
            document.body.appendChild(messageElement);
        }
    },

    


    validateContact: function(test, propertyName) {
        var result = true;
        var value = test.properties[propertyName];
        var values = Array.isArray(value) ? value : [value];
        for (var index = 0; index < values.length; index++) {
            value = values[index];
            var re = /(\S+)(\s*)<(.*)>(.*)/;
            if (! re.test(value)) {
                re = /(\S+)(\s+)(http[s]?:\/\/)(.*)/
                if (! re.test(value)) {
                    this.error('Metadata property "' + propertyName + 
                        '" for test: "' + test.name +
                        '" must have name and contact information ' +
                        '("name <email>" or "name http(s)://")');
                    result = false;
                }
            }
        }
        return result;
    },
    
    


    extractFromTest: function(test) {
        var testMetadata = {};
        
        for (var metaIndex = 0; metaIndex < this.metadataProperties.length;
             metaIndex++) {
            var meta = this.metadataProperties[metaIndex];
            if (test.properties.hasOwnProperty(meta)) {
                if ('author' == meta) {
                    this.validateContact(test, meta);
                }
                testMetadata[meta] = test.properties[meta];
            }
        }
        return testMetadata;
    },
    
    


    validateCache: function() {
        for (var testName in this.currentMetadata) {
            if (! this.cachedMetadata.hasOwnProperty(testName)) {
                return false;
            }
            var testMetadata = this.currentMetadata[testName];
            var cachedTestMetadata = this.cachedMetadata[testName];
            delete this.cachedMetadata[testName];
            
            for (var metaIndex = 0; metaIndex < this.metadataProperties.length;
                 metaIndex++) {
                var meta = this.metadataProperties[metaIndex];
                if (cachedTestMetadata.hasOwnProperty(meta) && 
                    testMetadata.hasOwnProperty(meta)) {
                    if (Array.isArray(cachedTestMetadata[meta])) {
                      if (! Array.isArray(testMetadata[meta])) {
                          return false;
                      }
                      if (cachedTestMetadata[meta].length == 
                          testMetadata[meta].length) {
                          for (var index = 0; 
                               index < cachedTestMetadata[meta].length; 
                               index++) {
                              if (cachedTestMetadata[meta][index] != 
                                  testMetadata[meta][index]) {
                                  return false;
                              }
                          }
                      }
                      else {
                          return false;
                      }
                    }
                    else {
                      if (Array.isArray(testMetadata[meta])) {
                        return false;
                      }
                      if (cachedTestMetadata[meta] != testMetadata[meta]) {
                        return false;
                      }
                    }
                }
                else if (cachedTestMetadata.hasOwnProperty(meta) || 
                         testMetadata.hasOwnProperty(meta)) {
                    return false;
                }
            }
        }
        for (var testName in this.cachedMetadata) {
            return false;
        }
        return true;
    },
  
    appendText: function(elemement, text) {
        elemement.appendChild(document.createTextNode(text));
    },
  
    jsonifyArray: function(arrayValue, indent) {
        var output = '[';

        if (1 == arrayValue.length) {
            output += JSON.stringify(arrayValue[0]);
        }
        else {
            for (var index = 0; index < arrayValue.length; index++) {
                if (0 < index) {
                    output += ',\n  ' + indent;
                }
                output += JSON.stringify(arrayValue[index]);
            }
        }
        output += ']';
        return output;
    },
    
    jsonifyObject: function(objectValue, indent) {
        var output = '{';
        
        var count = 0;
        for (var property in objectValue) {
            ++count;
            if (Array.isArray(objectValue[property]) || 
                ('object' == typeof(value))) {
                ++count;
            }
        }
        if (1 == count) {
            for (var property in objectValue) {
                output += ' "' + property + '": '
                          + JSON.stringify(objectValue[property])
                          + ' ';
            }
        }
        else {
            var first = true;
            for (var property in objectValue) {
                if (! first) {
                    output += ',';
                }
                first = false;
                output += '\n  ' + indent + '"' + property + '": ';
                var value = objectValue[property];
                if (Array.isArray(value)) {
                    output += this.jsonifyArray(value, indent + 
                        '                '.substr(0, 5 + property.length));
                }
                else if ('object' == typeof(value)) {
                    output += this.jsonifyObject(value, indent + '  ');
                }
                else {
                    output += JSON.stringify(value);
                }
            }
            if (1 < output.length) {
                output += '\n' + indent;
            }
        }
        output += '}';
        return output;
    },
  
    



    generateSource: function() {
        var source = 
            '<script id="metadata_cache">/*\n' + 
            this.jsonifyObject(this.currentMetadata, '') + '\n' + 
            '*/</script>\n';
        return source;
    },
    
    


    addSourceElement: function(event) {
        var sourceWrapper = document.createElement('div');
        sourceWrapper.setAttribute('id', 'metadata_source');

        var instructions = document.createElement('p');
        if (this.cachedMetadata) {
            this.appendText(instructions, 
                'Replace the existing <script id="metadata_cache"> element ' + 
                'in the test\'s <head> with the following:');
        }
        else {
            this.appendText(instructions, 
                'Copy the following into the <head> element of the test ' +
                'or the test\'s metadata sidecar file:');
        }
        sourceWrapper.appendChild(instructions);
        
        var sourceElement = document.createElement('pre');
        this.appendText(sourceElement, this.generateSource());

        sourceWrapper.appendChild(sourceElement);
        
        var messageElement = document.getElementById('metadata_issue');
        messageElement.parentNode.insertBefore(sourceWrapper, 
                                               messageElement.nextSibling);
        messageElement.parentNode.removeChild(messageElement);

        (event.preventDefault) ? event.preventDefault() : 
                                 event.returnValue = false;
    },
    
    


    getCachedMetadata: function() {
        var cacheElement = document.getElementById('metadata_cache');
        
        if (cacheElement) {
            var cacheText = cacheElement.firstChild.nodeValue;
            var openBrace = cacheText.indexOf('{');
            var closeBrace = cacheText.lastIndexOf('}');
            if ((-1 < openBrace) && (-1 < closeBrace)) {
                cacheText = cacheText.slice(openBrace, closeBrace + 1);
                try {
                    this.cachedMetadata = JSON.parse(cacheText);
                }
                catch (exc) {
                    this.cachedMetadata = 'Invalid JSON in Cached metadata. ';
                }
            }
            else {
                this.cachedMetadata = 'Metadata not found in cache element. ';
            }
        }
    },
    
    




    process: function(tests, harness_status) {
        for (var index = 0; index < tests.length; index++) {
            var test = tests[index];
            if (this.currentMetadata.hasOwnProperty(test.name)) {
                this.error('Duplicate test name: ' + test.name);
            }
            else {
                this.currentMetadata[test.name] = this.extractFromTest(test);
            }
        }

        this.getCachedMetadata();
        
        var message = null;
        var messageClass = 'warning';
        var showSource = false;
        
        if (0 == tests.length) {
            if (this.cachedMetadata) {
                message = 'Cached metadata present but no tests. ';
            }
        }
        else if (1 == tests.length) {
            if (this.cachedMetadata) {
                message = 'Single test files should not have cached metadata. ';
            }
            else {
                var testMetadata = this.currentMetadata[tests[0].name];
                var hasMetadata = false;
                for (var meta in testMetadata) {
                    hasMetadata |= testMetadata.hasOwnProperty(meta);
                }
                if (hasMetadata) {
                    message = 'Single tests should not have metadata. ' +
                              'Move metadata to <head>. ';
                }
            }
        }
        else {
            if (this.cachedMetadata) {
                messageClass = 'error';
                if ('string' == typeof(this.cachedMetadata)) {
                    message = this.cachedMetadata;
                    showSource = true;
                }
                else if (! this.validateCache()) {
                    message = 'Cached metadata out of sync. ';
                    showSource = true;
                }
            }
        }
        
        if (message) {
            var messageElement = document.createElement('p');
            messageElement.setAttribute('id', 'metadata_issue');
            messageElement.setAttribute('class', messageClass);
            this.appendText(messageElement, message);
            
            if (showSource) {
                var link = document.createElement('a');
                this.appendText(link, 'Click for source code.');
                link.setAttribute('href', '#');
                link.setAttribute('onclick', 
                                  'metadata_generator.addSourceElement(event)');
                messageElement.appendChild(link);
            }
            
            var summary = document.getElementById('summary');
            if (summary) {
                summary.parentNode.insertBefore(messageElement, summary);
            }
            else {
                var log = document.getElementById('log');
                if (log) {
                    log.appendChild(messageElement);
                }
            }
        }
    },

    setup: function() {
        add_completion_callback(
            function (tests, harness_status) { 
                metadata_generator.process(tests, harness_status)
            });
    }
}

metadata_generator.setup();

