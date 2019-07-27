



const { uri } = module;
const prefix = uri.substr(0, uri.lastIndexOf("/") + 1) + "fixtures/";

exports.url = (path="") => path && path.contains(":")
  ? path 
  : prefix + path.replace(/^\.\//, "");
