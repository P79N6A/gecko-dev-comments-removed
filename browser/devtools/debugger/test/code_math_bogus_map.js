

function stopMe(){throw Error("boom");}try{stopMe();var a=1;a=a*2;}catch(e){};

