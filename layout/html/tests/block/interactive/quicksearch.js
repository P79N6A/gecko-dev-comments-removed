




















var no_result="---";




function do_unshift(l, s) {
  l.length = l.length + 1;
  for (var i=l.length-1; i>0; i--) {
    l[i] = l[i-1];
  }
  l[0] = s;
  return l.length;
}




function do_shift(l) {
  var l0=l[0];
  for (var i=0; i<l.length-1; i++) {
    l[i] = l[i+1];
  }
  l.length = l.length - 1;
  return l0;
}

function go_to (url) {
  document.location.href = url;
  
}

function map(l, f) {
    var l1 = new Array();
    for (var i=0; i<l.length; i++) {
        l1[i] = f(l[i]);
    }
    return l1;
}

function isPrefix(s1, s2) {
    return (s1.length <= s2.length) &&
           (s1 == s2.substring(0,s1.length))
}

function member(s, l) {
    for (var i=0; i<l.length; i++) {
        if (l[i] == s) return true;
    }
    return false;
}

function add(s, l) {
    if (! member(s, l)) {
        do_unshift(l,s);
    }
}

function addAll(l1, l2) {
    for (var i=0; i<l1.length; i++) {
        add(l1[i],l2);
    }        
}

function isSubset (l1, l2) {
    return (l1.length == 0)
    || (member(l1[0],l2) && subset(l1.slice(1),l2));
}



var f1 = new Array();
var f2 = new Array();

function add_mapping(from,to) {
    f1[f1.length] = from;
    f2[f2.length] = to;
}


add_mapping("status",             "bug_status");
add_mapping("resolution",         "resolution");  
add_mapping("platform",           "rep_platform");
add_mapping("os",                 "op_sys");
add_mapping("opsys",              "op_sys");
add_mapping("priority",           "priority");    
add_mapping("pri",                "priority");
add_mapping("severity",           "bug_severity");
add_mapping("sev",                "bug_severity");

add_mapping("owner",              "assigned_to");
add_mapping("assignee",           "assigned_to");
add_mapping("assignedto",         "assigned_to");
add_mapping("reporter",           "reporter");    
add_mapping("rep",                "reporter");
add_mapping("qa",                 "qa_contact"); 
add_mapping("qacontact",          "qa_contact");
add_mapping("cc",                 "cc");          

add_mapping("product",            "product");     
add_mapping("prod",               "product");
add_mapping("version",            "version");     
add_mapping("ver",                "version");
add_mapping("component",          "component");   
add_mapping("comp",               "component");
add_mapping("milestone",          "target_milestone");
add_mapping("target",             "target_milestone");
add_mapping("targetmilestone",    "target_milestone");

add_mapping("summary",            "short_desc");
add_mapping("shortdesc",          "short_desc");
add_mapping("desc",               "longdesc");
add_mapping("description",        "longdesc");

          
add_mapping("longdesc",           "longdesc");
add_mapping("url",                "bug_file_loc");
add_mapping("whiteboard",         "status_whiteboard");
add_mapping("statuswhiteboard",   "status_whiteboard");
add_mapping("sw",                 "status_whiteboard");
add_mapping("keywords",           "keywords");    
add_mapping("kw",                 "keywords");

add_mapping("attachment",         "attachments.description");
add_mapping("attachmentdesc",     "attachments.description");
add_mapping("attachdesc",         "attachments.description");
add_mapping("attachmentdata",     "attachments.thedata");
add_mapping("attachdata",         "attachments.thedata");
add_mapping("attachmentmimetype", "attachments.mimetype");
add_mapping("attachmimetype",     "attachments.mimetype");


















function findIndex(array,value) {
    for (var i=0; i<array.length; i++)
        if (array[i] == value) return i;
    return -1;
}

function mapField(fieldname) {
    var i = findIndex(f1,fieldname);
    if (i >= 0) return f2[i];
    return no_result;
} 


 
function is_keyword(s) {
    return member(s, keywords);
}



function is_platform(str) {
    return member (str.toLowerCase(),platforms);
}



function is_severity(str) {
    return member(str.toLowerCase(),severities);
}



function match_product(str) {
    var s = str.toLowerCase();
    return (s.length > 2) && (! member(s,product_exceptions));
}



function match_component(str) {
    var s = str.toLowerCase();
    return (s.length > 2) && (! member(s,component_exceptions));
}

var status_and_resolution = ""; 
var charts = "";                



function make_chart(expr, field, type, value) {
    charts += "<tr>" +
              "<td><tt>" + expr + "</tt></td>" + 
              "<td><tt>" + field + "</tt></td>" + 
              "<td><tt>" + type + "</tt></td>" + 
              "<td><tt>" + value + "</tt></td>" +
              "</tr>";
    return "&field" + expr + "=" + field +
           "&type"  + expr + "=" + type  +
           "&value" + expr + "=" + encodeURIComponent(value).replace(/[+]/g,"%2B");
}


function addPrefixMatches(prefix, comparelist, resultlist) {
    var foundMatch = false;
    for (var i=0; i<comparelist.length; i++) {
        if (isPrefix(prefix,comparelist[i])) {
            foundMatch = true;
            add(comparelist[i],resultlist);
        }
    }
    return foundMatch;
}

function prefixesNotFoundError(prefixes,statusValues,resolutionValues) {
    var txt;
    if (prefixes.length == 1) {
        txt = "is not a prefix ";
    } else {
        txt = "are not prefixes ";
    }
    alert(prefixes + "\n" + txt + 
          "of one of these status or resolution values:\n" +
          statusValues + "\n" + resolutionValues + "\n");
}

function make_query_URL(url, input, searchLong) {

    status_and_resolution = "";
    charts = "";

    
    
    var searchURL = url;  
    var abort = false;    
   
    var i,j,k,l;          
    var parts,input2;     

    var word;                  
                               
    var alternative;           
                               
    var comma_separated_words; 
    var w;                     
                               
    
    var w0;               
    var prefixes;         
                          

    var expr;             
    var n,separator;      
 
    var colon_separated_parts, fields,values,field;
                          

    var chart,and,or;     
    var negation;         

    
    var statusOpen     = statuses_open;
    var statusResolved = statuses_resolved;
    var statusAll      = statusOpen.concat(statusResolved);

    
    var bug_status = statusOpen.slice().reverse(); 
    var resolution = new Array();
    
    
    parts = input.split('"');
    if ((parts.length % 2) != 1) {
        alert('Unterminated quote');
        abort = true;
        return no_result;      
    }
    for (i=1; i<parts.length; i+=2) {
        parts[i] = encodeURIComponent(parts[i]);
    }
    input2 = parts.join('"');

    
    if (input2.match(/[(]|[\)]/)) {
        alert('Brackets (...) are not supported.\n' + 
              'Use quotes "..." for values that contain special characters.');
        abort = true;
        return no_result;
    }

    
    input2 = input2.replace(/[\s]+AND[\s]+/g," ");
    input2 = input2.replace(/[\s]+OR[\s]+/g,"|");
    input2 = input2.replace(/[\s]+NOT[\s]+/g," -");

    
    word = input2.split(/[\s]+/);

    
    

    
    
    
    
    function matchPrefixes(prefixes,statuses,resolutions) {
        var failedPrefixes = new Array();
        var foundMatch = false;
        for (var j=0; j<prefixes.length; j++) {
            var ok1 = addPrefixMatches(prefixes[j],statuses,bug_status);
            var ok2 = addPrefixMatches(prefixes[j],resolutions,resolution);
            if ((! ok1) && (! ok2)) {
                add(prefixes[j],failedPrefixes);
            } else {
                foundMatch = true;
            }
        }
        
        if (foundMatch && (failedPrefixes.length > 0)) {
            prefixesNotFoundError(failedPrefixes,statuses,resolutions);
            abort = true;
        }
        return foundMatch;
    }
    
    if (word[0] == "ALL") {
        
        addAll(statusResolved,bug_status);
        do_shift(word);
    } else if (word[0] == "OPEN") {
        
        do_shift(word);
    } else if (word[0].match("^[+][A-Z]+(,[A-Z]+)*$")) {
        
        w0 = do_shift(word);
        prefixes = w0.substring(1).split(",");
        if (! matchPrefixes(prefixes,statusResolved,resolutions)) {
            do_unshift(word,w0);
        }
    } else if (word[0].match("^[A-Z]+(,[A-Z]+)*$")) {
        
        bug_status = new Array(); 
        w0 = do_shift(word);
        prefixes = w0.split(",");
        if (! matchPrefixes(prefixes,statusAll,resolutions)) {
            do_unshift(word,w0);
            bug_status = statusOpen.reverse(); 
        }
    } else {
        
        
        
        
    }
    if (resolution.length > 0) {
        resolution = resolution.reverse();
        do_unshift(resolution,"---");
        addAll(statusResolved,bug_status);
    }
    bug_status = bug_status.reverse();
    bug_status = map(bug_status,escape);
    searchURL += "?bug_status=" +  bug_status.join("&bug_status=");
    status_and_resolution += 'Status: <tt>'+bug_status+'</tt>';

    if (resolution.length > 0) {
        resolution = map(resolution,escape);
        searchURL += "&resolution=" + resolution.join("&resolution=");
        status_and_resolution += '<br>'+'Resolution: <tt>'+resolution+'</tt>';
    }
                              
    

    chart = 0;
    and   = 0;
    or    = 0;

    negation = false;

    function negate_comparison_type(type) {
        switch(type) {
            case "substring": return "notsubstring";
            case "anywords":  return "nowords";
            case "regexp":    return "notregexp";
            default:
                
                alert("Can't negate comparison type: `" + type + "'");
                abort = true;
                return "dummy";
        }
    }

    function add_chart(field,type,value) {
        
        var parts = value.split('"');
        if ((parts.length % 2) != 1) {
            alert('Internal error: unescaping failure');
            abort = true;
        }
        for (var i=1; i<parts.length; i+=2) {
            parts[i] = decodeURIComponent(parts[i]);
        }
        var value2 = parts.join('');

        
        var type2 = type;
        if (negation) {
            type2 = negate_comparison_type(type2);
        }
        searchURL += make_chart(chart+"-"+and+"-"+or,field,type2,value2);
        or++;
        if (negation) {
            and++;
            or=0;
        }
    }

    for (i=0; i<word.length; i++, chart++) {

        w = word[i];
        
        negation = false;
        if (w.charAt(0) == "-") {
            negation = true;
            w = w.substring(1);
        }

        switch (w.charAt(0)) {
            case "+":
                alternative = w.substring(1).split(/[|,]/);
                for (j=0; j<alternative.length; j++)
                    add_chart("short_desc","substring",alternative[j]);
                break;
            case "#":
                alternative = w.substring(1).replace(/[|,]/g," ");
                add_chart("short_desc","anywords",alternative);
                if (searchLong)
                    add_chart("longdesc","anywords",alternative);
                break;
            case ":":
                alternative = w.substring(1).split(",");
                for (j=0; j<alternative.length; j++) {
                    add_chart("product","substring",alternative[j]);
                    add_chart("component","substring",alternative[j]);
                }
                break;
            case "@":
                alternative = w.substring(1).split(",");
                for (j=0; j<alternative.length; j++)
                    add_chart("assigned_to","substring",alternative[j]);
                break;
            case "[":
                add_chart("short_desc","substring",w);
                add_chart("status_whiteboard","substring",w);
                break;
            case "!":
                add_chart("keywords","anywords",w.substring(1));
                break;
            default:
                alternative=w.split("|");
                for (j=0; j<alternative.length; j++) {

                    w=alternative[j];

                    
                    if (w.match("^votes[:][0-9]+$")) {
                        n = w.split(/[:]/)[1];
                        add_chart("votes","greaterthan",String(n-1));
                        continue;
                    }
                    
                    if (w.match("^[^:]+[:][^:\/][^:]*$")) {
                        colon_separated_parts = w.split(":");
                        fields = colon_separated_parts[0].split(/[,]+/);
                        values = colon_separated_parts[1].split(/[,]+/);
                        for (k=0; k<fields.length; k++) {
                            field = mapField(fields[k]);
                            if (field == no_result) {
                                alert("`"+fields[k]+"'"+
                                      " is not a valid field name.");
                                abort = true;
                                return no_result;
                            } else {
                                 for (l=0; l<values.length; l++) {
                                     add_chart(field,"substring",values[l]);
                                 }
                            }  
                        }
                        continue;
                    }
                    comma_separated_words=w.split(/[,]+/);
                    for (k=0; k<comma_separated_words.length; k++) {
                        w=comma_separated_words[k];

                        
                        if (is_platform(w)) {
                            add_chart("rep_platform","substring",w);
                            continue;
                        }
                        
                        if (w.match("^[pP][1-5](,[pP]?[1-5])*$")) {
                            expr = "["+w.replace(/[p,]/g,"")+"]";
                            add_chart("priority","regexp",expr);
                            continue;
                        }
                        if (w.match("^[pP][1-5]-[1-5]$")) {
                            expr = "["+w.substring(1)+"]";
                            add_chart("priority","regexp",expr);
                            continue;
                        }
                        
                        if (is_severity(w)) {
                            add_chart("bug_severity","substring",w);
                            continue;
                        }
                        
                        if (w.match("^votes>[0-9]+$")) {
                            n = w.split(">")[1];
                            add_chart("votes","greaterthan",n);
                            continue;
                        }
                        
                        if (w.match("^votes(>=|=>)[0-9]+$")) {
                            separator = w.match("^votes(>=|=>)[0-9]+$")[1];
                            n = w.split(separator)[1];
                            add_chart("votes","greaterthan",String(n-1));
                            continue;
                        }
                        
                        if (match_product(w)) {
                            add_chart("product","substring",w);
                        }
                        if (match_component(w)) {
                            add_chart("component","substring",w);
                        }
                        if (is_keyword(w)) {
                            add_chart("keywords","substring",w);
                            if (w.length > 2) {
                                add_chart("short_desc","substring",w);
                                add_chart("status_whiteboard","substring",w);
                            }
                        } else {
                            add_chart("short_desc","substring",w);
                            add_chart("status_whiteboard","substring",w);
                        }
                        if (searchLong)
                            add_chart("longdesc","substring",w);
                 
                        
                        if (w.match(/[0-9]+[.][0-9]+[.][0-9]+[.][0-9]+/)
                           || w.match(/^[A-Za-z]+([.][A-Za-z]+)+/)
                           || w.match(/[:][\/][\/]/)
                           || w.match(/localhost/)
                           || w.match(/mailto[:]?/)
                           
                           )
                            add_chart("bug_file_loc","substring",w);
                    }
                }
        }
        and = 0;
        or = 0;
    }

    

    if (abort == false) {
        return searchURL;
    } else {
        return no_result;
    }
}

function unique_id () {
    return (new Date()).getTime();
}

function ShowURL(mode) {
    var input = document.f.id.value;
    var searchURL = make_query_URL(bugzilla+"buglist.cgi", input, false);
    if (searchURL != no_result) {
        var pieces = searchURL.replace(/[\?]/g,"\n?").replace(/[\&]/g,"\n&");
        if (mode == "alert") {
            alert(pieces);
        } else {
            var table = "<table border=1>" + 
                          "<thead>" + 
                            "<tr>" + 
                              "<th>Chart-And-Or</th>" + 
                              "<th>Field</th>" + 
                              "<th>Type</th>" + 
                              "<th>Value</th>" + 
                            "</tr>" + 
                          "</thead>" + 
                          "<tbody>" + charts + "</tbody>" +
                        "</table>";
            var html = '<html>' + 
                         '<head>' + 
                           '<title>' + input + '</title>' +
                         '</head>' +
                         '<body>' + 
                           '<a href="' + searchURL + '">' +
                             'Submit Query' +
                           '</a>' +
                           '<p>' + status_and_resolution + 
                           '<p>' + table + 
                           '<pre>' +
                             pieces.replace(/[\n]/g,"<br>") +
                           '</pre>' +  
                         '</body>' +
                       '</html>';
            var w = window.open("","preview_"+unique_id());
            w.document.write(html);
            w.document.close();
        }
    }
}





function Search(url, input, searchLong) {
    var inputstring = new String(input);
    var word = inputstring.split(/[\s]+/);
  
    
    if ( word.length == 1 && word[0] == "" )
        return;
    
    
    if ((searchLong!=false) && word.length > 4) {  
        var message = "Searching Descriptions for more than four words " +
                      "will take a very long time indeed. Please choose " +
                      "no more than four keywords for your query.";
        alert(message);
        return;
    }
    var searchURL = make_query_URL(url, inputstring, searchLong);
    if (searchURL != no_result) {
        go_to(searchURL);
         
    } else {
        return;
    }
}



























function QuickSearch ()
{
    var input = document.f.id.value;

    
    input = input.replace(/^[\s]+/,"").replace(/[\s]+$/,"");

    if (input == "") 
    {
        
        go_to(bugzilla);
    } 
    else if (input.match(/^[0-9, ]*$/)) 
    {
        if (input.indexOf(",") == -1) {
            
            go_to(bugzilla+"show_bug.cgi?id="+encodeURIComponent(input));
        } else {
            
            go_to(bugzilla+"buglist.cgi?bug_id="+encodeURIComponent(input)
                  + "&bugidtype=include&order=bugs.bug_id");
        }
    }
    else
    {
        Search(bugzilla+"buglist.cgi",input,false);
    }
    return;
}

function LoadQuery() {
    var input = document.f.id.value;

    
    input = input.replace(/^[\s]+/,"").replace(/[\s]+$/,"");

    Search(bugzilla+"query.cgi",input,false);
    return;
}

