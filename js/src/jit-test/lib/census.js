

const Census = {};

(function () {

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  Census.walkCensus = (subject, name, walker) => walk(subject, name, walker, 0);
  function walk(subject, name, walker, count) {
    if (typeof subject === 'object') {
      print(name);
      for (let prop in subject) {
        count = walk(subject[prop],
                     name + "[" + uneval(prop) + "]",
                     walker.enter(prop),
                     count);
      }
      walker.done();
    } else {
      print(name + " = " + uneval(subject));
      walker.check(subject);
      count++;
    }

    return count;
  }

  
  Census.walkAnything = {
    enter: () => Census.walkAnything,
    done: () => undefined,
    check: () => undefined
  };

  
  Census.assertAllZeros = {
    enter: () => Census.assertAllZeros,
    done: () => undefined,
    check: elt => assertEq(elt, 0)
  };

  function expectedObject() {
    throw "Census mismatch: subject has leaf where basis has nested object";
  }

  function expectedLeaf() {
    throw "Census mismatch: subject has nested object where basis has leaf";
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  function makeBasisChecker({compare, missing, extra}) {
    return function makeWalker(basis) {
      if (typeof basis === 'object') {
        var unvisited = Set(Object.getOwnPropertyNames(basis));
        return {
          enter: prop => {
            unvisited.delete(prop);
            if (prop in basis) {
              return makeWalker(basis[prop]);
            } else {
              return extra(prop);
            }
          },

          done: () => unvisited.forEach(prop => missing(prop, basis[prop])),
          check: expectedObject
        };
      } else {
        return {
          enter: expectedLeaf,
          done: expectedLeaf,
          check: elt => compare(elt, basis)
        };
      }
    };
  }

  function missingProp() {
    throw "Census mismatch: subject lacks property present in basis";
  }

  function extraProp() {
    throw "Census mismatch: subject has property not present in basis";
  }

  
  
  Census.assertAllEqual = makeBasisChecker({
    compare: assertEq,
    missing: missingProp,
    extra: extraProp
  });

  
  
  Census.assertAllNotLessThan = makeBasisChecker({
    compare: (subject, basis) => assertEq(subject >= basis, true),
    missing: missingProp,
    extra: () => Census.walkAnything
  });

})();
