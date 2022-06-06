#ifndef __ROWHAMMER_H
#define __ROWHAMMER_H

#include <iostream>
#include <sstream>
#include "configuration.h"
#include <map>

namespace dramsim3{

class RowhammerCounter {
    public:
        RowhammerCounter(std::string config_file, std::string trace_file, 
                        float tREFW_ns, unsigned int T_rh);
        ~RowhammerCounter();
        virtual void CreateTable(){}
        virtual std::string TraverseTrace(){return output_name;}
        virtual void ProcessTransaction(int row){}
        virtual void UpdateTable(int row){}
        std::string gethexaddress(int row, Config cfg);
        std::string output_name;
    protected:
        Config cfg_;
        std::ifstream trace_;
        std::ofstream output_;
        unsigned int tREFW_;
        unsigned int T_rh_;
        unsigned int W_;
        unsigned int N_entries_;
        std::map<int, int> table_;
};

class Graphene : public RowhammerCounter {
    public:
        Graphene(std::string config_file, std::string trace_file,
                float tREFW_ns, unsigned int T_rh);
        void CreateTable() override;
        std::string TraverseTrace() override;
        void ProcessTransaction(int row) override;
        void UpdateTable(int row) override;
        void CheckThresholds(int row, int clock_cycle);
        void GenerateRefresh(int row, unsigned int clock_cycle);
        void ResetTable(int row);
        int find_min(std::map<int, int> table);
    
    protected:
        int T_;
};

}
#endif