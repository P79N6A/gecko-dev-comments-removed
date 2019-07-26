









from __future__ import with_statement
import re, sys, os, js2c, fileinput

def replaceErrorMsgs(source_files, messages_file, output_file):
    messages = buildMessagesTable(messages_file)
    
    
    message_keys = messages.keys()
    message_keys.sort(reverse=True)
    with open(output_file, 'w') as output:
        if len(source_files) == 0:
            return
        for line in fileinput.input(source_files):
            line = line if line[-1] == '\n' else line + '\n'
            output.write(replaceMessages(line, messages, message_keys))

def buildMessagesTable(messages_file):
    table = {}
    pattern = re.compile(r"MSG_DEF\(([\w_]+),\s*(\d+)")
    for line in fileinput.input(messages_file):
        match = pattern.match(line)
        if match:
            table[match.group(1)] = match.group(2)
    return table

def replaceMessages(line, messages, message_keys):
    if not 'JSMSG_' in line:
        return line
    for key in message_keys:
        line = line.replace(key, messages[key])
    return line

def main():
    debug = sys.argv[1] == '-d'
    if debug:
        sys.argv.pop(1)
    output_file = sys.argv[1]
    messages_file = sys.argv[2]
    macros_file = sys.argv[3]
    source_files = sys.argv[4:]
    combined_file = 'selfhosted.js'
    replaceErrorMsgs(source_files, messages_file, combined_file)
    combined_sources = js2c.JS2C([combined_file, macros_file], [output_file], { 'TYPE': 'CORE', 'COMPRESSION': 'off', 'DEBUG':debug })
    with open(combined_file, 'w') as output:
        output.write(combined_sources)

if __name__ == "__main__":
    main()
