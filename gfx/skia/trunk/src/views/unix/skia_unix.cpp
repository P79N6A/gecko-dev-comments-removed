






#include "SkApplication.h"
#include "SkEvent.h"
#include "SkWindow.h"

int main(int argc, char** argv){
    SkOSWindow* window = create_sk_window(NULL, argc, argv);

    
    while (SkEvent::ProcessEvent());

    
    application_init();

    window->loop();

    delete window;
    application_term();
    return 0;
}
