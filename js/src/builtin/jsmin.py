




























"""A JavaScript minifier.

It is far from being a complete JS parser, so there are many valid
JavaScript programs that will be ruined by it.  Another strangeness is that
it accepts $ and % as parts of identifiers.  It doesn't merge lines or strip
out blank lines in order to ease debugging.  Variables at the top scope are
properties of the global object so we can't rename them.  It is assumed that
you introduce variables with var as if JavaScript followed C++ scope rules
around curly braces, so the declaration must be above the first use.

Use as:
import jsmin
minifier = JavaScriptMinifier()
program1 = minifier.JSMinify(program1)
program2 = minifier.JSMinify(program2)
"""

import re


class JavaScriptMinifier(object):
  """An object that you can feed code snippets to to get them minified."""

  def __init__(self):
    
    
    
    self.seen_identifiers = {"do": True, "in": True}
    self.identifier_counter = 0
    self.in_comment = False
    self.map = {}
    self.nesting = 0

  def LookAtIdentifier(self, m):
    """Records identifiers or keywords that we see in use.

    (So we can avoid renaming variables to these strings.)
    Args:
      m: The match object returned by re.search.

    Returns:
      Nothing.
    """
    identifier = m.group(1)
    self.seen_identifiers[identifier] = True

  def Push(self):
    """Called when we encounter a '{'."""
    self.nesting += 1

  def Pop(self):
    """Called when we encounter a '}'."""
    self.nesting -= 1
    
    
    if self.nesting == 0:
      self.map = {}
      self.identifier_counter = 0

  def Declaration(self, m):
    """Rewrites bits of the program selected by a regexp.

    These can be curly braces, literal strings, function declarations and var
    declarations.  (These last two must be on one line including the opening
    curly brace of the function for their variables to be renamed).

    Args:
      m: The match object returned by re.search.

    Returns:
      The string that should replace the match in the rewritten program.
    """
    matched_text = m.group(0)
    if matched_text == "{":
      self.Push()
      return matched_text
    if matched_text == "}":
      self.Pop()
      return matched_text
    if re.match("[\"'/]", matched_text):
      return matched_text
    m = re.match(r"var ", matched_text)
    if m:
      var_names = matched_text[m.end():]
      var_names = re.split(r",", var_names)
      return "var " + ",".join(map(self.FindNewName, var_names))
    m = re.match(r"(function\b[^(]*)\((.*)\)\{$", matched_text)
    if m:
      up_to_args = m.group(1)
      args = m.group(2)
      args = re.split(r",", args)
      self.Push()
      return up_to_args + "(" + ",".join(map(self.FindNewName, args)) + "){"

    if matched_text in self.map:
      return self.map[matched_text]

    return matched_text

  def CharFromNumber(self, number):
    """A single-digit base-52 encoding using a-zA-Z."""
    if number < 26:
      return chr(number + 97)
    number -= 26
    return chr(number + 65)

  def FindNewName(self, var_name):
    """Finds a new 1-character or 2-character name for a variable.

    Enters it into the mapping table for this scope.

    Args:
      var_name: The name of the variable before renaming.

    Returns:
      The new name of the variable.
    """
    new_identifier = ""
    
    
    
    if var_name in self.map:
      return self.map[var_name]
    if self.nesting == 0:
      return var_name
    while True:
      identifier_first_char = self.identifier_counter % 52
      identifier_second_char = self.identifier_counter // 52
      new_identifier = self.CharFromNumber(identifier_first_char)
      if identifier_second_char != 0:
        new_identifier = (
            self.CharFromNumber(identifier_second_char - 1) + new_identifier)
      self.identifier_counter += 1
      if not new_identifier in self.seen_identifiers:
        break

    self.map[var_name] = new_identifier
    return new_identifier

  def RemoveSpaces(self, m):
    """Returns literal strings unchanged, replaces other inputs with group 2.

    Other inputs are replaced with the contents of capture 1.  This is either
    a single space or an empty string.

    Args:
      m: The match object returned by re.search.

    Returns:
      The string that should be inserted instead of the matched text.
    """
    entire_match = m.group(0)
    replacement = m.group(1)
    if re.match(r"'.*'$", entire_match):
      return entire_match
    if re.match(r'".*"$', entire_match):
      return entire_match
    if re.match(r"/.+/$", entire_match):
      return entire_match
    return replacement

  def JSMinify(self, text):
    """The main entry point.  Takes a text and returns a compressed version.

    The compressed version hopefully does the same thing.  Line breaks are
    preserved.

    Args:
      text: The text of the code snippet as a multiline string.

    Returns:
      The compressed text of the code snippet as a multiline string.
    """
    new_lines = []
    for line in re.split(r"\n", text):
      line = line.replace("\t", " ")
      if self.in_comment:
        m = re.search(r"\*/", line)
        if m:
          line = line[m.end():]
          self.in_comment = False
        else:
          new_lines.append("")
          continue

      if not self.in_comment:
        line = re.sub(r"/\*.*?\*/", " ", line)
        line = re.sub(r"//.*", "", line)
        m = re.search(r"/\*", line)
        if m:
          line = line[:m.start()]
          self.in_comment = True

      
      line = re.sub(r"^ +", "", line)
      line = re.sub(r" +$", "", line)
      
      
      
      double_quoted_string = r'"(?:[^"\\]|\\.)*"'
      
      single_quoted_string = r"'(?:[^'\\]|\\.)*'"
      
      
      
      
      
      slash_quoted_regexp = r"(?<![\w$'\")\]])/(?:(?=\()|(?:[^()/\\]|\\.)+)(?:\([^/\\]|\\.)*/"
      
      line = re.sub("|".join([double_quoted_string,
                              single_quoted_string,
                              slash_quoted_regexp,
                              "( )+"]),
                    self.RemoveSpaces,
                    line)
      
      
      line = re.sub("|".join([double_quoted_string,
                              single_quoted_string,
                              slash_quoted_regexp,
                              r"(?<![a-zA-Z_0-9$%]) | (?![a-zA-Z_0-9$%])()"]),
                    self.RemoveSpaces,
                    line)
      
      if self.nesting == 0:
        re.sub(r"([a-zA-Z0-9_$%]+)", self.LookAtIdentifier, line)
      function_declaration_regexp = (
          r"\bfunction"              
          r"( [\w$%]+)?"             
          r"\([\w$%,]+\)\{")         
      
      
      
      
      if re.search(r"\?", line):
        block_trailing_colon = r""
      else:
        block_trailing_colon = r"(?![:\w$%])"
      
      variable_use_regexp = r"(?<![.\w$%])[\w$%]+" + block_trailing_colon
      line = re.sub("|".join([double_quoted_string,
                              single_quoted_string,
                              slash_quoted_regexp,
                              r"\{",                  
                              r"\}",
                              r"\bvar [\w$%,]+",      
                              function_declaration_regexp,
                              variable_use_regexp]),
                    self.Declaration,
                    line)
      new_lines.append(line)

    return "\n".join(new_lines) + "\n"
