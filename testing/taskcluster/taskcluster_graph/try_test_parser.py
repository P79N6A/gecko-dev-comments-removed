def parse_test_opts(input_str):
    '''
    Test argument parsing is surprisingly complicated with the "restrictions"
    logic this function is responsible for parsing this out into a easier to
    work with structure like { test: '..', platforms: ['..'] }
    '''

    
    tests = []

    cur_test = {}
    token = ''
    in_platforms = False

    def add_test(value):
        cur_test['test'] = value.strip()
        tests.insert(0, cur_test)

    def add_platform(value):
        
        cur_test['platforms'] = cur_test.get('platforms', [])
        cur_test['platforms'].insert(0, value.strip())

    
    
    for char in reversed(input_str):

        
        if char == ',':

            
            if in_platforms:
                add_platform(token)

            
            else:
                add_test(token)
                cur_test = {}

            
            token = ''
        elif char == '[':
            
            add_platform(token)
            token = ''
            in_platforms = False
        elif char == ']':
            
            in_platforms = True
        else:
            
            token = char + token

    
    if token:
        add_test(token)

    return tests
