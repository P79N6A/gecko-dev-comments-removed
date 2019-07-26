




























importScripts("resource://gre/modules/devtools/acorn/acorn.js");
importScripts("resource://gre/modules/devtools/source-map.js");
importScripts("resource://gre/modules/devtools/pretty-fast.js");

self.onmessage = (event) => {
  const { data: { id, url, indent, source } } = event;
  try {
    const prettified = prettyFast(source, {
      url: url,
      indent: " ".repeat(indent)
    });

    self.postMessage({
      id: id,
      code: prettified.code,
      mappings: prettified.map._mappings
    });
  } catch (e) {
    self.postMessage({
      id: id,
      error: e.message + "\n" + e.stack
    });
  }
};
