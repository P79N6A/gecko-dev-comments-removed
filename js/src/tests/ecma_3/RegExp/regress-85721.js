
















































var gTestfile = 'regress-85721.js';
var BUGNUMBER = 85721;
var summary = 'Performance: execution of regular expression';
var FAST = 100; 
var MSG_FAST = 'Execution took less than ' + FAST + ' ms';
var MSG_SLOW = 'Execution took ';
var MSG_MS = ' ms';
var str = '';
var re = '';
var status = '';
var actual = '';
var expect= '';

printBugNumber(BUGNUMBER);
printStatus (summary);


function elapsedTime(startTime)
{
  return new Date() - startTime;
}


function isThisFast(ms)
{
  if (ms <= FAST)
    return MSG_FAST;
  return MSG_SLOW + ms + MSG_MS;
}






str='<sql:connection id="conn1"> <sql:url>www.m.com</sql:url> <sql:driver>drive.class</sql:driver>\n<sql:userId>foo</sql:userId> <sql:password>goo</sql:password> </sql:connection>';
re = /<sql:connection id="([^\r\n]*?)">\s*<sql:url>\s*([^\r\n]*?)\s*<\/sql:url>\s*<sql:driver>\s*([^\r\n]*?)\s*<\/sql:driver>\s*(\s*<sql:userId>\s*([^\r\n]*?)\s*<\/sql:userId>\s*)?\s*(\s*<sql:password>\s*([^\r\n]*?)\s*<\/sql:password>\s*)?\s*<\/sql:connection>/;
expect = Array("<sql:connection id=\"conn1\"> <sql:url>www.m.com</sql:url> <sql:driver>drive.class</sql:driver>\n<sql:userId>foo</sql:userId> <sql:password>goo</sql:password> </sql:connection>","conn1","www.m.com","drive.class","<sql:userId>foo</sql:userId> ","foo","<sql:password>goo</sql:password> ","goo");




status = inSection(1);
var start = new Date();
var result = re.exec(str);
actual = elapsedTime(start);
reportCompare(isThisFast(FAST), isThisFast(actual), status);




status = inSection(2);
testRegExp([status], [re], [str], [result], [expect]);









$esc        = '\\\\';     
$Period      = '\.';
$space      = '\040';              $tab         = '\t';
$OpenBR     = '\\[';               $CloseBR     = '\\]';
$OpenParen  = '\\(';               $CloseParen  = '\\)';
$NonASCII   = '\x80-\xff';         $ctrl        = '\000-\037';
$CRlist     = '\n\015';  

$qtext = '[^' + $esc + $NonASCII + $CRlist + '\"]';						  
$dtext = '[^' + $esc + $NonASCII + $CRlist + $OpenBR + $CloseBR + ']';    
$quoted_pair = $esc + '[^' + $NonASCII + ']';							  




$ctext   =  '[^' + $esc + $NonASCII + $CRlist + '()]';



$Cnested =
  $OpenParen +                                 
  $ctext + '*' +                            
  '(?:' + $quoted_pair + $ctext + '*)*' +   
  $CloseParen;                                 




$comment =
  $OpenParen +                                           
  $ctext + '*' +                                     
  '(?:' +                                            
  '(?:' + $quoted_pair + '|' + $Cnested + ')' +   
  $ctext + '*' +                                 
  ')*' +                                             
  $CloseParen;                                           




$X =
  '[' + $space + $tab + ']*' +					       
  '(?:' + $comment + '[' + $space + $tab + ']*)*';    



$atom_char   = '[^(' + $space + '<>\@,;:\".' + $esc + $OpenBR + $CloseBR + $ctrl + $NonASCII + ']';
$atom =
  $atom_char + '+' +            
  '(?!' + $atom_char + ')';     


$quoted_str =
  '\"' +                                         
  $qtext + '*' +                              
  '(?:' + $quoted_pair + $qtext + '*)*' +     
  '\"';                                          


$word =
  '(?:' +
  $atom +                
  '|' +                  
  $quoted_str +          
  ')'


  $domain_ref  = $atom;


$domain_lit  =
  $OpenBR +								   	     
  '(?:' + $dtext + '|' + $quoted_pair + ')*' +     
  $CloseBR;                                        


$sub_domain  =
  '(?:' +
  $domain_ref +
  '|' +
  $domain_lit +
  ')' +
  $X;                 


$domain =
  $sub_domain +
  '(?:' +
  $Period + $X + $sub_domain +
  ')*';


$route =
  '\@' + $X + $domain +
  '(?:,' + $X + '\@' + $X + $domain + ')*' +  
  ':' +
  $X;					


$local_part =
  $word + $X
  '(?:' +
  $Period + $X + $word + $X +		
  ')*';


$addr_spec  =
  $local_part + '\@' + $X + $domain;


$route_addr =
  '<' + $X +                     
  '(?:' + $route + ')?' +     
  $addr_spec +                
  '>';                           


$phrase_ctrl = '\000-\010\012-\037'; 



$phrase_char =
  '[^()<>\@,;:\".' + $esc + $OpenBR + $CloseBR + $NonASCII + $phrase_ctrl + ']';



$phrase =
  $word +                                                  
  $phrase_char + '*' +                                     
  '(?:' +
  '(?:' + $comment + '|' + $quoted_str + ')' +          
  $phrase_char + '*' +                                  
  ')*';


$mailbox =
  $X +                                
  '(?:' +
  $phrase + $route_addr +     
  '|' +                       
  $addr_spec +                
  ')';





re = new RegExp($mailbox, "g");
str = 'Jeffy<"That Tall Guy"@ora.com (this address is no longer active)>';
expect = Array('Jeffy<"That Tall Guy"@ora.com (this address is no longer active)>');




status = inSection(3);
var start = new Date();
var result = re.exec(str);
actual = elapsedTime(start);
reportCompare(isThisFast(FAST), isThisFast(actual), status);




status = inSection(4);
testRegExp([status], [re], [str], [result], [expect]);
