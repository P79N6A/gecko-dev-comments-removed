




"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
let {Promise: promise} = Cu.import("resource://gre/modules/Promise.jsm", {});
Cu.import("resource:///modules/devtools/VariablesView.jsm");
Cu.import("resource:///modules/devtools/ViewHelpers.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "devtools",
  "resource://gre/modules/devtools/Loader.jsm");

Object.defineProperty(this, "WebConsoleUtils", {
  get: function() {
    return devtools.require("devtools/toolkit/webconsole/utils").Utils;
  },
  configurable: true,
  enumerable: true
});

XPCOMUtils.defineLazyGetter(this, "VARIABLES_SORTING_ENABLED", () =>
  Services.prefs.getBoolPref("devtools.debugger.ui.variables-sorting-enabled")
);

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");

const MAX_LONG_STRING_LENGTH = 200000;
const DBG_STRINGS_URI = "chrome://browser/locale/devtools/debugger.properties";

this.EXPORTED_SYMBOLS = ["VariablesViewController", "StackFrameUtils"];



















function VariablesViewController(aView, aOptions = {}) {
  this.addExpander = this.addExpander.bind(this);

  this._setClientGetters(aOptions);
  this._setEvaluationMacros(aOptions);

  this._actors = new Set();
  this.view = aView;
  this.view.controller = this;
}

VariablesViewController.prototype = {
  


  _getterOrSetterEvalMacro: VariablesView.getterOrSetterEvalMacro,

  


  _overrideValueEvalMacro: VariablesView.overrideValueEvalMacro,

  


  _simpleValueEvalMacro: VariablesView.simpleValueEvalMacro,

  









  _setClientGetters: function(aOptions) {
    if (aOptions.getObjectClient) {
      this._getObjectClient = aOptions.getObjectClient;
    }
    if (aOptions.getLongStringClient) {
      this._getLongStringClient = aOptions.getLongStringClient;
    }
    if (aOptions.getEnvironmentClient) {
      this._getEnvironmentClient = aOptions.getEnvironmentClient;
    }
    if (aOptions.releaseActor) {
      this._releaseActor = aOptions.releaseActor;
    }
  },

  








  _setEvaluationMacros: function(aOptions) {
    if (aOptions.overrideValueEvalMacro) {
      this._overrideValueEvalMacro = aOptions.overrideValueEvalMacro;
    }
    if (aOptions.getterOrSetterEvalMacro) {
      this._getterOrSetterEvalMacro = aOptions.getterOrSetterEvalMacro;
    }
    if (aOptions.simpleValueEvalMacro) {
      this._simpleValueEvalMacro = aOptions.simpleValueEvalMacro;
    }
  },

  









  _populateFromLongString: function(aTarget, aGrip){
    let deferred = promise.defer();

    let from = aGrip.initial.length;
    let to = Math.min(aGrip.length, MAX_LONG_STRING_LENGTH);

    this._getLongStringClient(aGrip).substring(from, to, aResponse => {
      
      this.releaseActor(aGrip);

      
      aTarget.onexpand = null;
      aTarget.setGrip(aGrip.initial + aResponse.substring);
      aTarget.hideArrow();

      deferred.resolve();
    });

    return deferred.promise;
  },

  








  _populateFromObject: function(aTarget, aGrip) {
    let deferred = promise.defer();

    if (aGrip.class === "Promise" && aGrip.promiseState) {
      const { state, value, reason } = aGrip.promiseState;
      aTarget.addItem("<state>", { value: state });
      if (state === "fulfilled") {
        this.addExpander(aTarget.addItem("<value>", { value }), value);
      } else if (state === "rejected") {
        this.addExpander(aTarget.addItem("<reason>", { value: reason }), reason);
      }
    }

    let objectClient = this._getObjectClient(aGrip);
    objectClient.getPrototypeAndProperties(aResponse => {
      let { ownProperties, prototype } = aResponse;
      
      let safeGetterValues = aResponse.safeGetterValues || {};
      let sortable = VariablesView.isSortable(aGrip.class);

      
      
      for (let name of Object.keys(safeGetterValues)) {
        if (name in ownProperties) {
          let { getterValue, getterPrototypeLevel } = safeGetterValues[name];
          ownProperties[name].getterValue = getterValue;
          ownProperties[name].getterPrototypeLevel = getterPrototypeLevel;
        } else {
          ownProperties[name] = safeGetterValues[name];
        }
      }

      
      if (ownProperties) {
        aTarget.addItems(ownProperties, {
          
          sorted: sortable,
          
          callback: this.addExpander
        });
      }

      
      if (prototype && prototype.type != "null") {
        let proto = aTarget.addItem("__proto__", { value: prototype });
        
        this.addExpander(proto, prototype);
      }

      
      
      if (aGrip.class == "Function") {
        objectClient.getScope(aResponse => {
          if (aResponse.error) {
            
            
            
            console.warn(aResponse.error + ": " + aResponse.message);
            return void deferred.resolve();
          }
          this._populateWithClosure(aTarget, aResponse.scope).then(deferred.resolve);
        });
      } else {
        deferred.resolve();
      }
    });

    return deferred.promise;
  },

  







  _populateWithClosure: function(aTarget, aScope) {
    let objectScopes = [];
    let environment = aScope;
    let funcScope = aTarget.addItem("<Closure>");
    funcScope.target.setAttribute("scope", "");
    funcScope.showArrow();

    do {
      
      let label = StackFrameUtils.getScopeLabel(environment);

      
      let closure = funcScope.addItem(label, undefined, true);
      closure.target.setAttribute("scope", "");
      closure.showArrow();

      
      if (environment.bindings) {
        this._populateWithEnvironmentBindings(closure, environment.bindings);
      } else {
        let deferred = promise.defer();
        objectScopes.push(deferred.promise);
        this._getEnvironmentClient(environment).getBindings(response => {
          this._populateWithEnvironmentBindings(closure, response.bindings);
          deferred.resolve();
        });
      }
    } while ((environment = environment.parent));

    return promise.all(objectScopes).then(() => {
      
      this.view.emit("fetched", "scopes", funcScope);
    });
  },

  







  _populateWithEnvironmentBindings: function(aTarget, aBindings) {
    
    aTarget.addItems(aBindings.arguments.reduce((accumulator, arg) => {
      let name = Object.getOwnPropertyNames(arg)[0];
      let descriptor = arg[name];
      accumulator[name] = descriptor;
      return accumulator;
    }, {}), {
      
      sorted: false,
      
      callback: this.addExpander
    });

    
    aTarget.addItems(aBindings.variables, {
      
      sorted: VARIABLES_SORTING_ENABLED,
      
      callback: this.addExpander
    });
  },

  








  addExpander: function(aTarget, aSource) {
    
    if (aTarget.getter || aTarget.setter) {
      aTarget.evaluationMacro = this._overrideValueEvalMacro;
      let getter = aTarget.get("get");
      if (getter) {
        getter.evaluationMacro = this._getterOrSetterEvalMacro;
      }
      let setter = aTarget.get("set");
      if (setter) {
        setter.evaluationMacro = this._getterOrSetterEvalMacro;
      }
    } else {
      aTarget.evaluationMacro = this._simpleValueEvalMacro;
    }

    
    if (VariablesView.isPrimitive({ value: aSource })) {
      return;
    }

    
    if (WebConsoleUtils.isActorGrip(aSource) && aSource.type == "longString") {
      aTarget.showArrow();
    }

    
    aTarget.onexpand = () => this.populate(aTarget, aSource);

    
    
    if (aTarget.shouldPrefetch) {
      aTarget.addEventListener("mouseover", aTarget.onexpand, false);
    }

    
    for (let grip of [aTarget.value, aTarget.getter, aTarget.setter]) {
      if (WebConsoleUtils.isActorGrip(grip)) {
        this._actors.add(grip.actor);
      }
    }
  },

  












  populate: function(aTarget, aSource) {
    
    if (aTarget._fetched) {
      return aTarget._fetched;
    }
    
    if (!aSource) {
      return promise.reject(new Error("No actor grip was given for the variable."));
    }

    let deferred = promise.defer();
    aTarget._fetched = deferred.promise;

    
    if (VariablesView.isVariable(aTarget)) {
      this._populateFromObject(aTarget, aSource).then(() => {
        
        this.view.emit("fetched", "properties", aTarget);
        
        this.view.commitHierarchy();
        deferred.resolve();
      });
      return deferred.promise;
    }

    switch (aSource.type) {
      case "longString":
        this._populateFromLongString(aTarget, aSource).then(() => {
          
          this.view.emit("fetched", "longString", aTarget);
          deferred.resolve();
        });
        break;
      case "with":
      case "object":
        this._populateFromObject(aTarget, aSource.object).then(() => {
          
          this.view.emit("fetched", "variables", aTarget);
          
          this.view.commitHierarchy();
          deferred.resolve();
        });
        break;
      case "block":
      case "function":
        this._populateWithEnvironmentBindings(aTarget, aSource.bindings);
        
        
        
        
        this.view.commitHierarchy();
        deferred.resolve();
        break;
      default:
        let error = "Unknown Debugger.Environment type: " + aSource.type;
        Cu.reportError(error);
        deferred.reject(error);
    }

    return deferred.promise;
  },

  





  releaseActor: function(aActor){
    if (this._releaseActor) {
      this._releaseActor(aActor);
    }
    this._actors.delete(aActor);
  },

  





  releaseActors: function(aFilter) {
    for (let actor of this._actors) {
      if (!aFilter || aFilter(actor)) {
        this.releaseActor(actor);
      }
    }
  },

  



















  setSingleVariable: function(aOptions, aConfiguration = {}) {
    this._setEvaluationMacros(aConfiguration);
    this.view.empty();

    let scope = this.view.addScope(aOptions.label);
    scope.expanded = true; 
    scope.locked = true; 

    let variable = scope.addItem("", { enumerable: true });
    let populated;

    if (aOptions.objectActor) {
      populated = this.populate(variable, aOptions.objectActor);
      variable.expand();
    } else if (aOptions.rawObject) {
      variable.populate(aOptions.rawObject, { expanded: true });
      populated = promise.resolve();
    }

    return { variable: variable, expanded: populated };
  },
};












VariablesViewController.attach = function(aView, aOptions) {
  if (aView.controller) {
    return aView.controller;
  }
  return new VariablesViewController(aView, aOptions);
};




let StackFrameUtils = {
  






  getFrameTitle: function(aFrame) {
    if (aFrame.type == "call") {
      let c = aFrame.callee;
      return (c.name || c.userDisplayName || c.displayName || "(anonymous)");
    }
    return "(" + aFrame.type + ")";
  },

  







  getScopeLabel: function(aEnv) {
    let name = "";

    
    if (!aEnv.parent) {
      name = L10N.getStr("globalScopeLabel");
    }
    
    else {
      name = aEnv.type.charAt(0).toUpperCase() + aEnv.type.slice(1);
    }

    let label = L10N.getFormatStr("scopeLabel", name);
    switch (aEnv.type) {
      case "with":
      case "object":
        label += " [" + aEnv.object.class + "]";
        break;
      case "function":
        let f = aEnv.function;
        label += " [" +
          (f.name || f.userDisplayName || f.displayName || "(anonymous)") +
        "]";
        break;
    }
    return label;
  }
};




let L10N = new ViewHelpers.L10N(DBG_STRINGS_URI);
