

code = "\
j = 0;\
for (i = 0; i < 10; i++)\
{\
  j += 5;\
}\
";

eval(code);
print (j);

checkStats({
recorderStarted: 1,
recorderAborted: 0,
traceCompleted: 1,
});
