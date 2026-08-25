#include "application/application.hpp"

int main()
{
    Application app(5555, 30, 5000, 50000);
    app.detection_loop("../logs/server.log");

    return 0;
}