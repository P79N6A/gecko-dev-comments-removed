










var imports = {};
Cu.import("resource:///modules/devtools/Templater.jsm", imports);
Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js", imports);

function test() {
  addTab("http://example.com/browser/browser/devtools/shared/test/browser_templater_basic.html", function() {
    info("Starting DOM Templater Tests");
    runTest(0);
  });
}

function runTest(index) {
  var options = tests[index] = tests[index]();
  var holder = content.document.createElement('div');
  holder.id = options.name;
  var body = content.document.body;
  body.appendChild(holder);
  holder.innerHTML = options.template;

  info('Running ' + options.name);
  imports.template(holder, options.data, options.options);

  if (typeof options.result == 'string') {
    is(holder.innerHTML, options.result, options.name);
  }
  else {
    ok(holder.innerHTML.match(options.result), options.name);
  }

  if (options.also) {
    options.also(options);
  }

  function runNextTest() {
    index++;
    if (index < tests.length) {
      runTest(index);
    }
    else {
      finished();
    }
  }

  if (options.later) {
    var ais = is.bind(this);

    function createTester(holder, options) {
      return function() {
        ais(holder.innerHTML, options.later, options.name + ' later');
        runNextTest();
      }.bind(this);
    }

    executeSoon(createTester(holder, options));
  }
  else {
    runNextTest();
  }
}

function finished() {
  gBrowser.removeCurrentTab();
  info("Finishing DOM Templater Tests");
  tests = null;
  finish();
}









var tests = [
  function() { return {
    name: 'simpleNesting',
    template: '<div id="ex1">${nested.value}</div>',
    data: { nested:{ value:'pass 1' } },
    result: '<div id="ex1">pass 1</div>'
  };},

  function() { return {
    name: 'returnDom',
    template: '<div id="ex2">${__element.ownerDocument.createTextNode(\'pass 2\')}</div>',
    options: { allowEval: true },
    data: {},
    result: '<div id="ex2">pass 2</div>'
  };},

  function() { return {
    name: 'srcChange',
    template: '<img _src="${fred}" id="ex3">',
    data: { fred:'green.png' },
    result: /<img( id="ex3")? src="green.png"( id="ex3")?>/
  };},

  function() { return {
    name: 'ifTrue',
    template: '<p if="${name !== \'jim\'}">hello ${name}</p>',
    options: { allowEval: true },
    data: { name: 'fred' },
    result: '<p>hello fred</p>'
  };},

  function() { return {
    name: 'ifFalse',
    template: '<p if="${name !== \'jim\'}">hello ${name}</p>',
    options: { allowEval: true },
    data: { name: 'jim' },
    result: ''
  };},

  function() { return {
    name: 'simpleLoop',
    template: '<p foreach="index in ${[ 1, 2, 3 ]}">${index}</p>',
    options: { allowEval: true },
    data: {},
    result: '<p>1</p><p>2</p><p>3</p>'
  };},

  function() { return {
    name: 'loopElement',
    template: '<loop foreach="i in ${array}">${i}</loop>',
    data: { array: [ 1, 2, 3 ] },
    result: '123'
  };},

  
  
  function() { return {
    name: 'asyncLoopElement',
    template: '<loop foreach="i in ${array}">${i}</loop>',
    data: { array: delayReply([1, 2, 3]) },
    result: '<span></span>',
    later: '123'
  };},

  function() { return {
    name: 'saveElement',
    template: '<p save="${element}">${name}</p>',
    data: { name: 'pass 8' },
    result: '<p>pass 8</p>',
    also: function(options) {
      ok(options.data.element.innerHTML, 'pass 9', 'saveElement saved');
      delete options.data.element;
    }
  };},

  function() { return {
    name: 'useElement',
    template: '<p id="pass9">${adjust(__element)}</p>',
    options: { allowEval: true },
    data: {
      adjust: function(element) {
        is('pass9', element.id, 'useElement adjust');
        return 'pass 9b'
      }
    },
    result: '<p id="pass9">pass 9b</p>'
  };},

  function() { return {
    name: 'asyncInline',
    template: '${delayed}',
    data: { delayed: delayReply('inline') },
    result: '<span></span>',
    later: 'inline'
  };},

  
  function() { return {
    name: 'asyncArray',
    template: '<p foreach="i in ${delayed}">${i}</p>',
    data: { delayed: delayReply([1, 2, 3]) },
    result: '<span></span>',
    later: '<p>1</p><p>2</p><p>3</p>'
  };},

  function() { return {
    name: 'asyncMember',
    template: '<p foreach="i in ${delayed}">${i}</p>',
    data: { delayed: [delayReply(4), delayReply(5), delayReply(6)] },
    result: '<span></span><span></span><span></span>',
    later: '<p>4</p><p>5</p><p>6</p>'
  };},

  
  function() { return {
    name: 'asyncBoth',
    template: '<p foreach="i in ${delayed}">${i}</p>',
    data: {
      delayed: delayReply([
        delayReply(4),
        delayReply(5),
        delayReply(6)
      ])
    },
    result: '<span></span>',
    later: '<p>4</p><p>5</p><p>6</p>'
  };},

  
  function() { return {
    name: 'functionReturningUndefiend',
    template: '<p>${foo()}</p>',
    options: { allowEval: true },
    data: {
      foo: function() {}
    },
    result: '<p>undefined</p>'
  };},

  
  function() { return {
    name: 'propertySimple',
    template: '<p>${a.b.c}</p>',
    data: { a: { b: { c: 'hello' } } },
    result: '<p>hello</p>'
  };},

  function() { return {
    name: 'propertyPass',
    template: '<p>${Math.max(1, 2)}</p>',
    options: { allowEval: true },
    result: '<p>2</p>'
  };},

  function() { return {
    name: 'propertyFail',
    template: '<p>${Math.max(1, 2)}</p>',
    result: '<p>${Math.max(1, 2)}</p>'
  };},

  
  
  function() { return {
    name: 'propertyUndefAttrFull',
    template: '<p>${nullvar}|${undefinedvar1}|${undefinedvar2}</p>',
    data: { nullvar: null, undefinedvar1: undefined },
    result: '<p>null|undefined|undefined</p>'
  };},

  function() { return {
    name: 'propertyUndefAttrBlank',
    template: '<p>${nullvar}|${undefinedvar1}|${undefinedvar2}</p>',
    data: { nullvar: null, undefinedvar1: undefined },
    options: { blankNullUndefined: true },
    result: '<p>||</p>'
  };},

  function() { return {
    name: 'propertyUndefAttrFull',
    template: '<div><p value="${nullvar}"></p><p value="${undefinedvar1}"></p><p value="${undefinedvar2}"></p></div>',
    data: { nullvar: null, undefinedvar1: undefined },
    result: '<div><p value="null"></p><p value="undefined"></p><p value="undefined"></p></div>'
  };},

  function() { return {
    name: 'propertyUndefAttrBlank',
    template: '<div><p value="${nullvar}"></p><p value="${undefinedvar1}"></p><p value="${undefinedvar2}"></p></div>',
    data: { nullvar: null, undefinedvar1: undefined },
    options: { blankNullUndefined: true },
    result: '<div><p value=""></p><p value=""></p><p value=""></p></div>'
  };}
];

function delayReply(data) {
  var d = imports.Promise.defer();
  executeSoon(function() {
    d.resolve(data);
  });
  return d.promise;
}
