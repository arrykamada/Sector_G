
#include <iostream>

#include "Core/Application.h"
#include "Utils/Log.h"

int main() {
    SectorG::Log::Info("Sector_G starting...");

    {
        SectorG::Application app;
        app.Run();
    }

    SectorG::Log::Info("Sector_G finished");
    return 0;
}