#if 0



#endif

XPCOMUtils.defineLazyModuleGetter(this, "console",
  "resource://gre/modules/devtools/Console.jsm");

let gDebug = false;
try {
  gDebug = Services.prefs.getBoolPref(kPrefCustomizationDebug);
} catch (e) {}

function LOG(...args) {
  if (gDebug) {
    args.unshift(gModuleName);
    console.log.apply(console, args);
  }
}

function ERROR(...args) {
  args.unshift(gModuleName);
  console.error.apply(console, args);
}

function INFO(...args) {
  args.unshift(gModuleName);
  console.info.apply(console, args);
}

