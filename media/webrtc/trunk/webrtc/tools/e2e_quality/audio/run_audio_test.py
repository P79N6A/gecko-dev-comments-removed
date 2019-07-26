









"""Runs an end-to-end audio quality test on Linux.

Expects the presence of PulseAudio virtual devices (null sinks). These are
configured as default devices for a VoiceEngine audio call. A PulseAudio
utility (pacat) is used to play to and record from the virtual devices.

The input reference file is then compared to the output file.
"""

import optparse
import os
import re
import shlex
import subprocess
import sys
import time

import perf.perf_utils

def main(argv):
  parser = optparse.OptionParser()
  usage = 'Usage: %prog [options]'
  parser.set_usage(usage)
  parser.add_option('--input', default='input.pcm', help='input PCM file')
  parser.add_option('--output', default='output.pcm', help='output PCM file')
  parser.add_option('--codec', default='ISAC', help='codec name')
  parser.add_option('--rate', default='16000', help='sample rate in Hz')
  parser.add_option('--channels', default='1', help='number of channels')
  parser.add_option('--play_sink', default='capture',
      help='name of PulseAudio sink to which to play audio')
  parser.add_option('--rec_sink', default='render',
      help='name of PulseAudio sink whose monitor will be recorded')
  parser.add_option('--harness',
      default=os.path.abspath(os.path.dirname(sys.argv[0]) +
          '/../../../out/Debug/audio_e2e_harness'),
      help='path to audio harness executable')
  parser.add_option('--compare',
                    help='command-line arguments for comparison tool')
  parser.add_option('--regexp',
                    help='regular expression to extract the comparison metric')
  options, _ = parser.parse_args(argv[1:])

  
  command = ['pacmd', 'list-sources']
  print ' '.join(command)
  proc = subprocess.Popen(command, stdout=subprocess.PIPE)
  output = proc.communicate()[0]
  if proc.returncode != 0:
    return proc.returncode
  default_source = re.search(r'(^  \* index: )([0-9]+$)', output,
                             re.MULTILINE).group(2)

  
  
  
  
  
  
  command = ['pacmd', 'set-default-source', options.play_sink + '.monitor']
  print ' '.join(command)
  retcode = subprocess.call(command, stdout=subprocess.PIPE)
  if retcode != 0:
    return retcode

  command = [options.harness, '--render=' + options.rec_sink,
      '--codec=' + options.codec, '--rate=' + options.rate]
  print ' '.join(command)
  voe_proc = subprocess.Popen(command)

  
  
  
  
  time.sleep(5)

  format_args = ['--format=s16le', '--rate=' + options.rate,
      '--channels=' + options.channels, '--raw']
  command = (['pacat', '-p', '-d', options.play_sink] + format_args +
      [options.input])
  print ' '.join(command)
  play_proc = subprocess.Popen(command)

  command = (['pacat', '-r', '-d', options.rec_sink + '.monitor'] +
      format_args + [options.output])
  print ' '.join(command)
  record_proc = subprocess.Popen(command)

  retcode = play_proc.wait()
  
  record_proc.kill()
  voe_proc.kill()
  if retcode != 0:
    return retcode

  
  command = ['pacmd', 'set-default-source', default_source]
  print ' '.join(command)
  retcode = subprocess.call(command, stdout=subprocess.PIPE)
  if retcode != 0:
    return retcode

  if options.compare and options.regexp:
    command = shlex.split(options.compare) + [options.input, options.output]
    print ' '.join(command)
    proc = subprocess.Popen(command, stdout=subprocess.PIPE)
    output = proc.communicate()[0]
    if proc.returncode != 0:
      return proc.returncode

    
    value = ''.join(re.findall(options.regexp, output))

    perf.perf_utils.PrintPerfResult(graph_name='audio_e2e_score',
                                    series_name='e2e_score',
                                    data_point=value,
                                    units='MOS')  

  return 0

if __name__ == '__main__':
  sys.exit(main(sys.argv))
