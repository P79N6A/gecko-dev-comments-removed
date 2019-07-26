




























importScripts("resource://gre/modules/devtools/escodegen/escodegen.worker.js");

self.onmessage = ({ data: { id, url, indent, ast } }) => {
  try {
    const prettified = escodegen.generate(ast, {
      format: {
        indent: {
          style: " ".repeat(indent)
        }
      },
      sourceMap: url,
      sourceMapWithCode: true
    });

    self.postMessage({
      id: id,
      code: prettified.code,
      mappings: prettified.map._mappings
    });
  } catch (e) {
    self.postMessage({
      error: e.message + "\n" + e.stack
    });
  }
};
