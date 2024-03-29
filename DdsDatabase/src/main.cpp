/**
 * @file main.cpp
 * @author your name (you@domain.com)
 * @brief 
 * @version 0.1
 * @date 2023-07-03
 * 
 * @copyright Copyright (c) 2023
 * 
 */

#include <iostream>

#include "Database.hpp"
#include "Controller.hpp"

using std::cout;
using std::endl;

int main(int argc, char const *argv[])
{
    cout << "==== Starting Database" << endl;
    Controller *c = new Controller();
    c->initialise_controller();
    return 0;
}
