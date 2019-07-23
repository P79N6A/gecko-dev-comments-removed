





































START("Do not crash when qn->uri is null");

var bug = 323338;
var summary = 'Do not crash when qn->uri is null';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber (bug);
printStatus (summary);

testFunc();
testFunc();

function testFunc()
{
  var htmlXML = 
   <html>
    <body>
     <div>
      <div id="summary" />
      <div id="desc" />
     </div> 
    </body>
   </html>;
  var childs = htmlXML.children();

  var el = htmlXML.body.div..div.(function::attribute('id') == 'summary');
  el.div += <div>
              <strong>Prototype:</strong>
              Test
              <br />
             </div>;
}

TEST(1, expect, actual);

END();
