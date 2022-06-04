#include <iostream>
#include "./../ext/headers/args.hxx"
#include "cpu.h"
#include "configuration.h"
#include <map>

using namespace dramsim3;

int find_min(std::map<int, int> table);
std::string rowhammer_preprocessing(std::string config_file, std::string tracefile);

int main(int argc, const char **argv) {
    args::ArgumentParser parser(
        "DRAM Simulator.",
        "Examples: \n."
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -c 100 -t "
        "sample_trace.txt\n"
        "./build/dramsim3main configs/DDR4_8Gb_x8_3200.ini -s random -c 100");
    args::HelpFlag help(parser, "help", "Display the help menu", {'h', "help"});
    args::ValueFlag<uint64_t> num_cycles_arg(parser, "num_cycles",
                                             "Number of cycles to simulate",
                                             {'c', "cycles"}, 100000);
    args::ValueFlag<std::string> output_dir_arg(
        parser, "output_dir", "Output directory for stats files",
        {'o', "output-dir"}, ".");
    args::ValueFlag<std::string> stream_arg(
        parser, "stream_type", "address stream generator - (random), stream",
        {'s', "stream"}, "");
    args::ValueFlag<std::string> trace_file_arg(
        parser, "trace",
        "Trace file, setting this option will ignore -s option",
        {'t', "trace"});
    args::Positional<std::string> config_arg(
        parser, "config", "The config file name (mandatory)");

    try {
        parser.ParseCLI(argc, argv);
    } catch (args::Help) {
        std::cout << parser;
        return 0;
    } catch (args::ParseError e) {
        std::cerr << e.what() << std::endl;
        std::cerr << parser;
        return 1;
    }

    std::string config_file = args::get(config_arg);
    if (config_file.empty()) {
        std::cerr << parser;
        return 1;
    }

    uint64_t cycles = args::get(num_cycles_arg);
    std::string output_dir = args::get(output_dir_arg);
    std::string trace_file = args::get(trace_file_arg);
    std::string stream_type = args::get(stream_arg);

    trace_file = rowhammer_preprocessing(config_file, trace_file);

    CPU *cpu;
    if (!trace_file.empty()) {
        cpu = new TraceBasedCPU(config_file, output_dir, trace_file);
    } else {
        if (stream_type == "stream" || stream_type == "s") {
            cpu = new StreamCPU(config_file, output_dir);
        } else {
            cpu = new RandomCPU(config_file, output_dir);
        }
    }

    for (uint64_t clk = 0; clk < cycles; clk++) {
        cpu->ClockTick();
    }
    cpu->PrintStats();

    delete cpu;

    return 0;
}

// can create separate file for this, but I am too lazy
std::string rowhammer_preprocessing(std::string config_file, std::string trace_file) {
    Config cfg = Config(config_file, "./");
    std::ifstream trace = std::ifstream(trace_file);
    // Assumptions
    float tREFW_time = 64000000; // reset window in nanosec
    int T_rh = 50000;            // Rowhammer threshold

    // Calculating W
    int tREFW = tREFW_time / cfg.tCK;                   // reset window in clock cycles
    int W = tREFW * (1 - cfg.tRFC/cfg.tREFI) / cfg.tRC; // max number of ACTs in a reset window

    // Graphene
    int T = T_rh / 4;           // table threshold
    int N_entry = W / T + 1;    // number of entries in the table
    std::cout << "Number of table entries: " << N_entry << "\n";
    std::cout << "Threshold: " << T << "\n";

    std::string output_file = trace_file + "p";
    std::ofstream output = std::ofstream(output_file);

    std::map<int, int> table;
    table[-1] = 0;          // spillover counter

    int clock_cycle;
    std::string str_addr, command;
    while (trace >> str_addr >> command >> clock_cycle) {
        output << str_addr << " " << command << " " << clock_cycle << "\n";

        uint64_t hex_addr;
        std::stringstream temp;
        temp << std::hex << str_addr;
        temp >> hex_addr;
        Address addr = cfg.AddressMapping(hex_addr);

        int last_upd = 0;
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
                std::cout << "ATTACK DETECTED. AGRESSOR ROW: " << addr.row << "\n";
                output << str_addr << " TRR " << clock_cycle + 1 << "\n"; // write instr to output for addr.row +- 1
                output << str_addr << " TRR " << clock_cycle + 2 << "\n";
                table[addr.row] = 0;
            }
        }
    }

    trace.close();
    output.close();
    return output_file;
}

int find_min(std::map<int, int> table) {
    int key = -1;
    for (auto i: table)
        if (i.second < table[key])
            key = i.first;
    return key;
}