




































var gTestfile = 'regress-380237-01.js';


var BUGNUMBER = 380237;
var summary = 'Generator expressions - sudoku';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

if (this.version) version(180);


Object.prototype.copy = function () {
    let o = {}
    for (let i in this)
        o[i] = this[i]
    return o
}


Array.prototype.__iterator__ = String.prototype.__iterator__ = function () {
    for (let i = 0; i < this.length; i++)
        yield this[i]
}


Array.prototype.contains = String.prototype.contains = function (e) {
    return this.indexOf(e) != -1
}

Array.prototype.repeat = String.prototype.repeat = function (n) {
    let s = this.constructor()
    for (let i = 0; i < n; i++)
        s = s.concat(this)
    return s
}

String.prototype.center = function (w) {
    let n = this.length
    if (w <= n)
        return this
    let m = Math.floor((w - n) / 2)
    return ' '.repeat(m) + this + ' '.repeat(w - n - m)
}

Array.prototype.toString = Array.prototype.toSource
Object.prototype.toString = Object.prototype.toSource



function all(seq) {
    for (let e in seq)
        if (!e)
            return false
    return true
}

function some(seq) {
    for (let e in seq)
        if (e)
            return e
    return false
}

function cross(A, B) {
    return [a+b for (a in A) for (b in B)]
}

function dict(A) {
    let d = {}
    for (let e in A)
        d[e[0]] = e[1]
    return d
}

function set(A) {
    let s = []
    for (let e in A)
        if (!s.contains(e))
            s.push(e)
    return s
}

function zip(A, B) {
    let z = []
    let n = Math.min(A.length, B.length)
    for (let i = 0; i < n; i++)
        z.push([A[i], B[i]])
    return z
}

rows = 'ABCDEFGHI'
cols = '123456789'
digits   = '123456789'
squares  = cross(rows, cols)
unitlist = [cross(rows, c) for (c in cols)]
           .concat([cross(r, cols) for (r in rows)])
           .concat([cross(rs, cs) for (rs in ['ABC','DEF','GHI']) for (cs in ['123','456','789'])])
units = dict([s, [u for (u in unitlist) if (u.contains(s))]] 
             for (s in squares))
peers = dict([s, set([s2 for (u in units[s]) for (s2 in u) if (s2 != s)])]
             for (s in squares))


function parse_grid(grid) {
    grid = [c for (c in grid) if ('0.-123456789'.contains(c))]
    let values = dict([s, digits] for (s in squares))

    for (let [s, d] in zip(squares, grid))
        if (digits.contains(d) && !assign(values, s, d))
            return false
    return values
}


function assign(values, s, d) {
    if (all(eliminate(values, s, d2) for (d2 in values[s]) if (d2 != d)))
        return values
    return false
}


function eliminate(values, s, d) {
    if (!values[s].contains(d))
        return values 
    values[s] = values[s].replace(d, '')
    if (values[s].length == 0)
	return false  
    if (values[s].length == 1) {
	
        let d2 = values[s][0]
        if (!all(eliminate(values, s2, d2) for (s2 in peers[s])))
            return false
    }
    
    for (let u in units[s]) {
	let dplaces = [s for (s in u) if (values[s].contains(d))]
	if (dplaces.length == 0)
	    return false
	if (dplaces.length == 1)
	    
            if (!assign(values, dplaces[0], d))
                return false
    }
    return values
}


function print_board(values) {
    let width = 1 + Math.max.apply(Math, [values[s].length for (s in squares)])
    let line = '\n' + ['-'.repeat(width*3)].repeat(3).join('+')
    for (let r in rows)
        print([values[r+c].center(width) + ('36'.contains(c) && '|' || '')
               for (c in cols)].join('') + ('CF'.contains(r) && line || ''))
    print('\n')
}

easy = "..3.2.6..9..3.5..1..18.64....81.29..7.......8..67.82....26.95..8..2.3..9..5.1.3.."

print_board(parse_grid(easy))


function search(values) {
    if (!values)
        return false    
    if (all(values[s].length == 1 for (s in squares))) 
        return values   

    
    
    
    let a = [values[s].length + s for (s in squares) if (values[s].length > 1)].sort()
    let s = a[0].slice(-2)

    return some(search(assign(values.copy(), s, d)) for (d in values[s]))
}

hard = '4.....8.5.3..........7......2.....6.....8.4......1.......6.3.7.5..2.....1.4......'

print_board(search(parse_grid(hard)))

  delete Object.prototype.copy;
 
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
