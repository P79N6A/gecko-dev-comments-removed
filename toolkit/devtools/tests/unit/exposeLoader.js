


exports.exerciseLazyRequire = (name, path) => {
  const o = {};
  loader.lazyRequireGetter(o, name, path);
  return o;
};
