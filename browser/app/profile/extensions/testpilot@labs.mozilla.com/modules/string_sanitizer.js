




































EXPORTED_SYMBOLS = ["sanitizeString", "sanitizeJSONStrings"];

function sanitizeString(input) {
  
  
  return input.replace(/[^a-zA-Z0-9 .\-_]/g, '?');
}

function sanitizeJSONStrings(jsonBlob) {
  
  for (let x in jsonBlob) {
    if (typeof jsonBlob[x] == "string") {
      jsonBlob[x] = sanitizeString(jsonBlob[x]);
    } else if (typeof jsonBlob[x] == "object") {
      jsonBlob[x] = sanitizeJSONStrings(jsonBlob[x]);
    }
  }
  return jsonBlob;
}