







































var script = document.createElement('script');
script.type = 'text/javascript';
script.src = '../../SimpleTest/SimpleTest.js';

var headFC = document.getElementsByTagName('head')[0];

headFC.parentNode.insertBefore(script, headFC);
