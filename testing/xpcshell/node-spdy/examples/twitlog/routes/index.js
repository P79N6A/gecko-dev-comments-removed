var fs = require('fs');





var image = fs.readFileSync(__dirname + '/../public/images/nodejs.png');

exports.index = function(req, res){
  if (res.isSpdy) {
    var ua = req.headers['user-agent'];

    
    
    
    if (!ua || ua.match(/Firefox/i) === null) {
      
      res.push(
        '/images/nodejs.png',
        { 'content-type': 'image/png' },
        function(err, stream) {
          if (err) return;
          stream.on('error', function() {});
          stream.end(image);
        }
      );
    }
  }
  res.render('index', {
    title: 'SPDY - Twitlog',
    notice: req.isSpdy ?
      'Yay! This page was requested via SPDY protocol'
      :
      'Oh, no... your browser requested this page via HTTPS'
  });
};
