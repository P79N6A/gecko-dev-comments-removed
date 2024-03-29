

load('CharsetConversionTests.js');

const inStrings = [

  "\uFF61\uFF62\uFF63\uFF64\uFF65\uFF66\uFF67\uFF68\uFF69\uFF6A\uFF6B\uFF6C\uFF6D\uFF6E\uFF6F\uFF70\uFF71\uFF72\uFF73\uFF74\uFF75\uFF76\uFF77\uFF78\uFF79\uFF7A\uFF7B\uFF7C\uFF7D\uFF7E\uFF7F\uFF80\uFF81\uFF82\uFF83\uFF84\uFF85\uFF86\uFF87\uFF88\uFF89\uFF8A\uFF8B\uFF8C\uFF8D\uFF8E\uFF8F\uFF90\uFF91\uFF92\uFF93\uFF94\uFF95\uFF96\uFF97\uFF98\uFF99\uFF9A\uFF9B\uFF9C\uFF9D\uFF9E\uFF9F",





  "\uFF76\uFF9E\uFF77\uFF9E\uFF78\uFF9E\uFF79\uFF9E\uFF7A\uFF9E\uFF7B\uFF9E\uFF7C\uFF9E\uFF7D\uFF9E\uFF7E\uFF9E\uFF7F\uFF9E\uFF80\uFF9E\uFF81\uFF9E\uFF82\uFF9E\uFF83\uFF9E\uFF84\uFF9E\uFF8A\uFF9E\uFF8B\uFF9E\uFF8C\uFF9E\uFF8D\uFF9E\uFF8E\uFF9E",





  "\uFF8A\uFF9F\uFF8B\uFF9F\uFF8C\uFF9F\uFF8D\uFF9F\uFF8E\uFF9F",






  "\u30D5\u30E9\u30F3\u30C4\u30FB\uFF96\uFF70\uFF7E\uFF9E\uFF8C\u30FB\u30CF\u30A4\u30C9\u30F3",



  "Mozilla (\uFF93\uFF7C\uFF9E\uFF97) Foundation",



"\u0926\u093F\u0932\u094D\u0932\u0940\uFF65\uFF83\uFF9E\uFF98\uFF70\uFF65\u0A26\u0A3F\u0A71\u0A32\u0A40"
                   ];

const expectedStrings = [
                         "\x1B$B!#!V!W!\x22!&%r%!%#%%%'%)%c%e%g%C!<%\x22%$%&%(%*%+%-%/%1%3%5%7%9%;%=%?%A%D%F%H%J%K%L%M%N%O%R%U%X%[%^%_%`%a%b%d%f%h%i%j%k%l%m%o%s!+!,\x1B(B",
                         "\x1B$B%,%.%0%2%4%6%8%:%<%>%@%B%E%G%I%P%S%V%Y%\x5C\x1B(B",
                         "\x1B$B%Q%T%W%Z%]\x1B(B",
			 "\x1B$B%U%i%s%D!&%h!<%<%U!&%O%$%I%s\x1B(B",
			 "Mozilla (\x1B$B%b%8%i\x1B(B) Foundation",
			 "??????\x1B$B!&%G%j!<!&\x1B(B?????"
			 ];

function run_test()
{
    for (var i = 0; i < inStrings.length; ++i) {
        checkEncode(CreateScriptableConverter(), "ISO-2022-JP",
                    inStrings[i], expectedStrings[i]);
    }
}
