#ifndef SYSYF_DEADCODEELI_H
#define SYSYF_DEADCODEELI_H

#include "Pass.h"
#include "Module.h"
#include <queue>

class DeadCodeEli : public Pass
{
public:
    DeadCodeEli(Module *module) : Pass(module) {}
    void execute() final;
    const std::string get_name() const override {return name;}
    bool is_critical(Instruction* inst);
    void mark();
    void sweep();
private:
    Function *func_;
    std::set<Instruction*> mark_insts;
    const std::string name = "DeadCodeElimination";
};

#endif  // SYSYF_DEADCODEELI_H
