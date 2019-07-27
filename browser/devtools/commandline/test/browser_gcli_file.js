















'use strict';




const exports = {};

function test() {
  helpers.runTestModule(exports, "browser_gcli_file.js");
}



var local = false;

exports.testBasic = function(options) {
  return helpers.audit(options, [
    {
      skipRemainingIf: options.isFirefox, 
      setup:    'tsfile open /',
      check: {
        input:  'tsfile open /',
        hints:               '',
        markup: 'VVVVVVVVVVVVI',
        cursor: 13,
        current: 'p1',
        status: 'ERROR',
        message: '\'/\' is not a file',
        args: {
          command: { name: 'tsfile open' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' is not a file'
          }
        }
      }
    },
    {
      setup:    'tsfile open /zxcv',
      check: {
        input:  'tsfile open /zxcv',
        
        markup: 'VVVVVVVVVVVVIIIII',
        cursor: 17,
        current: 'p1',
        status: 'ERROR',
        message: '\'/zxcv\' doesn\'t exist',
        args: {
          command: { name: 'tsfile open' },
          p1: {
            value: undefined,
            arg: ' /zxcv',
            status: 'INCOMPLETE',
            message: '\'/zxcv\' doesn\'t exist'
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile open /mach_kernel',
      check: {
        input:  'tsfile open /mach_kernel',
        hints:                          '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 24,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile open' },
          p1: {
            value: '/mach_kernel',
            arg: ' /mach_kernel',
            status: 'VALID',
            message: ''
           }
        }
      }
    },
    {
      setup:    'tsfile saveas /',
      check: {
        input:  'tsfile saveas /',
        hints:                 '',
        markup: 'VVVVVVVVVVVVVVI',
        cursor: 15,
        current: 'p1',
        status: 'ERROR',
        message: '\'/\' already exists',
        args: {
          command: { name: 'tsfile saveas' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' already exists'
          }
        }
      }
    },
    {
      setup:    'tsfile saveas /zxcv',
      check: {
        input:  'tsfile saveas /zxcv',
        
        markup: 'VVVVVVVVVVVVVVVVVVV',
        cursor: 19,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile saveas' },
          p1: {
            value: '/zxcv',
            arg: ' /zxcv',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile saveas /mach_kernel',
      check: {
        input:  'tsfile saveas /mach_kernel',
        hints:                            '',
        markup: 'VVVVVVVVVVVVVVIIIIIIIIIIII',
        cursor: 26,
        current: 'p1',
        status: 'ERROR',
        message: '\'/mach_kernel\' already exists',
        args: {
          command: { name: 'tsfile saveas' },
          p1: {
            value: undefined,
            arg: ' /mach_kernel',
            status: 'INCOMPLETE',
            message: '\'/mach_kernel\' already exists'
          }
        }
      }
    },
    {
      setup:    'tsfile save /',
      check: {
        input:  'tsfile save /',
        hints:               '',
        markup: 'VVVVVVVVVVVVI',
        cursor: 13,
        current: 'p1',
        status: 'ERROR',
        message: '\'/\' is not a file',
        args: {
          command: { name: 'tsfile save' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' is not a file'
          }
        }
      }
    },
    {
      setup:    'tsfile save /zxcv',
      check: {
        input:  'tsfile save /zxcv',
        
        markup: 'VVVVVVVVVVVVVVVVV',
        cursor: 17,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile save' },
          p1: {
            value: '/zxcv',
            arg: ' /zxcv',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile save /mach_kernel',
      check: {
        input:  'tsfile save /mach_kernel',
        hints:                          '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 24,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile save' },
          p1: {
            value: '/mach_kernel',
            arg: ' /mach_kernel',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tsfile cd /',
      check: {
        input:  'tsfile cd /',
        hints:             '',
        markup: 'VVVVVVVVVVV',
        cursor: 11,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile cd' },
          p1: {
            value: '/',
            arg: ' /',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tsfile cd /zxcv',
      check: {
        input:  'tsfile cd /zxcv',
        
        markup: 'VVVVVVVVVVIIIII',
        cursor: 15,
        current: 'p1',
        status: 'ERROR',
        message: '\'/zxcv\' doesn\'t exist',
        args: {
          command: { name: 'tsfile cd' },
          p1: {
            value: undefined,
            arg: ' /zxcv',
            status: 'INCOMPLETE',
            message: '\'/zxcv\' doesn\'t exist'
          }
        }
      }
    },
    {
      skipIf: true || !local,
      setup:    'tsfile cd /etc/passwd',
      check: {
        input:  'tsfile cd /etc/passwd',
        hints:                       ' -> /etc/pam.d/',
        markup: 'VVVVVVVVVVIIIIIIIIIII',
        cursor: 21,
        current: 'p1',
        status: 'ERROR',
        message: '\'/etc/passwd\' is not a directory',
        args: {
          command: { name: 'tsfile cd' },
          p1: {
            value: undefined,
            arg: ' /etc/passwd',
            status: 'INCOMPLETE',
            message: '\'/etc/passwd\' is not a directory'
          }
        }
      }
    },
    {
      setup:    'tsfile mkdir /',
      check: {
        input:  'tsfile mkdir /',
        hints:                '',
        markup: 'VVVVVVVVVVVVVI',
        cursor: 14,
        current: 'p1',
        status: 'ERROR',
        message: ''/' already exists',
        args: {
          command: { name: 'tsfile mkdir' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' already exists'
          }
        }
      }
    },
    {
      setup:    'tsfile mkdir /zxcv',
      check: {
        input:  'tsfile mkdir /zxcv',
        
        markup: 'VVVVVVVVVVVVVVVVVV',
        cursor: 18,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile mkdir' },
          p1: {
            value: '/zxcv',
            arg: ' /zxcv',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile mkdir /mach_kernel',
      check: {
        input:  'tsfile mkdir /mach_kernel',
        hints:                           '',
        markup: 'VVVVVVVVVVVVVIIIIIIIIIIII',
        cursor: 25,
        current: 'p1',
        status: 'ERROR',
        message: '\'/mach_kernel\' already exists',
        args: {
          command: { name: 'tsfile mkdir' },
          p1: {
            value: undefined,
            arg: ' /mach_kernel',
            status: 'INCOMPLETE',
            message: '\'/mach_kernel\' already exists'
          }
        }
      }
    },
    {
      setup:    'tsfile rm /',
      check: {
        input:  'tsfile rm /',
        hints:             '',
        markup: 'VVVVVVVVVVV',
        cursor: 11,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile rm' },
          p1: {
            value: '/',
            arg: ' /',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tsfile rm /zxcv',
      check: {
        input:  'tsfile rm /zxcv',
        
        markup: 'VVVVVVVVVVIIIII',
        cursor: 15,
        current: 'p1',
        status: 'ERROR',
        message: '\'/zxcv\' doesn\'t exist',
        args: {
          command: { name: 'tsfile rm' },
          p1: {
            value: undefined,
            arg: ' /zxcv',
            status: 'INCOMPLETE',
            message: '\'/zxcv\' doesn\'t exist'
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile rm /mach_kernel',
      check: {
        input:  'tsfile rm /mach_kernel',
        hints:                        '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVV',
        cursor: 22,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile rm' },
          p1: {
            value: '/mach_kernel',
            arg: ' /mach_kernel',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};

exports.testFirefoxBasic = function(options) {
  return helpers.audit(options, [
    {
      
      
      skipRemainingIf: true,
      
      skipIf: true,
      setup:    'tsfile open /',
      check: {
        input:  'tsfile open /',
        hints:               '',
        markup: 'VVVVVVVVVVVVI',
        cursor: 13,
        current: 'p1',
        status: 'ERROR',
        message: '\'/\' is not a file',
        args: {
          command: { name: 'tsfile open' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' is not a file'
          }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile open /zxcv',
      check: {
        input:  'tsfile open /zxcv',
        
        markup: 'VVVVVVVVVVVVIIIII',
        cursor: 17,
        current: 'p1',
        status: 'ERROR',
        message: '\'/zxcv\' doesn\'t exist',
        args: {
          command: { name: 'tsfile open' },
          p1: {
            value: undefined,
            arg: ' /zxcv',
            status: 'INCOMPLETE',
            message: '\'/zxcv\' doesn\'t exist'
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile open /mach_kernel',
      check: {
        input:  'tsfile open /mach_kernel',
        hints:                          '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 24,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile open' },
          p1: {
            value: '/mach_kernel',
            arg: ' /mach_kernel',
            status: 'VALID',
            message: ''
           }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile saveas /',
      check: {
        input:  'tsfile saveas /',
        hints:                 '',
        markup: 'VVVVVVVVVVVVVVI',
        cursor: 15,
        current: 'p1',
        status: 'ERROR',
        message: '\'/\' already exists',
        args: {
          command: { name: 'tsfile saveas' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' already exists'
          }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile saveas /zxcv',
      check: {
        input:  'tsfile saveas /zxcv',
        
        markup: 'VVVVVVVVVVVVVVVVVVV',
        cursor: 19,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile saveas' },
          p1: {
            value: '/zxcv',
            arg: ' /zxcv',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile saveas /mach_kernel',
      check: {
        input:  'tsfile saveas /mach_kernel',
        hints:                            '',
        markup: 'VVVVVVVVVVVVVVIIIIIIIIIIII',
        cursor: 26,
        current: 'p1',
        status: 'ERROR',
        message: '\'/mach_kernel\' already exists',
        args: {
          command: { name: 'tsfile saveas' },
          p1: {
            value: undefined,
            arg: ' /mach_kernel',
            status: 'INCOMPLETE',
            message: '\'/mach_kernel\' already exists'
          }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile save /',
      check: {
        input:  'tsfile save /',
        hints:               '',
        markup: 'VVVVVVVVVVVVI',
        cursor: 13,
        current: 'p1',
        status: 'ERROR',
        message: '\'/\' is not a file',
        args: {
          command: { name: 'tsfile save' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' is not a file'
          }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile save /zxcv',
      check: {
        input:  'tsfile save /zxcv',
        
        markup: 'VVVVVVVVVVVVVVVVV',
        cursor: 17,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile save' },
          p1: {
            value: '/zxcv',
            arg: ' /zxcv',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile save /mach_kernel',
      check: {
        input:  'tsfile save /mach_kernel',
        hints:                          '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVVVV',
        cursor: 24,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile save' },
          p1: {
            value: '/mach_kernel',
            arg: ' /mach_kernel',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tsfile cd /',
      check: {
        input:  'tsfile cd /',
        hints:             '',
        markup: 'VVVVVVVVVVV',
        cursor: 11,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile cd' },
          p1: {
            value: '/',
            arg: ' /',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      setup:    'tsfile cd /zxcv',
      check: {
        input:  'tsfile cd /zxcv',
        
        
        cursor: 15,
        current: 'p1',
        
        message: '\'/zxcv\' doesn\'t exist',
        args: {
          command: { name: 'tsfile cd' },
          p1: {
            value: undefined,
            arg: ' /zxcv',
            
            message: '\'/zxcv\' doesn\'t exist'
          }
        }
      }
    },
    {
      skipIf: true || !local,
      setup:    'tsfile cd /etc/passwd',
      check: {
        input:  'tsfile cd /etc/passwd',
        hints:                       ' -> /etc/pam.d/',
        markup: 'VVVVVVVVVVIIIIIIIIIII',
        cursor: 21,
        current: 'p1',
        status: 'ERROR',
        message: '\'/etc/passwd\' is not a directory',
        args: {
          command: { name: 'tsfile cd' },
          p1: {
            value: undefined,
            arg: ' /etc/passwd',
            status: 'INCOMPLETE',
            message: '\'/etc/passwd\' is not a directory'
          }
        }
      }
    },
    {
      setup:    'tsfile mkdir /',
      check: {
        input:  'tsfile mkdir /',
        hints:                '',
        markup: 'VVVVVVVVVVVVVI',
        cursor: 14,
        current: 'p1',
        status: 'ERROR',
        message: ''/' already exists',
        args: {
          command: { name: 'tsfile mkdir' },
          p1: {
            value: undefined,
            arg: ' /',
            status: 'INCOMPLETE',
            message: '\'/\' already exists'
          }
        }
      }
    },
    {
      setup:    'tsfile mkdir /zxcv',
      check: {
        input:  'tsfile mkdir /zxcv',
        
        markup: 'VVVVVVVVVVVVVVVVVV',
        cursor: 18,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile mkdir' },
          p1: {
            value: '/zxcv',
            arg: ' /zxcv',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile mkdir /mach_kernel',
      check: {
        input:  'tsfile mkdir /mach_kernel',
        hints:                           '',
        markup: 'VVVVVVVVVVVVVIIIIIIIIIIII',
        cursor: 25,
        current: 'p1',
        status: 'ERROR',
        message: '\'/mach_kernel\' already exists',
        args: {
          command: { name: 'tsfile mkdir' },
          p1: {
            value: undefined,
            arg: ' /mach_kernel',
            status: 'INCOMPLETE',
            message: '\'/mach_kernel\' already exists'
          }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile rm /',
      check: {
        input:  'tsfile rm /',
        hints:             '',
        markup: 'VVVVVVVVVVV',
        cursor: 11,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile rm' },
          p1: {
            value: '/',
            arg: ' /',
            status: 'VALID',
            message: ''
          }
        }
      }
    },
    {
      skipIf: true,
      setup:    'tsfile rm /zxcv',
      check: {
        input:  'tsfile rm /zxcv',
        
        markup: 'VVVVVVVVVVIIIII',
        cursor: 15,
        current: 'p1',
        status: 'ERROR',
        message: '\'/zxcv\' doesn\'t exist',
        args: {
          command: { name: 'tsfile rm' },
          p1: {
            value: undefined,
            arg: ' /zxcv',
            status: 'INCOMPLETE',
            message: '\'/zxcv\' doesn\'t exist'
          }
        }
      }
    },
    {
      skipIf: !local,
      setup:    'tsfile rm /mach_kernel',
      check: {
        input:  'tsfile rm /mach_kernel',
        hints:                        '',
        markup: 'VVVVVVVVVVVVVVVVVVVVVV',
        cursor: 22,
        current: 'p1',
        status: 'VALID',
        message: '',
        args: {
          command: { name: 'tsfile rm' },
          p1: {
            value: '/mach_kernel',
            arg: ' /mach_kernel',
            status: 'VALID',
            message: ''
          }
        }
      }
    }
  ]);
};
