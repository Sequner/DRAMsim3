#include <iostream>
#include <sstream>
#include "configuration.h"
#include <map>

namespace dramsim3{

    int find_min(std::map<int, int> table);
    std::string rowhammer_preprocessing(std::string config_file, std::string tracefile);
    std::string gethexaddress(int row, Config cfg);
    std::string dec2hexa(unsigned int w);

}
