SimpleTest.waitForExplicitFinish();

var pointer_events_values = [
  'auto',
  'visiblePainted',
  'visibleFill',
  'visibleStroke',
  'visible',
  'painted',
  'fill',
  'stroke',
  'all',
  'none'
];

var paint_values = [
  'blue',
  'transparent',
  'none'
];

var opacity_values = [
  '1',
  '0.5',
  '0'
];

var visibility_values = [
  'visible',
  'hidden',
  'collapse'
];











var hit_test_inputs = {
  fill: [
    { name: 'pointer-events',  values: pointer_events_values },
    { name: 'fill',            values: paint_values },
    { name: 'fill-opacity',    values: opacity_values },
    { name: 'opacity',         values: opacity_values },
    { name: 'visibility',      values: visibility_values }
  ],
  stroke: [
    { name: 'pointer-events',  values: pointer_events_values },
    { name: 'stroke',          values: paint_values },
    { name: 'stroke-opacity',  values: opacity_values },
    { name: 'opacity',         values: opacity_values },
    { name: 'visibility',      values: visibility_values }
  ],
  both: [
    { name: 'pointer-events',  values: pointer_events_values },
    { name: 'fill',            values: paint_values },
    { name: 'fill-opacity',    values: opacity_values },
    { name: 'stroke',          values: paint_values },
    { name: 'stroke-opacity',  values: opacity_values },
    { name: 'opacity',         values: opacity_values },
    { name: 'visibility',      values: visibility_values }
  ]
}


















var hit_conditions = {
  auto: {
    'fill-intercepts-iff': {
      'visibility': ['visible'],
      'fill!': ['none']
    },
    'stroke-intercepts-iff': {
      'visibility': ['visible'],
      'stroke!': ['none']
    }
  },
  visiblePainted: {
    'fill-intercepts-iff': {
      'visibility': ['visible'],
      'fill!': ['none']
    },
    'stroke-intercepts-iff': {
      'visibility': ['visible'],
      'stroke!': ['none']
    }
  },
  visibleFill: {
    'fill-intercepts-iff': {
      visibility: ['visible']
    }
    
  },
  visibleStroke: {
    
    'stroke-intercepts-iff': {
      visibility: ['visible']
    }
  },
  visible: {
    'fill-intercepts-iff': {
      visibility: ['visible']
    },
    'stroke-intercepts-iff': {
      visibility: ['visible']
    }
  },
  painted: {
    'fill-intercepts-iff': {
      'fill!': ['none']
    },
    'stroke-intercepts-iff': {
      'stroke!': ['none']
    }
  },
  fill: {
    'fill-intercepts-iff': {
      
    }
    
  },
  stroke: {
    
    'stroke-intercepts-iff': {
      
    }
  },
  all: {
    'fill-intercepts-iff': {
      
    },
    'stroke-intercepts-iff': {
      
    }
  },
  none: {
    
  }
}


var POINT_OVER_FILL   = 0x1;
var POINT_OVER_STROKE = 0x2;






function hit_expected(element, over )
{
  function expect_hit(target){
    var intercepts_iff =
      hit_conditions[element.getAttribute('pointer-events')][target + '-intercepts-iff'];

    if (!intercepts_iff) {
      return false; 
    }

    for (var attr in intercepts_iff) {
      var vals = intercepts_iff[attr];  
      var invert = false;
      if (attr.substr(-1) == '!') {
        invert = true;
        attr = attr.substr(0, attr.length-1);
      }
      var match = vals.indexOf(element.getAttribute(attr)) > -1;
      if (invert) {
        match = !match;
      }
      if (!match) {
        return false;
      }
    }

    return true;
  }

  return (over & POINT_OVER_FILL) != 0   && expect_hit('fill') ||
         (over & POINT_OVER_STROKE) != 0 && expect_hit('stroke');
}

function for_all_permutations(inputs, callback)
{
  var current_permutation = arguments[2] || {};
  var index = arguments[3] || 0;

  if (index < inputs.length) {
    var name = inputs[index].name;
    var values = inputs[index].values;
    for (var i = 0; i < values.length; ++i) {
      current_permutation[name] = values[i];
      for_all_permutations(inputs, callback, current_permutation, index+1);
    }
    return;
  }

  callback(current_permutation);
}

function make_log_msg(over, tag, attributes)
{
  var target;
  if (over == (POINT_OVER_FILL | POINT_OVER_STROKE)) {
    target = 'fill and stroke';
  } else if (over == POINT_OVER_FILL) {
    target = 'fill';
  } else if (over == POINT_OVER_STROKE) {
    target = 'stroke';
  } else {
    throw "unexpected bit combination in 'over'";
  }
  var msg = 'Check if events are intercepted at a point over the '+target+' on <'+tag+'> for';
  for (var attr in attributes) {
    msg += ' '+attr+'='+attributes[attr];
  }
  return msg;
}

var dx, dy; 

function test_element(id, x, y, over )
{
  var element = document.getElementById(id);
  var tag = element.tagName;

  function test_permutation(attributes) {
    for (var attr in attributes) {
      element.setAttribute(attr, attributes[attr]);
    }
    var hits = document.elementFromPoint(dx + x, dy + y) == element;
    var msg = make_log_msg(over, tag, attributes);

    is(hits, hit_expected(element, over), msg);
  }

  var inputs;
  if (over == (POINT_OVER_FILL | POINT_OVER_STROKE)) {
    inputs = hit_test_inputs['both'];
  } else if (over == POINT_OVER_FILL) {
    inputs = hit_test_inputs['fill'];
  } else if (over == POINT_OVER_STROKE) {
    inputs = hit_test_inputs['stroke'];
  } else {
    throw "unexpected bit combination in 'over'";
  }

  for_all_permutations(inputs, test_permutation);

  
  element.setAttribute('fill', 'none');
  element.setAttribute('stroke', 'none');
}

function run_tests(subtest)
{
  var div = document.getElementById("div");
  dx = div.offsetLeft;
  dy = div.offsetTop;

  
  
  
  
  var partition = Math.floor(pointer_events_values.length / 2);
  switch (subtest) {
    case 0:
      pointer_events_values.splice(partition);
      break;
    case 1:
      pointer_events_values.splice(0, partition);
      break;
    case 2:
      throw "unexpected subtest number";
  }

  test_element('rect', 30, 30, POINT_OVER_FILL);
  test_element('rect', 5, 5, POINT_OVER_STROKE);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  test_element('text', 210, 30, POINT_OVER_FILL | POINT_OVER_STROKE);

  SimpleTest.finish();
}
