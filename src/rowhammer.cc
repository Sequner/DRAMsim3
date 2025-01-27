#include "rowhammer.h"

namespace dramsim3{

RowhammerCounter::RowhammerCounter(std::string config_file, 
                                   std::string trace_file,
                                   float tREFW_ns, unsigned int T_rh)
    : cfg_(config_file, "./"),
      trace_(trace_file),
      output_(trace_file + "_protected"),
      T_rh_(T_rh) {
          std::cout << "Reset Window in nanoseconds: " << tREFW_ns << "\n";
          tREFW_ = tREFW_ns / cfg_.tCK;
          std::cout << "Reset Window in clock cycles: " << tREFW_ << "\n";
          W_ = tREFW_ * (1 - cfg_.tRFC/cfg_.tREFI) / cfg_.tRC;
          std::cout << "Max number of ACTs per Reset Window: " << W_ << "\n";
          output_name = trace_file + "_protected";
      }

RowhammerCounter::~RowhammerCounter() {
    output_.close();
    trace_.close();
}

Graphene::Graphene(std::string config_file,
                   std::string trace_file,
                   float tREFW_ns, unsigned int T_rh)
    : RowhammerCounter(config_file, trace_file, tREFW_ns, T_rh) {
        T_ = T_rh / 4;
        CreateTable();
    }

void Graphene::CreateTable() {
    N_entries_ = W_ / T_ + 1;
    table_[-1] = 0;
}

void Graphene::ResetTable(int row) {
    table_.clear();
    table_[-1] = 0;
    table_[row] = 1;
}

void Graphene::GenerateRefresh(int row, unsigned int clock_cycle) {
    output_ << "0x" << gethexaddress(row, cfg_) << " TRR " << clock_cycle << "\n";
}

void Graphene::ProcessTransaction(int row, unsigned int clock_cycle) {
    UpdateTable(row);
    // check if counter is above the threshold
    if (table_[row] >= T_) {
        std::cout << "ATTACK DETECTED. AGRESSOR ROW: " << row << ". REFRESH COMMENCING \n";
        GenerateRefresh(row-1, clock_cycle+1);
        GenerateRefresh(row+1, clock_cycle+2);
        table_[row] = 0;
    }   
}

void Graphene::UpdateTable(int row) {
    if (table_.find(row) != table_.end()) {
        table_[row] += 1;
    }
    else {
        if (table_.size() < N_entries_)
            table_[row] = 1;
        else {
            table_[-1] += 1;

            int min_key = find_min(table_);  // finding min element in the map
            if (min_key != -1) {            // spillover counter > table row counter
                // swapping spillover counter and table row
                int temp_val = table_[min_key];
                table_.erase(min_key);
                table_[row] = table_[-1];
                table_[-1] = temp_val;
            }
        }
    }
}

std::string Graphene::TraverseTrace() {
    std::cout << "Threshold: " << T_ << "\n";
    std::cout << "Number of table entries: " << N_entries_ << "\n";
    std::string str_addr, command;
    unsigned int clock_cycle;
    unsigned int last_upd = 0;

    while (trace_ >> str_addr >> command >> clock_cycle) {
        output_ << str_addr << " " << command << " " << clock_cycle << "\n";

        uint64_t hex_addr;
        std::stringstream temp;
        temp << std::hex << str_addr;
        temp >> hex_addr;
        Address addr = cfg_.AddressMapping(hex_addr);

        
        if (tREFW_+last_upd <= clock_cycle) {
            ResetTable(addr.row);
            last_upd = clock_cycle / tREFW_ * tREFW_;
        }
        else {
            ProcessTransaction(addr.row, clock_cycle);
        }
    }

    return output_name;
}

int Graphene::find_min(std::map<int, int> table) {
    int key = -1;
    for (auto i: table)
        if (i.second < table[key])
            key = i.first;
    return key;
}

std::string RowhammerCounter::gethexaddress(int row, Config cfg) {
    int total_shift_bits = cfg.shift_bits + cfg.ro_pos;
    unsigned int row_unsigned = (unsigned int) row;
    unsigned int row_shifted = row_unsigned << total_shift_bits;
    static const char* digits = "0123456789ABCDEF";
    std::string rc(8,'0');
    for (size_t i=0, j=28 ; i<8; ++i,j-=4)
        rc[i] = digits[(row_shifted>>j) & 0x0f];
    return rc;
}

GrapheneII::GrapheneII(std::string config_file,
                   std::string trace_file,
                   float tREFW_ns, unsigned int T_rh)
    : Graphene(config_file, trace_file, tREFW_ns, T_rh) {
        T_ = T_rh / 2;
        CreateTable();
}

void GrapheneII::ResetRow(int row) {
    if (table_.find(row) != table_.end()) {
        table_[row] = 0;
    }
}

void GrapheneII::CreateTable() {
    N_entries_ = W_ * 2 / T_ + 1; // 2 is the number of adjacent rows affected by ACT
    table_[-1] = 0;
}

void GrapheneII::ProcessTransaction(int row, unsigned int clock_cycle) {
    ResetRow(row);

    for (int i=-1; i < 2; i += 2) {
        UpdateTable(row+i);

        // check if counter is above the threshold
        if (table_[row+i] >= T_) {
            std::cout << "ATTACK DETECTED. VICTIM ROW: " << row+i << ". REFRESH COMMENCING \n";
            GenerateRefresh(row+i, clock_cycle+1);
            table_[row+i] = 0;
        }
    }
}

}