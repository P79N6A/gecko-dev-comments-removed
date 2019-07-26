









ALIVE="Letov is alive"

function __cont(){

    function __func(){
        return ALIVE;
    };
    
    
    
    if (delete __func) {
    	$ERROR('#1: delete __func returning false');
    }
    
    
    
    
    
    if (__func() !== ALIVE) {
    	$ERROR('#2: __func() === ALIVE. Actual: __func() ==='+__func());
    }
    
    
};

__cont();

