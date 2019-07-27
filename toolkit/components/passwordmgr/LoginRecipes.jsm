



"use strict";

this.EXPORTED_SYMBOLS = ["LoginRecipesParent"];

const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;
const REQUIRED_KEYS = ["hosts"];
const OPTIONAL_KEYS = ["description", "pathRegex"];
const SUPPORTED_KEYS = REQUIRED_KEYS.concat(OPTIONAL_KEYS);

Cu.importGlobalProperties(["URL"]);

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "LoginHelper",
                                  "resource://gre/modules/LoginHelper.jsm");

XPCOMUtils.defineLazyGetter(this, "log", () => LoginHelper.createLogger("LoginRecipes"));

function LoginRecipesParent() {
  if (Services.appinfo.processType != Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT) {
    throw new Error("LoginRecipesParent should only be used from the main process");
  }

  this._recipesByHost = new Map();

  this.initializationPromise = Promise.resolve(this);
}

LoginRecipesParent.prototype = {
  




  initializationPromise: null,

  



  _recipesByHost: null,

  




  load(aRecipes) {
    for (let rawRecipe of aRecipes.siteRecipes) {
      try {
        rawRecipe.pathRegex = rawRecipe.pathRegex ? new RegExp(rawRecipe.pathRegex) : undefined;
        this.add(rawRecipe);
      } catch (ex) {
        log.error("Error loading recipe", rawRecipe, ex);
      }
    }

    return Promise.resolve();
  },

  




  add(recipe) {
    log.debug("Adding recipe:", recipe);
    let recipeKeys = Object.keys(recipe);
    let unknownKeys = recipeKeys.filter(key => SUPPORTED_KEYS.indexOf(key) == -1);
    if (unknownKeys.length > 0) {
      throw new Error("The following recipe keys aren't supported: " + unknownKeys.join(", "));
    }

    let missingRequiredKeys = REQUIRED_KEYS.filter(key => recipeKeys.indexOf(key) == -1);
    if (missingRequiredKeys.length > 0) {
      throw new Error("The following required recipe keys are missing: " + missingRequiredKeys.join(", "));
    }

    if (!Array.isArray(recipe.hosts)) {
      throw new Error("'hosts' must be a array");
    }

    if (!recipe.hosts.length) {
      throw new Error("'hosts' must be a non-empty array");
    }

    if (recipe.pathRegex && recipe.pathRegex.constructor.name != "RegExp") {
      throw new Error("'pathRegex' must be a regular expression");
    }

    if (recipe.description && typeof(recipe.description) != "string") {
      throw new Error("'description' must be a string");
    }

    
    for (let host of recipe.hosts) {
      if (!this._recipesByHost.has(host)) {
        this._recipesByHost.set(host, new Set());
      }
      this._recipesByHost.get(host).add(recipe);
    }
  },

  





  getRecipesForHost(aHost) {
    let hostRecipes = this._recipesByHost.get(aHost);
    if (!hostRecipes) {
      return new Set();
    }

    return hostRecipes;
  },
};
