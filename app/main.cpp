#include "app.h"

int main()
{
    App app;
    if (!app.init()) {
        return 1;
    }
    
    app.main_loop();

    return 0;
}