








var s = '';
for (var i = 0; i < 70000; i++) {
    s += 'function x' + i + '() { x' + i + '(); }\n';
}
s += 'trap(0); pc2line(1);\n'
evaluate(s);
