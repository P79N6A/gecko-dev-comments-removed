


function run_test() {
  Components.utils.import("resource:///modules/ActivitiesServiceFilter.jsm")

  do_check_true(!!ActivitiesServiceFilter);

  
  do_check_true(ActivitiesServiceFilter.match(null, null));
  do_check_true(ActivitiesServiceFilter.match({}, {}));

  
  do_check_true(ActivitiesServiceFilter.match({foobar: 42}, null));

  
  do_check_true(ActivitiesServiceFilter.match({}, {a: 'foobar', b: [1, 2, 3], c: 42}));


  
  do_check_true(ActivitiesServiceFilter.match({a: 'foobar'},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_false(ActivitiesServiceFilter.match({a: 'foobar', b: 2, c: true},
                                               {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_true(ActivitiesServiceFilter.match({a: 'foobar', b: 2, c: 42},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_false(ActivitiesServiceFilter.match({a: 'foobar2'},
                                               {a: 'foobar', b: [1, 2, 3], c: 42}));

  
  do_check_true(ActivitiesServiceFilter.match({b: 2},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_false(ActivitiesServiceFilter.match({b: 4},
                                               {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_true(ActivitiesServiceFilter.match({b: [2, 4]},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_false(ActivitiesServiceFilter.match({b: [4, 5]},
                                               {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_false(ActivitiesServiceFilter.match({a: [4, 'foobar2']},
                                               {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_true(ActivitiesServiceFilter.match({a: [4, 'foobar']},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_true(ActivitiesServiceFilter.match({a: ['foo', 'bar']},
                                              {a: 'foo'}));

  
  do_check_true(ActivitiesServiceFilter.match({k: 4},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));
  do_check_true(ActivitiesServiceFilter.match({k: [1,2,3,4]},
                                              {a: 'foobar', b: [1, 2, 3], c: 42}));

  
  do_check_false(ActivitiesServiceFilter.match({},
                                               {a: { required: true, value: 'foobar'}}));
  do_check_true(ActivitiesServiceFilter.match({a: 'foobar'},
                                              {a: { required: true, value: 'foobar'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'foobar2'},
                                               {a: { required: true, value: 'foobar'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'foobar2'},
                                               {a: { required: true, value: ['a', 'b', 'foobar']}}));
  do_check_true(ActivitiesServiceFilter.match({a: 'foobar'},
                                              {a: { required: true, value: ['a', 'b', 'foobar']}}));
  do_check_true(ActivitiesServiceFilter.match({a: ['k', 'z', 'foobar']},
                                              {a: { required: true, value: ['a', 'b', 'foobar']}}));
  do_check_false(ActivitiesServiceFilter.match({a: ['k', 'z', 'foobar2']},
                                               {a: { required: true, value: ['a', 'b', 'foobar']}}));

  
  do_check_true(ActivitiesServiceFilter.match({a: 42},
                                              {a: { required: true}}));
  do_check_false(ActivitiesServiceFilter.match({},
                                               {a: { required: true}}));

  
  do_check_true(ActivitiesServiceFilter.match({a: false},
                                              {a: { required: true, value: false}}));
  do_check_false(ActivitiesServiceFilter.match({a: true},
                                               {a: { required: true, value: false}}));
  do_check_true(ActivitiesServiceFilter.match({a: [false, true]},
                                              {a: { required: true, value: false}}));
  do_check_true(ActivitiesServiceFilter.match({a: [false, true]},
                                              {a: { required: true, value: [false,true]}}));

  
  do_check_true(ActivitiesServiceFilter.match({a: 42},
                                              {a: { required: true, value: 42}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, value: 42}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, min: 1}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, min: 2}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, min: 3}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, max: 1}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, max: 2}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, max: 3}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, min: 1, max: 1}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, min: 1, max: 2}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, min: 2, max: 2}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, value: 'foo'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, min: 100, max: 0}}));
  do_check_true(ActivitiesServiceFilter.match({a: 2},
                                              {a: { required: true, min: 'a', max: 'b'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 2},
                                               {a: { required: true, min: 10, max: 1}}));

  
  do_check_true(ActivitiesServiceFilter.match({a: 'foo'},
                                              {a: { required: true, value: 'foo'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'foo2'},
                                               {a: { required: true, value: 'foo'}}));

  
  do_check_true(ActivitiesServiceFilter.match({a: '42'},
                                              {a: { required: true, value: 42}}));
  do_check_true(ActivitiesServiceFilter.match({a: 42},
                                              {a: { required: true, value: '42'}}));
  do_check_true(ActivitiesServiceFilter.match({a: '-42e+12'},
                                              {a: { required: true, value: -42e+12}}));
  do_check_true(ActivitiesServiceFilter.match({a: 42},
                                              {a: { required: true, min: '1', max: '50'}}));
  do_check_true(ActivitiesServiceFilter.match({a: '42'},
                                              {a: 42 }));
  do_check_true(ActivitiesServiceFilter.match({a: 42},
                                              {a: '42' }));
  do_check_false(ActivitiesServiceFilter.match({a: 42},
                                               {a: { min: '44' }}));
  do_check_false(ActivitiesServiceFilter.match({a: 42},
                                               {a: { max: '0' }}));

  
  do_check_true(ActivitiesServiceFilter.match({a: 'foobar'},
                                              {a: { required: true, pattern: 'foobar'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'aafoobar'},
                                               {a: { required: true, pattern: 'foobar'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'aaFOOsdsad'},
                                               {a: { required: true, pattern: 'foo', patternFlags: 'i'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'aafoobarasdsad'},
                                               {a: { required: true, pattern: 'foo'}}));
  do_check_false(ActivitiesServiceFilter.match({a: 'aaFOOsdsad'},
                                               {a: { required: true, pattern: 'foobar'}}));
  do_check_true(ActivitiesServiceFilter.match({a: 'FoOBaR'},
                                              {a: { required: true, pattern: 'foobar', patternFlags: 'i'}}));
}
