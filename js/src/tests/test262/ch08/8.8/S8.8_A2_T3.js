









function __mFunc(){var __accum=""; for (var i = 0; i < arguments.length; ++i){__accum += arguments[i]};return __accum;};


if (__mFunc("A","B","C","D","E","F") !== "ABCDEF"){
  $ERROR('#1: function __mFunc(){var __accum=""; for (var i = 0; i < arguments.length; ++i){__accum += arguments[i]};return __accum;}; __mFunc("A","B","C","D","E","F") === "ABCDEF". Actual: ' + (__mFunc("A","B","C","D","E","F")));
}



