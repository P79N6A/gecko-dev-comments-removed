




























importScripts("resource://gre/modules/devtools/shared/worker-helper.js");
importScripts("resource://gre/modules/devtools/acorn/acorn.js");
importScripts("resource://gre/modules/devtools/source-map.js");
importScripts("resource://gre/modules/devtools/pretty-fast.js");

workerHelper.createTask(self, "pretty-print", ({ url, indent, source }) => {
  try {
    const prettified = prettyFast(source, {
      url: url,
      indent: " ".repeat(indent)
    });

    return {
      code: prettified.code,
      mappings: prettified.map._mappings
    };
  }
  catch(e) {
    return new Error(e.message + "\n" + e.stack);
  }
});
