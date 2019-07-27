


this.__proto__ = Proxy.create({has:function(){return false}});
(function(){
  eval("(function(){ for(var j=0;j<6;++j) if(j%2==1) p=0; })")();
})()
