





function copy(obj) {
  var o = {};
  for (var i in obj)
    o[i] = obj[i];
  return o;
}

Array.prototype.contains = String.prototype.contains = function (e) {
  for (var elt of this)
    if (elt == e)
      return true;
  return false;
}

Array.prototype.repeat = String.prototype.repeat = function (n) {
  var s = this.constructor();
  for (var i = 0; i < n; i++)
    s = s.concat(this);
  return s;
}

String.prototype.center = function (w) {
  var n = this.length;
  if (w <= n)
    return this;
  var m = Math.floor((w - n) / 2);
  return ' '.repeat(m) + this + ' '.repeat(w - n - m);
}

Array.prototype.toString = Array.prototype.toSource
Object.prototype.toString = Object.prototype.toSource

function all(seq) {
  for (var e of seq)
    if (!e)
      return false;
  return true;
}

function some(seq) {
  for (var e of seq)
    if (e)
      return e;
  return false;
}

function cross(A, B) {
  return [for (a of A) for (b of B) a+b];
}

function dict(A) {
  var d = {};
  for (var e of A)
    d[e[0]] = e[1];
  return d;
}

function set(A) {
  var s = [];
  for (var e of A)
    if (!s.contains(e))
      s.push(e);
  return s;
}

function zip(A, B) {
  var z = [];
  var n = Math.min(A.length, B.length);
  for (var i = 0; i < n; i++)
    z.push([A[i], B[i]]);
  return z;
}

rows = 'ABCDEFGHI';
cols = '123456789';
digits   = '123456789';
squares  = cross(rows, cols);
unitlist = [for (c of cols) cross(rows, c)]
  .concat([for (r of rows) cross(r, cols)])
  .concat([for (rs of ['ABC','DEF','GHI']) for (cs of ['123','456','789']) cross(rs, cs)]);
units = dict((for (s of squares)
                [s, [for (u of unitlist) if (u.contains(s)) u]]));

peers = dict((for (s of squares)
                [s, set([for (u of units[s]) for (s2 of u) if (s2 != s) s2])]));


function parse_grid(grid) {
  grid = [for (c of grid) if ('0.-123456789'.contains(c)) c];
  var values = dict((for (s of squares) [s, digits]));

  for (var pair of zip(squares, grid)) {
    var s = pair[0], d = pair[1];
    if (digits.contains(d) && !assign(values, s, d))
      return false;
  }
  return values;
}


function assign(values, s, d) {
  if (all((for (d2 of values[s]) if (d2 != d) eliminate(values, s, d2))))
    return values;
  return false;
}


function eliminate(values, s, d) {
  if (!values[s].contains(d))
    return values; 
  values[s] = values[s].replace(d, '');
  if (values[s].length == 0)
    return false;  
  if (values[s].length == 1) {
    
    var d2 = values[s][0];
    if (!all((for (s2 of peers[s]) eliminate(values, s2, d2))))
      return false;
  }
  
  for (var u of units[s]) {
    var dplaces = [for (s of u) if (values[s].contains(d)) s];
    if (dplaces.length == 0)
      return false;
    if (dplaces.length == 1)
	    
      if (!assign(values, dplaces[0], d))
        return false;
  }
  return values;
}


function print_board(values) {
  var width = 1 + Math.max.apply(Math, [for (s of squares) values[s].length]);
  var line = '\n' + ['-'.repeat(width*3)].repeat(3).join('+');
  for (var r of rows)
    print([for (c of cols)
              values[r+c].center(width) + ('36'.contains(c) && '|' || '')]
          .join('') + ('CF'.contains(r) && line || ''));
  print('\n');
}

easy = "..3.2.6..9..3.5..1..18.64....81.29..7.......8..67.82....26.95..8..2.3..9..5.1.3..";

print_board(parse_grid(easy));


function search(values) {
  if (!values)
    return false;    
  if (all((for (s of squares) values[s].length == 1)))
    return values;   

  
  
  
  var a = [for (s of squares) if (values[s].length > 1) values[s].length + s].sort();
  var s = a[0].slice(-2);

  return some((for (d of values[s]) search(assign(copy(values), s, d))));
}

hard = '4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......';

print_board(search(parse_grid(hard)))

if (typeof reportCompare === "function")
  reportCompare(true, true);
