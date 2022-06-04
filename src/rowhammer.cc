#include "rowhammer.h"

namespace dramsim3{

std::string Graphene(std::string config_file, std::string trace_file) {
    Config cfg = Config(config_file, "./");
    std::ifstream trace = std::ifstream(trace_file);
    // Assumptions
    float tREFW_time = 64000000; // reset window in nanosec
    unsigned int T_rh = 50000;            // Rowhammer threshold

    // Calculating W
    unsigned int tREFW = tREFW_time / cfg.tCK;                   // reset window in clock cycles
    unsigned int W = tREFW * (1 - cfg.tRFC/cfg.tREFI) / cfg.tRC; // max number of ACTs in a reset window

    // Graphene
    int T = T_rh / 4;           // table threshold
    unsigned int N_entry = W / T + 1;    // number of entries in the table
    std::cout << "Reset Window in clock cycles: " << tREFW << "\n";
    std::cout << "Number of table entries: " << N_entry << "\n";
    std::cout << "Threshold: " << T << "\n";

    std::string output_file = trace_file + "_protected";
    std::ofstream output = std::ofstream(output_file);

    std::map<int, int> table;
    table[-1] = 0;          // spillover counter

    unsigned int TRR_count = 0;

    unsigned int clock_cycle;
    std::string str_addr, command;
    while (trace >> str_addr >> command >> clock_cycle) {
        output << str_addr << " " << command << " " << clock_cycle << "\n";

        uint64_t hex_addr;
        std::stringstream temp;
        temp << std::hex << str_addr;
        temp >> hex_addr;
        Address addr = cfg.AddressMapping(hex_addr);

        unsigned int last_upd = 0;
        if (tREFW+last_upd < clock_cycle) {                 // reset table
            table.clear();
            table[-1] = 0;
            table[addr.row] = 1;
            last_upd = clock_cycle/tREFW * tREFW;
        }
        else {
            if (table.find(addr.row) != table.end()) {      // row in table
                table[addr.row] += 1;
            }
            else {                                          // row not in table
                if (table.size() < N_entry)
                    table[addr.row] = 1;
                else {
                    table[-1] += 1;

                    int min_key = find_min(table);  // finding min element in the map
                    if (min_key != -1) {            // spillover counter > table row counter
                        // swapping spillover counter and table row
                        int temp_val = table[min_key];
                        table.erase(min_key);
                        table[addr.row] = table[-1];
                        table[-1] = temp_val;
                    }
                }
            }

            // check if counter is above the threshold
            if (table[addr.row] >= T) {
                std::cout << "ATTACK DETECTED. AGRESSOR ROW: " << addr.row << ". REFRESH COMMENCING \n";

                output << gethexaddress(addr.row-1, cfg) << " TRR " << clock_cycle + 1 << "\n"; 
                output << gethexaddress(addr.row+1, cfg) << " TRR " << clock_cycle + 2 << "\n";
                table[addr.row] = 0;
                TRR_count+=2;
            }
        }
    }

    trace.close();
    output.close();

    std::cout << "Total # of TRR: " << TRR_count << "\n";

    return output_file;
}


std::string Graphene_improved(std::string config_file, std::string trace_file) {
    Config cfg = Config(config_file, "./");
    std::ifstream trace = std::ifstream(trace_file);
    // Assumptions
    float tREFW_time = 64000000; // reset window in nanosec
    unsigned int T_rh = 50000;            // Rowhammer threshold

    // Calculating W
    unsigned int tREFW = tREFW_time / cfg.tCK;                   // reset window in clock cycles
    unsigned int W = tREFW * (1 - cfg.tRFC/cfg.tREFI) / cfg.tRC; // max number of ACTs in a reset window

    // Graphene
    int T = T_rh / 4;           // table threshold
    unsigned int N_entry = W / T + 1;    // number of entries in the table
    std::cout << "Reset Window in clock cycles: " << tREFW << "\n";
    std::cout << "Number of table entries: " << N_entry << "\n";
    std::cout << "Threshold: " << T << "\n";

    std::string output_file = trace_file + "_protected";
    std::ofstream output = std::ofstream(output_file);

    std::map<int, int> table;
    table[-1] = 0;          // spillover counter

    unsigned int TRR_count = 0;

    unsigned int clock_cycle;
    std::string str_addr, command;
    while (trace >> str_addr >> command >> clock_cycle) {
        output << str_addr << " " << command << " " << clock_cycle << "\n";

        uint64_t hex_addr;
        std::stringstream temp;
        temp << std::hex << str_addr;
        temp >> hex_addr;
        Address addr = cfg.AddressMapping(hex_addr);

        unsigned int last_upd = 0;
        if (tREFW+last_upd < clock_cycle) {                 // reset table
            table.clear();
            table[-1] = 0;
            table[addr.row] = 1;
            last_upd = clock_cycle/tREFW * tREFW;
        }
        else {
            if (table.find(addr.row) != table.end()) {      // row in table
                table[addr.row] += 1;
            }
            else {                                          // row not in table
                if (table.size() < N_entry)
                    table[addr.row] = 1;
                else {
                    table[-1] += 1;

                    int min_key = find_min(table);  // finding min element in the map
                    if (min_key != -1) {            // spillover counter > table row counter
                        // swapping spillover counter and table row
                        int temp_val = table[min_key];
                        table.erase(min_key);
                        table[addr.row] = table[-1];
                        table[-1] = temp_val;
                    }
                }
            }

            // check if counter is above the threshold
            if (table[addr.row] >= T) {
                std::cout << "ATTACK DETECTED. AGRESSOR ROW: " << addr.row << ". REFRESH COMMENCING \n";

                output << gethexaddress(addr.row-1, cfg) << " TRR " << clock_cycle + 1 << "\n"; 
                output << gethexaddress(addr.row+1, cfg) << " TRR " << clock_cycle + 2 << "\n";
                table[addr.row] = 0;
                TRR_count+=2;
            }
        }
    }

    trace.close();
    output.close();

    std::cout << "Total # of TRR: " << TRR_count << "\n";

    return output_file;
}



int find_min(std::map<int, int> table) {
    int key = -1;
    for (auto i: table)
        if (i.second < table[key])
            key = i.first;
    return key;
}

std::string gethexaddress(int row, Config cfg){
    int total_shift_bits = cfg.shift_bits + cfg.ro_pos;
    unsigned int row_unsigned = (unsigned int) row;
    unsigned int row_shifted = row_unsigned << total_shift_bits;
    static const char* digits = "0123456789ABCDEF";
    std::string rc(8,'0');
    for (size_t i=0, j=28 ; i<8; ++i,j-=4)
        rc[i] = digits[(row_shifted>>j) & 0x0f];
    return rc;
}



}