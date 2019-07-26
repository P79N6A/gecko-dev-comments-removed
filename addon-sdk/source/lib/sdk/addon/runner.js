





module.metadata = {
  "stability": "experimental"
};

const { Cc, Ci } = require('chrome');
const { descriptor, Sandbox, evaluate, main, resolveURI } = require('toolkit/loader');
const { once } = require('../system/events');
const { exit, env, staticArgs, name } = require('../system');
const { when: unload } = require('../system/unload');
const { loadReason } = require('../self');
const { rootURI } = require("@loader/options");
const globals = require('../system/globals');

const NAME2TOPIC = {
  'Firefox': 'sessionstore-windows-restored',
  'Fennec': 'sessionstore-windows-restored',
  'SeaMonkey': 'sessionstore-windows-restored',
  'Thunderbird': 'mail-startup-done',
  '*': 'final-ui-startup'
};



const APP_STARTUP = NAME2TOPIC[name] || NAME2TOPIC['*'];


function setDefaultPrefs(prefsURI) {
  const prefs = Cc['@mozilla.org/preferences-service;1'].
                getService(Ci.nsIPrefService).
                QueryInterface(Ci.nsIPrefBranch2);
  const branch = prefs.getDefaultBranch('');
  const sandbox = Sandbox({
    name: prefsURI,
    prototype: {
      pref: function(key, val) {
        switch (typeof val) {
          case 'boolean':
            branch.setBoolPref(key, val);
            break;
          case 'number':
            if (val % 1 == 0) 
              branch.setIntPref(key, val);
            break;
          case 'string':
            branch.setCharPref(key, val);
            break;
        }
      }
    }
  });
  
  evaluate(sandbox, prefsURI);
}

function definePseudo(loader, id, exports) {
  let uri = resolveURI(id, loader.mapping);
  loader.modules[uri] = { exports: exports };
}

function wait(reason, options) {
  once(APP_STARTUP, function() {
    startup(null, options);
  });
}

function startup(reason, options) {
  if (reason === 'startup')
    return wait(reason, options);

  
  Object.defineProperties(options.loader.globals, descriptor(globals));

  
  
  require('../l10n/loader').
    load(rootURI).
    then(null, function failure(error) {
      console.info("Error while loading localization: " + error.message);
    }).
    then(function onLocalizationReady(data) {
      
      
      definePseudo(options.loader, '@l10n/data', data ? data : null);
      run(options);
    });
}

function run(options) {
  try {
    
    
    try {
      
      
      
      if (options.main !== 'test-harness/run-tests')
        require('../l10n/html').enable();
    }
    catch(error) {
      console.exception(error);
    }
    
    
    try {
      require('../l10n/prefs');
    }
    catch(error) {
      console.exception(error);
    }

    
    
    setDefaultPrefs(options.prefsURI);

    
    let program = main(options.loader, options.main);

    if (typeof(program.onUnload) === 'function')
      unload(program.onUnload);

    if (typeof(program.main) === 'function') {

      program.main({
        loadReason: loadReason,
        staticArgs: staticArgs 
      }, { 
        print: function print(_) { dump(_ + '\n') },
        quit: exit 
      });
    }
  } catch (error) {
    console.exception(error);
    throw error;
  }
}
exports.startup = startup;




if (env.CFX_COMMAND === 'run') {
  unload(function(reason) {
    if (reason === 'shutdown')
      exit(0);
  });
}
