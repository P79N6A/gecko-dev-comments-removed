


(function(){
  eval("\
    for each(let a in['A','A','A','A','A','A','A','A']) {\
      L:for each(let b in a) {\
        break L\
      }\
    }\
  ")
})()
