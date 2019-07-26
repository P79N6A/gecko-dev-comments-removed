




#include "nsIndexedToHTML.h"
#include "mozilla/dom/EncodingUtils.h"
#include "nsNetUtil.h"
#include "netCore.h"
#include "nsStringStream.h"
#include "nsIFileURL.h"
#include "nsEscape.h"
#include "nsIDirIndex.h"
#include "nsDateTimeFormatCID.h"
#include "nsURLHelper.h"
#include "nsIPlatformCharset.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsIPrefLocalizedString.h"
#include "nsIChromeRegistry.h"
#include "nsICharsetConverterManager.h"
#include "nsIDateTimeFormat.h"
#include "nsIStringBundle.h"
#include "nsITextToSubURI.h"
#include "nsXPIDLString.h"
#include <algorithm>

NS_IMPL_ISUPPORTS4(nsIndexedToHTML,
                   nsIDirIndexListener,
                   nsIStreamConverter,
                   nsIRequestObserver,
                   nsIStreamListener)

static void AppendNonAsciiToNCR(const nsAString& in, nsAFlatString& out)
{
  nsAString::const_iterator start, end;

  in.BeginReading(start);
  in.EndReading(end);

  while (start != end) {
    if (*start < 128) {
      out.Append(*start++);
    } else {
      out.AppendLiteral("&#x");
      nsAutoString hex;
      hex.AppendInt(*start++, 16);
      out.Append(hex);
      out.Append((PRUnichar)';');
    }
  }
}

nsresult
nsIndexedToHTML::Create(nsISupports *aOuter, REFNSIID aIID, void **aResult) {
    nsresult rv;
    if (aOuter)
        return NS_ERROR_NO_AGGREGATION;
    
    nsIndexedToHTML* _s = new nsIndexedToHTML();
    if (_s == nullptr)
        return NS_ERROR_OUT_OF_MEMORY;
    
    rv = _s->QueryInterface(aIID, aResult);
    return rv;
}

nsresult
nsIndexedToHTML::Init(nsIStreamListener* aListener) {

    nsXPIDLString ellipsis;
    nsCOMPtr<nsIPrefBranch> prefs(do_GetService(NS_PREFSERVICE_CONTRACTID));
    if (prefs) {
      nsCOMPtr<nsIPrefLocalizedString> prefVal;
      prefs->GetComplexValue("intl.ellipsis",
                           NS_GET_IID(nsIPrefLocalizedString),
                           getter_AddRefs(prefVal));
      if (prefVal)
        prefVal->ToString(getter_Copies(ellipsis));
    }
    if (ellipsis.IsEmpty())
      mEscapedEllipsis.AppendLiteral("&#8230;");
    else
      mEscapedEllipsis.Adopt(nsEscapeHTML2(ellipsis.get(), ellipsis.Length()));

    nsresult rv = NS_OK;

    mListener = aListener;

    mDateTime = do_CreateInstance(NS_DATETIMEFORMAT_CONTRACTID, &rv);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIStringBundleService> sbs =
        do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    if (NS_FAILED(rv)) return rv;
    rv = sbs->CreateBundle(NECKO_MSGS_URL, getter_AddRefs(mBundle));

    mExpectAbsLoc = false;

    return rv;
}

NS_IMETHODIMP
nsIndexedToHTML::Convert(nsIInputStream* aFromStream,
                         const char* aFromType,
                         const char* aToType,
                         nsISupports* aCtxt,
                         nsIInputStream** res) {
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsIndexedToHTML::AsyncConvertData(const char *aFromType,
                                  const char *aToType,
                                  nsIStreamListener *aListener,
                                  nsISupports *aCtxt) {
    return Init(aListener);
}

NS_IMETHODIMP
nsIndexedToHTML::OnStartRequest(nsIRequest* request, nsISupports *aContext) {
    nsString buffer;
    nsresult rv = DoOnStartRequest(request, aContext, buffer);
    if (NS_FAILED(rv)) {
        request->Cancel(rv);
    }
    
    rv = mListener->OnStartRequest(request, aContext);
    if (NS_FAILED(rv)) return rv;

    
    
    request->GetStatus(&rv);
    if (NS_FAILED(rv)) return rv;

    

    rv = FormatInputStream(request, aContext, buffer);
    return rv;
}

nsresult
nsIndexedToHTML::DoOnStartRequest(nsIRequest* request, nsISupports *aContext,
                                  nsString& aBuffer) {
    nsresult rv;

    nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
    nsCOMPtr<nsIURI> uri;
    rv = channel->GetURI(getter_AddRefs(uri));
    if (NS_FAILED(rv)) return rv;

    channel->SetContentType(NS_LITERAL_CSTRING("text/html"));

    mParser = do_CreateInstance("@mozilla.org/dirIndexParser;1",&rv);
    if (NS_FAILED(rv)) return rv;

    rv = mParser->SetListener(this);
    if (NS_FAILED(rv)) return rv;
    
    rv = mParser->OnStartRequest(request, aContext);
    if (NS_FAILED(rv)) return rv;

    nsAutoCString baseUri, titleUri;
    rv = uri->GetAsciiSpec(baseUri);
    if (NS_FAILED(rv)) return rv;
    titleUri = baseUri;

    nsCString parentStr;

    
    
    
    
    
    
    

    bool isScheme = false;
    bool isSchemeFile = false;
    if (NS_SUCCEEDED(uri->SchemeIs("ftp", &isScheme)) && isScheme) {

        
        
        
        
        nsAutoCString pw;
        rv = uri->GetPassword(pw);
        if (NS_FAILED(rv)) return rv;
        if (!pw.IsEmpty()) {
             nsCOMPtr<nsIURI> newUri;
             rv = uri->Clone(getter_AddRefs(newUri));
             if (NS_FAILED(rv)) return rv;
             rv = newUri->SetPassword(EmptyCString());
             if (NS_FAILED(rv)) return rv;
             rv = newUri->GetAsciiSpec(titleUri);
             if (NS_FAILED(rv)) return rv;
        }

        nsAutoCString path;
        rv = uri->GetPath(path);
        if (NS_FAILED(rv)) return rv;

        if (!path.EqualsLiteral("//") && !path.LowerCaseEqualsLiteral("/%2f")) {
            rv = uri->Resolve(NS_LITERAL_CSTRING(".."),parentStr);
            if (NS_FAILED(rv)) return rv;
        }
    } else if (NS_SUCCEEDED(uri->SchemeIs("file", &isSchemeFile)) && isSchemeFile) {
        nsCOMPtr<nsIFileURL> fileUrl = do_QueryInterface(uri);
        nsCOMPtr<nsIFile> file;
        rv = fileUrl->GetFile(getter_AddRefs(file));
        if (NS_FAILED(rv)) return rv;
        file->SetFollowLinks(true);
        
        nsAutoCString url;
        rv = net_GetURLSpecFromFile(file, url);
        if (NS_FAILED(rv)) return rv;
        baseUri.Assign(url);
        
        nsCOMPtr<nsIFile> parent;
        rv = file->GetParent(getter_AddRefs(parent));
        
        if (parent && NS_SUCCEEDED(rv)) {
            net_GetURLSpecFromDir(parent, url);
            if (NS_FAILED(rv)) return rv;
            parentStr.Assign(url);
        }

        
        rv = mParser->SetEncoding("UTF-8");
        NS_ENSURE_SUCCESS(rv, rv);

    } else if (NS_SUCCEEDED(uri->SchemeIs("jar", &isScheme)) && isScheme) {
        nsAutoCString path;
        rv = uri->GetPath(path);
        if (NS_FAILED(rv)) return rv;

        
        
        
        
        
        
        if (!StringEndsWith(path, NS_LITERAL_CSTRING("!/"))) {
            rv = uri->Resolve(NS_LITERAL_CSTRING(".."), parentStr);
            if (NS_FAILED(rv)) return rv;
        }
    }
    else {
        
        
        nsAutoCString path;
        rv = uri->GetPath(path);
        if (NS_FAILED(rv)) return rv;
        if (baseUri.Last() != '/') {
            baseUri.Append('/');
            path.Append('/');
            uri->SetPath(path);
        }
        if (!path.EqualsLiteral("/")) {
            rv = uri->Resolve(NS_LITERAL_CSTRING(".."), parentStr);
            if (NS_FAILED(rv)) return rv;
        }
    }

    nsString buffer;
    buffer.AppendLiteral("<!DOCTYPE html>\n"
                         "<html>\n<head>\n"
                         "<meta http-equiv=\"content-type\" content=\"text/html; charset=");
    
    
    
    
    

    nsXPIDLCString encoding;
    rv = mParser->GetEncoding(getter_Copies(encoding));
    if (NS_FAILED(rv)) return rv;

    AppendASCIItoUTF16(encoding, buffer);
    buffer.AppendLiteral("\">\n"
                         "<style type=\"text/css\">\n"
                         ":root {\n"
                         "  font-family: sans-serif;\n"
                         "}\n"
                         "img {\n"
                         "  border: 0;\n"
                         "}\n"
                         "th {\n"
                         "  text-align: start;\n"
                         "  white-space: nowrap;\n"
                         "}\n"
                         "th > a {\n"
                         "  color: inherit;\n"
                         "}\n"
                         "table[order] > thead > tr > th {\n"
                         "  cursor: pointer;\n"
                         "}\n"
                         "table[order] > thead > tr > th::after {\n"
                         "  display: none;\n"
                         "  width: .8em;\n"
                         "  -moz-margin-end: -.8em;\n"
                         "  text-align: end;\n"
                         "}\n"
                         "table[order=\"asc\"] > thead > tr > th::after {\n"
                         "  content: \"\\2193\"; /* DOWNWARDS ARROW (U+2193) */\n"
                         "}\n"
                         "table[order=\"desc\"] > thead > tr > th::after {\n"
                         "  content: \"\\2191\"; /* UPWARDS ARROW (U+2191) */\n"
                         "}\n"
                         "table[order][order-by=\"0\"] > thead > tr > th:first-child > a ,\n"
                         "table[order][order-by=\"1\"] > thead > tr > th:first-child + th > a ,\n"
                         "table[order][order-by=\"2\"] > thead > tr > th:first-child + th + th > a {\n"
                         "  text-decoration: underline;\n"
                         "}\n"
                         "table[order][order-by=\"0\"] > thead > tr > th:first-child::after ,\n"
                         "table[order][order-by=\"1\"] > thead > tr > th:first-child + th::after ,\n"
                         "table[order][order-by=\"2\"] > thead > tr > th:first-child + th + th::after {\n"
                         "  display: inline-block;\n"
                         "}\n"
                         "table.remove-hidden > tbody > tr.hidden-object {\n"
                         "  display: none;\n"
                         "}\n"
                         "td > a {\n"
                         "  display: inline-block;\n"
                         "}\n"
                         "/* name */\n"
                         "th:first-child {\n"
                         "  -moz-padding-end: 2em;\n"
                         "}\n"
                         "/* size */\n"
                         "th:first-child + th {\n"
                         "  -moz-padding-end: 1em;\n"
                         "}\n"
                         "td:first-child + td {\n"
                         "  text-align: end;\n"
                         "  -moz-padding-end: 1em;\n"
                         "  white-space: nowrap;\n"
                         "}\n"
                         "/* date */\n"
                         "td:first-child + td + td {\n"
                         "  -moz-padding-start: 1em;\n"
                         "  -moz-padding-end: .5em;\n"
                         "  white-space: nowrap;\n"
                         "}\n"
                         "/* time */\n"
                         "td:last-child {\n"
                         "  -moz-padding-start: .5em;\n"
                         "  white-space: nowrap;\n"
                         "}\n"
                         ".symlink {\n"
                         "  font-style: italic;\n"
                         "}\n"
                         ".dir ,\n"
                         ".symlink ,\n"
                         ".file {\n"
                         "  -moz-margin-start: 20px;\n"
                         "}\n"
                         ".dir::before ,\n"
                         ".file > img {\n"
                         "  -moz-margin-end: 4px;\n"
                         "  -moz-margin-start: -20px;\n"
                         "  max-width: 16px;\n"
                         "  max-height: 16px;\n"
                         "  vertical-align: middle;\n"
                         "}\n"
                         ".dir::before {\n"
                         "  content: url(resource://gre/res/html/folder.png);\n"
                         "}\n"
                         "</style>\n"
                         "<link rel=\"stylesheet\" media=\"screen, projection\" type=\"text/css\""
                         " href=\"chrome://global/skin/dirListing/dirListing.css\">\n"
                         "<script type=\"application/javascript\">\n"
                         "var gTable, gOrderBy, gTBody, gRows, gUI_showHidden;\n"
                         "document.addEventListener(\"DOMContentLoaded\", function() {\n"
                         "  gTable = document.getElementsByTagName(\"table\")[0];\n"
                         "  gTBody = gTable.tBodies[0];\n"
                         "  if (gTBody.rows.length < 2)\n"
                         "    return;\n"
                         "  gUI_showHidden = document.getElementById(\"UI_showHidden\");\n"
                         "  var headCells = gTable.tHead.rows[0].cells,\n"
                         "      hiddenObjects = false;\n"
                         "  function rowAction(i) {\n"
                         "    return function(event) {\n"
                         "      event.preventDefault();\n"
                         "      orderBy(i);\n"
                         "    }\n"
                         "  }\n"
                         "  for (var i = headCells.length - 1; i >= 0; i--) {\n"
                         "    var anchor = document.createElement(\"a\");\n"
                         "    anchor.href = \"\";\n"
                         "    anchor.appendChild(headCells[i].firstChild);\n"
                         "    headCells[i].appendChild(anchor);\n"
                         "    headCells[i].addEventListener(\"click\", rowAction(i), true);\n"
                         "  }\n"
                         "  if (gUI_showHidden) {\n"
                         "    gRows = Array.slice(gTBody.rows);\n"
                         "    hiddenObjects = gRows.some(function (row) row.className == \"hidden-object\");\n"
                         "  }\n"
                         "  gTable.setAttribute(\"order\", \"\");\n"
                         "  if (hiddenObjects) {\n"
                         "    gUI_showHidden.style.display = \"block\";\n"
                         "    updateHidden();\n"
                         "  }\n"
                         "}, \"false\");\n"
                         "function compareRows(rowA, rowB) {\n"
                         "  var a = rowA.cells[gOrderBy].getAttribute(\"sortable-data\") || \"\";\n"
                         "  var b = rowB.cells[gOrderBy].getAttribute(\"sortable-data\") || \"\";\n"
                         "  var intA = +a;\n"
                         "  var intB = +b;\n"
                         "  if (a == intA && b == intB) {\n"
                         "    a = intA;\n"
                         "    b = intB;\n"
                         "  } else {\n"
                         "    a = a.toLowerCase();\n"
                         "    b = b.toLowerCase();\n"
                         "  }\n"
                         "  if (a < b)\n"
                         "    return -1;\n"
                         "  if (a > b)\n"
                         "    return 1;\n"
                         "  return 0;\n"
                         "}\n"
                         "function orderBy(column) {\n"
                         "  if (!gRows)\n"
                         "    gRows = Array.slice(gTBody.rows);\n"
                         "  var order;\n"
                         "  if (gOrderBy == column) {\n"
                         "    order = gTable.getAttribute(\"order\") == \"asc\" ? \"desc\" : \"asc\";\n"
                         "  } else {\n"
                         "    order = \"asc\";\n"
                         "    gOrderBy = column;\n"
                         "    gTable.setAttribute(\"order-by\", column);\n"
                         "    gRows.sort(compareRows);\n"
                         "  }\n"
                         "  gTable.removeChild(gTBody);\n"
                         "  gTable.setAttribute(\"order\", order);\n"
                         "  if (order == \"asc\")\n"
                         "    for (var i = 0; i < gRows.length; i++)\n"
                         "      gTBody.appendChild(gRows[i]);\n"
                         "  else\n"
                         "    for (var i = gRows.length - 1; i >= 0; i--)\n"
                         "      gTBody.appendChild(gRows[i]);\n"
                         "  gTable.appendChild(gTBody);\n"
                         "}\n"
                         "function updateHidden() {\n"
                         "  gTable.className = gUI_showHidden.getElementsByTagName(\"input\")[0].checked ?\n"
                         "                     \"\" :\n"
                         "                     \"remove-hidden\";\n"
                         "}\n"
                         "</script>\n");

    buffer.AppendLiteral("<link rel=\"icon\" type=\"image/png\" href=\"");
    nsCOMPtr<nsIURI> innerUri = NS_GetInnermostURI(uri);
    if (!innerUri)
        return NS_ERROR_UNEXPECTED;
    nsCOMPtr<nsIFileURL> fileURL(do_QueryInterface(innerUri));
    
    if (fileURL) {
        
        buffer.AppendLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB"
                             "AAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9i"
                             "ZSBJbWFnZVJlYWR5ccllPAAAAjFJREFUeNqsU8uOElEQPffR"
                             "3XQ3ONASdBJCSBxHos5%2B3Bg3rvkCv8PElS78gPkO%2FATj"
                             "QoUdO2ftrJiRh6aneTb9sOpC4weMN6lcuFV16pxDIfI8x12O"
                             "YIDhcPiu2Wx%2B%2FHF5CW1Z6Jyegt%2FTNEWSJIjjGFEUIQ"
                             "xDrFYrWFSzXC4%2FdLvd95pRKpXKy%2BpRFZ7nwaWo1%2BsG"
                             "nQG2260BKJfLKJVKGI1GEEJw7ateryd0v993W63WEwjgxfn5"
                             "obGYzgCbzcaEbdsIggDj8Riu6z6iUk9SYZMSx8W0LMsM%2FS"
                             "KK75xnJlIq80anQXdbEp0OhcPJ0eiaJnGRMEyyPDsAKKUM9c"
                             "lkYoDo3SZJzzSdp0VSKYmfV1co%2Bz580kw5KDIM8RbRfEnU"
                             "f1HzxtQyMAGcaGruTKczMzEIaqhKifV6jd%2BzGQQB5llunF"
                             "%2FM52BizC2K5sYPYvZcu653tjOM9O93wnYc08gmkgg4VAxi"
                             "xfqFUJT36AYBZGd6PJkFCZnnlBxMp38gqIgLpZB0y4Nph18l"
                             "yWh5FFbrOSxbl3V4G%2BVB7T4ajYYxTyuLtO%2BCvWGgJE1M"
                             "c7JNsJEhvgw%2FQV4fo%2F24nbEsX2u1d5sVyn8sJO0ZAQiI"
                             "YnFh%2BxrfLz%2Fj29cBS%2FO14zg3i8XigW3ZkErDtmKoeM"
                             "%2BAJGRMnXeEPGKf0nCD1ydvkDzU9Jbc6OpR7WIw6L8lQ%2B"
                             "4pQ1%2FlPF0RGM9Ns91Wmptk0GfB4EJkt77vXYj%2F8m%2B8"
                             "y%2FkrwABHbz2H9V68DQAAAABJRU5ErkJggg%3D%3D");
    } else {
        
        buffer.AppendLiteral("data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAB"
                             "AAAAAQCAYAAAAf8%2F9hAAAAGXRFWHRTb2Z0d2FyZQBBZG9i"
                             "ZSBJbWFnZVJlYWR5ccllPAAAAeBJREFUeNqcU81O20AQ%2Ft"
                             "Z2AgQSYQRqL1UPVG2hAUQkxLEStz4DrXpLpD5Drz31Cajax%"
                             "2Bghhx6qHIJURBTxIwQRwopCBbZjHMcOTrzermPipsSt1Iw0"
                             "3p3ZmW%2B%2B2R0TxhgOD34wjCHZlQ0iDYz9yvEfhxMTCYhE"
                             "QDIZhkxKd2sqzX2TOD2vBQCQhpPefng1ZP2dVPlLLdpL8SEM"
                             "cxng%2Fbs0RIHhtgs4twxOh%2BHjZxvzDx%2F3GQQiDFISiR"
                             "BLFMPKTRMollzcWECrDVhtxtdRVsL9youPxGj%2FbdfFlUZh"
                             "tDyYbYqWRUdai1oQRZ5oHeHl2gNM%2B01Uqio8RlH%2Bnsaz"
                             "JzNwXcq1B%2BiXPHprlEEymeBfXs1w8XxxihfyuXqoHqpoGj"
                             "ZM04bddgG%2F9%2B8WGj87qDdsrK9m%2BoA%2BpbhQTDh2l1"
                             "%2Bi2weNbSHMZyjvNXmVbqh9Fj5Oz27uEoP%2BSTxANruJs9"
                             "L%2FT6P0ewqPx5nmiAG5f6AoCtN1PbJzuRyJAyDBzzSQYvEr"
                             "f06yYxhGXlEa8H2KVGoasjwLx3Ewk858opQWXm%2B%2Fib9E"
                             "QrBzclLLLy89xYvlpchvtixcX6uo1y%2FzsiwHrkIsgKbp%2"
                             "BYWFOWicuqppoNTnStHzPFCPQhBEBOyGAX4JMADFetubi4BS"
                             "YAAAAABJRU5ErkJggg%3D%3D");
    }
    buffer.AppendLiteral("\">\n<title>");

    
    

    if (!mTextToSubURI) {
        mTextToSubURI = do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    nsXPIDLString unEscapeSpec;
    rv = mTextToSubURI->UnEscapeAndConvert(encoding, titleUri.get(),
                                           getter_Copies(unEscapeSpec));
    
    
    
    
    if (NS_FAILED(rv) && isSchemeFile) {
        nsCOMPtr<nsIPlatformCharset> platformCharset(do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv));
        NS_ENSURE_SUCCESS(rv, rv);
        nsAutoCString charset;
        rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, charset);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = mTextToSubURI->UnEscapeAndConvert(charset.get(), titleUri.get(),
                                               getter_Copies(unEscapeSpec));
    }
    if (NS_FAILED(rv)) return rv;

    nsXPIDLString htmlEscSpec;
    htmlEscSpec.Adopt(nsEscapeHTML2(unEscapeSpec.get(),
                                    unEscapeSpec.Length()));

    nsXPIDLString title;
    const PRUnichar* formatTitle[] = {
        htmlEscSpec.get()
    };

    rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("DirTitle").get(),
                                       formatTitle,
                                       sizeof(formatTitle)/sizeof(PRUnichar*),
                                       getter_Copies(title));
    if (NS_FAILED(rv)) return rv;

    
    
    AppendNonAsciiToNCR(title, buffer);

    buffer.AppendLiteral("</title>\n");    

    
    
    
    
    
    
    
    

    if (baseUri.FindChar('"') == kNotFound)
    {
        
        
        
        buffer.AppendLiteral("<base href=\"");
        NS_ConvertUTF8toUTF16 utf16BaseURI(baseUri);
        nsString htmlEscapedUri;
        htmlEscapedUri.Adopt(nsEscapeHTML2(utf16BaseURI.get(), utf16BaseURI.Length()));
        buffer.Append(htmlEscapedUri);
        buffer.AppendLiteral("\" />\n");
    }
    else
    {
        NS_ERROR("broken protocol handler didn't escape double-quote.");
    }

    nsAutoString direction(NS_LITERAL_STRING("ltr"));
    nsCOMPtr<nsIXULChromeRegistry> reg =
      mozilla::services::GetXULChromeRegistryService();
    if (reg) {
      bool isRTL = false;
      reg->IsLocaleRTL(NS_LITERAL_CSTRING("global"), &isRTL);
      if (isRTL) {
        direction.AssignLiteral("rtl");
      }
    }

    buffer.AppendLiteral("</head>\n<body dir=\"");
    buffer.Append(direction);
    buffer.AppendLiteral("\">\n<h1>");
    
    const PRUnichar* formatHeading[] = {
        htmlEscSpec.get()
    };

    rv = mBundle->FormatStringFromName(NS_LITERAL_STRING("DirTitle").get(),
                                       formatHeading,
                                       sizeof(formatHeading)/sizeof(PRUnichar*),
                                       getter_Copies(title));
    if (NS_FAILED(rv)) return rv;
    
    AppendNonAsciiToNCR(title, buffer);
    buffer.AppendLiteral("</h1>\n");

    if (!parentStr.IsEmpty()) {
        nsXPIDLString parentText;
        rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DirGoUp").get(),
                                        getter_Copies(parentText));
        if (NS_FAILED(rv)) return rv;

        buffer.AppendLiteral("<p id=\"UI_goUp\"><a class=\"up\" href=\"");

        NS_ConvertUTF8toUTF16 utf16ParentStr(parentStr);
        nsString htmlParentStr;
        htmlParentStr.Adopt(nsEscapeHTML2(utf16ParentStr.get(), utf16ParentStr.Length()));
        buffer.Append(htmlParentStr);
        buffer.AppendLiteral("\">");
        AppendNonAsciiToNCR(parentText, buffer);
        buffer.AppendLiteral("</a></p>\n");
    }

    if (isSchemeFile) {
        nsXPIDLString showHiddenText;
        rv = mBundle->GetStringFromName(NS_LITERAL_STRING("ShowHidden").get(),
                                        getter_Copies(showHiddenText));
        if (NS_FAILED(rv)) return rv;

        buffer.AppendLiteral("<p id=\"UI_showHidden\" style=\"display:none\"><label><input type=\"checkbox\" checked onchange=\"updateHidden()\">");
        AppendNonAsciiToNCR(showHiddenText, buffer);
        buffer.AppendLiteral("</label></p>\n");
    }

    buffer.AppendLiteral("<table>\n");

    nsXPIDLString columnText;

    buffer.AppendLiteral(" <thead>\n"
                         "  <tr>\n"
                         "   <th>");

    rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DirColName").get(),
                                    getter_Copies(columnText));
    if (NS_FAILED(rv)) return rv;
    AppendNonAsciiToNCR(columnText, buffer);
    buffer.AppendLiteral("</th>\n"
                         "   <th>");

    rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DirColSize").get(),
                                    getter_Copies(columnText));
    if (NS_FAILED(rv)) return rv;
    AppendNonAsciiToNCR(columnText, buffer);
    buffer.AppendLiteral("</th>\n"
                         "   <th colspan=\"2\">");

    rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DirColMTime").get(),
                                    getter_Copies(columnText));
    if (NS_FAILED(rv)) return rv;
    AppendNonAsciiToNCR(columnText, buffer);
    buffer.AppendLiteral("</th>\n"
                         "  </tr>\n"
                         " </thead>\n");
    buffer.AppendLiteral(" <tbody>\n");

    aBuffer = buffer;
    return rv;
}

NS_IMETHODIMP
nsIndexedToHTML::OnStopRequest(nsIRequest* request, nsISupports *aContext,
                               nsresult aStatus) {
    if (NS_SUCCEEDED(aStatus)) {
        nsString buffer;
        buffer.AssignLiteral("</tbody></table></body></html>\n");

        aStatus = FormatInputStream(request, aContext, buffer);
    }

    mParser->OnStopRequest(request, aContext, aStatus);
    mParser = 0;
    
    return mListener->OnStopRequest(request, aContext, aStatus);
}

nsresult
nsIndexedToHTML::FormatInputStream(nsIRequest* aRequest, nsISupports *aContext, const nsAString &aBuffer) 
{
    nsresult rv = NS_OK;

    
    if (!mUnicodeEncoder) {
      nsXPIDLCString encoding;
      rv = mParser->GetEncoding(getter_Copies(encoding));
      if (NS_SUCCEEDED(rv)) {
        nsCOMPtr<nsICharsetConverterManager> charsetConverterManager;
        charsetConverterManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);
        rv = charsetConverterManager->GetUnicodeEncoder(encoding.get(), 
                                                          getter_AddRefs(mUnicodeEncoder));
        if (NS_SUCCEEDED(rv))
            rv = mUnicodeEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, 
                                                       nullptr, (PRUnichar)'?');
      }
    }

    
    char *buffer = nullptr;
    int32_t dstLength;
    if (NS_SUCCEEDED(rv)) {
      int32_t unicharLength = aBuffer.Length();
      rv = mUnicodeEncoder->GetMaxLength(PromiseFlatString(aBuffer).get(), 
                                         unicharLength, &dstLength);
      if (NS_SUCCEEDED(rv)) {
        buffer = (char *) nsMemory::Alloc(dstLength);
        NS_ENSURE_TRUE(buffer, NS_ERROR_OUT_OF_MEMORY);

        rv = mUnicodeEncoder->Convert(PromiseFlatString(aBuffer).get(), &unicharLength, 
                                      buffer, &dstLength);
        if (NS_SUCCEEDED(rv)) {
          int32_t finLen = 0;
          rv = mUnicodeEncoder->Finish(buffer + dstLength, &finLen);
          if (NS_SUCCEEDED(rv))
            dstLength += finLen;
        }
      }
    }

    
    if (NS_FAILED(rv)) {
      rv = NS_OK;
      if (buffer) {
        nsMemory::Free(buffer);
        buffer = nullptr;
      }
    }

    nsCOMPtr<nsIInputStream> inputData;
    if (buffer) {
      rv = NS_NewCStringInputStream(getter_AddRefs(inputData), Substring(buffer, dstLength));
      nsMemory::Free(buffer);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mListener->OnDataAvailable(aRequest, aContext,
                                      inputData, 0, dstLength);
    }
    else {
      NS_ConvertUTF16toUTF8 utf8Buffer(aBuffer);
      rv = NS_NewCStringInputStream(getter_AddRefs(inputData), utf8Buffer);
      NS_ENSURE_SUCCESS(rv, rv);
      rv = mListener->OnDataAvailable(aRequest, aContext,
                                      inputData, 0, utf8Buffer.Length());
    }
    return (rv);
}

NS_IMETHODIMP
nsIndexedToHTML::OnDataAvailable(nsIRequest *aRequest,
                                 nsISupports *aCtxt,
                                 nsIInputStream* aInput,
                                 uint64_t aOffset,
                                 uint32_t aCount) {
    return mParser->OnDataAvailable(aRequest, aCtxt, aInput, aOffset, aCount);
}

NS_IMETHODIMP
nsIndexedToHTML::OnIndexAvailable(nsIRequest *aRequest,
                                  nsISupports *aCtxt,
                                  nsIDirIndex *aIndex) {
    nsresult rv;
    if (!aIndex)
        return NS_ERROR_NULL_POINTER;

    nsString pushBuffer;
    pushBuffer.AppendLiteral("<tr");

    nsXPIDLString description;
    aIndex->GetDescription(getter_Copies(description));
    if (description.First() == PRUnichar('.'))
        pushBuffer.AppendLiteral(" class=\"hidden-object\"");

    pushBuffer.AppendLiteral(">\n <td sortable-data=\"");

    
    
    uint32_t type;
    aIndex->GetType(&type);
    switch (type) {
        case nsIDirIndex::TYPE_SYMLINK:
            pushBuffer.AppendInt(0);
            break;
        case nsIDirIndex::TYPE_DIRECTORY:
            pushBuffer.AppendInt(1);
            break;
        case nsIDirIndex::TYPE_FILE:
        case nsIDirIndex::TYPE_UNKNOWN:
            pushBuffer.AppendInt(2);
            break;
    }
    PRUnichar* escaped = nsEscapeHTML2(description.get(), description.Length());
    pushBuffer.Append(escaped);

    pushBuffer.AppendLiteral("\"><a class=\"");
    switch (type) {
        case nsIDirIndex::TYPE_DIRECTORY:
            pushBuffer.AppendLiteral("dir");
            break;
        case nsIDirIndex::TYPE_SYMLINK:
            pushBuffer.AppendLiteral("symlink");
            break;
        case nsIDirIndex::TYPE_FILE:
        case nsIDirIndex::TYPE_UNKNOWN:
            pushBuffer.AppendLiteral("file");
            break;
    }
    pushBuffer.AppendLiteral("\"");

    
    
    nsString escapedShort;
    if (description.Length() > 71) {
        nsCOMPtr<nsIChannel> channel = do_QueryInterface(aRequest);
        nsCOMPtr<nsIURI> uri;
        rv = channel->GetURI(getter_AddRefs(uri));
        if (NS_FAILED(rv)) return rv;

        
        nsXPIDLString descriptionAffix;
        descriptionAffix.Assign(description);
        descriptionAffix.Cut(0, descriptionAffix.Length() - 25);
        if (NS_IS_LOW_SURROGATE(descriptionAffix.First()))
            descriptionAffix.Cut(0, 1);
        description.Truncate(std::min<uint32_t>(71, description.Length() - 28));
        if (NS_IS_HIGH_SURROGATE(description.Last()))
            description.Truncate(description.Length() - 1);

        escapedShort.Adopt(nsEscapeHTML2(description.get(), description.Length()));

        escapedShort.Append(mEscapedEllipsis);
        
        escapedShort.AppendLiteral("&#8203;");
        nsString tmp;
        tmp.Adopt(nsEscapeHTML2(descriptionAffix.get(), descriptionAffix.Length()));
        escapedShort.Append(tmp);

        pushBuffer.AppendLiteral(" title=\"");
        pushBuffer.Append(escaped);
        pushBuffer.AppendLiteral("\"");
    }
    if (escapedShort.IsEmpty())
        escapedShort.Assign(escaped);
    nsMemory::Free(escaped);

    pushBuffer.AppendLiteral(" href=\"");
    nsXPIDLCString loc;
    aIndex->GetLocation(getter_Copies(loc));

    nsXPIDLCString encoding;
    rv = mParser->GetEncoding(getter_Copies(encoding));
    if (NS_FAILED(rv)) return rv;

    
    loc.SetLength(nsUnescapeCount(loc.BeginWriting()));

    
    nsAutoCString locEscaped;

    
    
    if ((type == nsIDirIndex::TYPE_DIRECTORY) && (loc.Last() != '/')) {
        loc.Append('/');
    }

    
    uint32_t escFlags;
    
    
    
    if (mExpectAbsLoc &&
        NS_SUCCEEDED(net_ExtractURLScheme(loc, nullptr, nullptr, nullptr))) {
        
        escFlags = esc_Forced | esc_OnlyASCII | esc_AlwaysCopy | esc_Minimal;
    }
    else {
        
        
        
        
        escFlags = esc_Forced | esc_OnlyASCII | esc_AlwaysCopy | esc_FileBaseName | esc_Colon | esc_Directory;
    }
    NS_EscapeURL(loc.get(), loc.Length(), escFlags, locEscaped);
    
    
    
    locEscaped.ReplaceSubstring(";", "%3b");
    nsAutoString utf16URI;
    if (encoding.EqualsLiteral("UTF-8")) {
        
        nsCOMPtr<nsIUnicodeDecoder> decoder =
            mozilla::dom::EncodingUtils::DecoderForEncoding("UTF-8");
        decoder->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);

        int32_t len = locEscaped.Length();
        int32_t outlen = 0;
        rv = decoder->GetMaxLength(locEscaped.get(), len, &outlen);
        if (NS_FAILED(rv)) {
            return rv;
        }
        nsAutoArrayPtr<PRUnichar> outbuf(new PRUnichar[outlen]);
        rv = decoder->Convert(locEscaped.get(), &len, outbuf, &outlen);
        
        if (rv == NS_OK) {
            utf16URI.Append(outbuf, outlen);
        }
    }
    if (utf16URI.IsEmpty()) {
        
        nsAutoCString outstr;
        NS_EscapeURL(locEscaped, esc_AlwaysCopy | esc_OnlyNonASCII, outstr);
        CopyASCIItoUTF16(outstr, utf16URI);
    }
    nsString htmlEscapedURL;
    htmlEscapedURL.Adopt(nsEscapeHTML2(utf16URI.get(), utf16URI.Length()));
    pushBuffer.Append(htmlEscapedURL);

    pushBuffer.AppendLiteral("\">");

    if (type == nsIDirIndex::TYPE_FILE || type == nsIDirIndex::TYPE_UNKNOWN) {
        pushBuffer.AppendLiteral("<img src=\"moz-icon://");
        int32_t lastDot = locEscaped.RFindChar('.');
        if (lastDot != kNotFound) {
            locEscaped.Cut(0, lastDot);
            NS_ConvertUTF8toUTF16 utf16LocEscaped(locEscaped);
            nsString htmlFileExt;
            htmlFileExt.Adopt(nsEscapeHTML2(utf16LocEscaped.get(), utf16LocEscaped.Length()));
            pushBuffer.Append(htmlFileExt);
        } else {
            pushBuffer.AppendLiteral("unknown");
        }
        pushBuffer.AppendLiteral("?size=16\" alt=\"");

        nsXPIDLString altText;
        rv = mBundle->GetStringFromName(NS_LITERAL_STRING("DirFileLabel").get(),
                                        getter_Copies(altText));
        if (NS_FAILED(rv)) return rv;
        AppendNonAsciiToNCR(altText, pushBuffer);
        pushBuffer.AppendLiteral("\">");
    }

    pushBuffer.Append(escapedShort);
    pushBuffer.AppendLiteral("</a></td>\n <td");

    if (type == nsIDirIndex::TYPE_DIRECTORY || type == nsIDirIndex::TYPE_SYMLINK) {
        pushBuffer.AppendLiteral(">");
    } else {
        int64_t size;
        aIndex->GetSize(&size);

        if (uint64_t(size) != UINT64_MAX) {
            pushBuffer.AppendLiteral(" sortable-data=\"");
            pushBuffer.AppendInt(size);
            pushBuffer.AppendLiteral("\">");
            nsAutoString  sizeString;
            FormatSizeString(size, sizeString);
            pushBuffer.Append(sizeString);
        } else {
            pushBuffer.AppendLiteral(">");
        }
    }
    pushBuffer.AppendLiteral("</td>\n <td");

    PRTime t;
    aIndex->GetLastModified(&t);

    if (t == -1) {
        pushBuffer.AppendLiteral("></td>\n <td>");
    } else {
        pushBuffer.AppendLiteral(" sortable-data=\"");
        pushBuffer.AppendInt(static_cast<int64_t>(t));
        pushBuffer.AppendLiteral("\">");
        nsAutoString formatted;
        mDateTime->FormatPRTime(nullptr,
                                kDateFormatShort,
                                kTimeFormatNone,
                                t,
                                formatted);
        AppendNonAsciiToNCR(formatted, pushBuffer);
        pushBuffer.AppendLiteral("</td>\n <td>");
        mDateTime->FormatPRTime(nullptr,
                                kDateFormatNone,
                                kTimeFormatSeconds,
                                t,
                                formatted);
        
        AppendNonAsciiToNCR(formatted, pushBuffer);
    }

    pushBuffer.AppendLiteral("</td>\n</tr>");

    return FormatInputStream(aRequest, aCtxt, pushBuffer);
}

NS_IMETHODIMP
nsIndexedToHTML::OnInformationAvailable(nsIRequest *aRequest,
                                        nsISupports *aCtxt,
                                        const nsAString& aInfo) {
    nsAutoString pushBuffer;
    PRUnichar* escaped = nsEscapeHTML2(PromiseFlatString(aInfo).get());
    if (!escaped)
        return NS_ERROR_OUT_OF_MEMORY;
    pushBuffer.AppendLiteral("<tr>\n <td>");
    pushBuffer.Append(escaped);
    nsMemory::Free(escaped);
    pushBuffer.AppendLiteral("</td>\n <td></td>\n <td></td>\n <td></td>\n</tr>\n");
    
    return FormatInputStream(aRequest, aCtxt, pushBuffer);
}

void nsIndexedToHTML::FormatSizeString(int64_t inSize, nsString& outSizeString)
{
    outSizeString.Truncate();
    if (inSize > int64_t(0)) {
        
        int64_t  upperSize = (inSize + int64_t(1023)) / int64_t(1024);
        outSizeString.AppendInt(upperSize);
        outSizeString.AppendLiteral(" KB");
    }
}

nsIndexedToHTML::nsIndexedToHTML() {
}

nsIndexedToHTML::~nsIndexedToHTML() {
}
