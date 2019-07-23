





































"""Writes a report with the results of the Ts (startup) and Tp (page load) tests.

   The report contains the mean startup time for each profile and the standard
   deviation, the sum of page load times and the standard deviation, and a graph
   of each performance counter measured during the page load test.
"""

__author__ = 'annie.sullivan@gmail.com (Annie Sullivan)'


import csv
import math
import matplotlib.mlab
import os
import pylab
import re
import time

import paths


def MakeArray(start, len, step):
  """Helper function to create an array for an axis to plot counter data.
  
  Args:
    start: The first value in the array
    len: The length of the array
    step: The difference between values in the array
  
  Returns:
    An array starting at start, with len values each step apart.
  """
  
  count = start
  end = start + (len * step)
  array = []
  while count < end:
    array.append(count)
    count += step
    
  return array


def GetPlottableData(counter_name, data):
  """Some counters should be displayed as a moving average, or
     may need other adjustment to be plotted.  This function
     makes adjustments to the data based on counter name.

  Args:
    counter_name: The name of the counter, i.e 'Working Set'
    data: The original data collected from the counter

  Returns:
    data array adjusted based on counter name.
  """

  if counter_name == '% Processor Time':
    
    return matplotlib.mlab.movavg(data, 5)
  if counter_name == 'Working Set' or counter_name == 'Private Bytes':
    
    return [float(x) / 1000000 for x in data]

  
  return data


def GenerateReport(title, filename, configurations, ts_times, tp_times, tp_counters, tp_resolution):
  """ Generates a report file in html using the given data
  
  Args:
    title: Title of the report
    filename: Filename of the report, before the timestamp
    configurations: Array of strings, containing the name of
                    each configuration tested.
    ts_times: Array of arrays of ts startup times for each configuration.
    tp_times: Array of page load times for each configuration tested.
    tp_counters: Array of counter data for page load configurations
    
  Returns:
    filename of html report.
  """
  
  
  graphs_subdir = os.path.join(paths.REPORTS_DIR, 'graphs')
  if not os.path.exists(graphs_subdir):
    os.makedirs(graphs_subdir) 

  
  localtime = time.localtime()
  timestamp = int(time.mktime(localtime))
  report_filename = os.path.join(paths.REPORTS_DIR, filename + "_" + str(timestamp) + ".html")
  report = open(report_filename, 'w')
  report.write('<html><head><title>Performance Report for %s, %s</title></head>\n' %
               (title, time.strftime('%m-%d-%y')))
  report.write('<body>\n')
  report.write('<h1>%s, %s</h1>' % (title, time.strftime('%m-%d-%y')))
  
  
  report.write('<p><h2>Startup Test (Ts) Results</h2>\n')
  report.write('<table border="1" cellpadding="5" cellspacing="0">\n')
  report.write('<tr>')
  report.write('<th>Profile Tested</th>')
  report.write('<th>Mean</th>')
  report.write('<th>Standard Deviation</th></tr>\n')
  ts_csv_filename =  os.path.join(paths.REPORTS_DIR, filename + "_" + str(timestamp) + '_ts.csv')
  ts_csv_file = open(ts_csv_filename, 'wb')
  ts_csv = csv.writer(ts_csv_file)
  for i in range (0, len(configurations)):
    
    mean = 0
    for ts_time in ts_times[i]:
      mean += float(ts_time)
    mean = mean / len(ts_times[i])
    
    
    stdd = 0
    for ts_time in ts_times[i]:
      stdd += (float(ts_time) - mean) * (float(ts_time) - mean)
    stdd = stdd / len(ts_times[i])
    stdd = math.sqrt(stdd)
    
    report.write('<tr><td>%s</td><td>%f</td><td>%f</td></tr>\n' %
                 (configurations[i], mean, stdd))
    ts_csv.writerow([configurations[i], mean, stdd])
  report.write('</table></p>\n')
  ts_csv_file.close()
  
  
  report.write('<p><h2>Page Load Test (Tp) Results</h2>\n')
  report.write('<table border="1" cellpadding="5" cellspacing="0">\n')
  report.write('<tr>')
  report.write('<th>Profile Tested</th>')
  report.write('<th>Sum of mean times</th>')
  report.write('<th>Sum of Standard Deviations</th></tr>\n')
  tp_csv_filename =  os.path.join(paths.REPORTS_DIR, filename + "_" + str(timestamp) + '_tp.csv')
  tp_csv_file = open(tp_csv_filename, 'wb')
  tp_csv = csv.writer(tp_csv_file)
  
  
  for i in range (0, len(tp_times)):
    (tmp1, mean, tmp2, stdd) = tp_times[i].split()
    report.write('<tr><td>%s</td><td>%f</td><td>%f</td></tr>\n' %
                 (configurations[i], float(mean), float(stdd)))
    tp_csv.writerow([configurations[i], float(mean), float(stdd)])
  report.write('</table></p>\n')
  tp_csv_file.close()
  
  
  report.write('<p><h2>Performance Data</h2></p>\n')
  
  
  colors = ['r-', 'g-', 'b-', 'y-', 'c-', 'm-']
  nonchar = re.compile('[\W]*')
  if len(tp_counters) > 0:
    counter_names = []
    for counter in tp_counters[0]:
      counter_names.append(counter)
    for counter_name in counter_names:

      
      pylab.clf()

      
      pylab.title(counter_name)
      pylab.ylabel(counter_name)
      pylab.xlabel("Time")

      
      current_color = 0
      line_handles = [] 
      for count_data in tp_counters:
        data = GetPlottableData(counter_name, count_data[counter_name])
        times = MakeArray(0, len(data), tp_resolution)
        handle = pylab.plot(times, data, colors[current_color])
        line_handles.append(handle)
        current_color = (current_color + 1) % len(colors)

      
      legend = pylab.legend(line_handles, configurations, 'upper right')
      ltext = legend.get_texts()
      pylab.setp(ltext, fontsize='small')  

      
      image_name = os.path.join(graphs_subdir,
                                filename + "_" + str(timestamp) + nonchar.sub('', counter_name) + '.png')
      pylab.savefig(image_name)
      img_src = image_name[len(paths.REPORTS_DIR) : ]
      if img_src.startswith('\\'):
        img_src = img_src[1 : ]
      img_src = img_src.replace('\\', '/')
      report.write('<p><img src="%s" alt="%s"></p>\n' % (img_src, counter_name))
      
  report.write('</body></html>\n')
  
  return report_filename
