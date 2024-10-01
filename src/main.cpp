/**
 * Copyright(c) 2024 Reecho inc. All rights reserved.
 */

#include <iostream>
#include "server/world.hpp"
#include "front/window.hpp"

int main()
{
    auto world = CWorld::GetInstance();
    
    world->Initialize();
    world->GetWindow()->Run();

    return 0;
}