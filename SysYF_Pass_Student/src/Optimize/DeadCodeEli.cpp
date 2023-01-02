#include "DeadCodeEli.h"
#include <RDominateTree.h>

void DeadCodeEli::execute() {
    // Start from here!
    module->set_print_name();
    auto r_tree = new RDominateTree(module);
    r_tree->execute();
    mark();
    sweep();
}

bool DeadCodeEli::is_critical(Instruction *inst) {
    if (inst->is_call() || inst->is_store() || inst->is_ret()) {
        return true;
    }
    return false;
}

void DeadCodeEli::mark() {
    std::set<Instruction*> worklist;
    mark_insts.clear();
    for (auto &func: this->module->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            for (auto bb: func->get_basic_blocks()) {
                for (auto inst: bb->get_instructions()) {
                    if (is_critical(inst)) {
                        mark_insts.insert(inst);
                        worklist.insert(inst);
                    }
                }
            }
        }
    }
    while (!worklist.empty()) {
        auto it = worklist.begin();
        auto inst = *it;
        worklist.erase(it);
        for (auto op : inst->get_operands()) {
            auto def_inst = dynamic_cast<Instruction*>(op);
            if (mark_insts.find(def_inst) == mark_insts.end()) {
                mark_insts.insert(def_inst);
                worklist.insert(def_inst);
            }
        }
        auto bb_inst = dynamic_cast<BasicBlock*>(inst);
        auto rdf = bb_inst->get_rdom_frontier();
        for (auto bb : rdf) {
            auto br_inst = bb->get_terminator();
            if (br_inst->is_br()) {
                if (mark_insts.find(br_inst) == mark_insts.end()) {
                    mark_insts.insert(br_inst);
                    worklist.insert(br_inst);
                }
            }
        }
    }
}

void DeadCodeEli::sweep() {
    for (auto &func: this->module->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            for (auto bb: func->get_basic_blocks()) {
                for (auto inst: bb->get_instructions()) {
                    if (mark_insts.find(inst) == mark_insts.end()) {
                        if (inst->is_br()) {
                            std::queue<BasicBlock*> bb_queue;
                            BasicBlock* n_bb;
                            bool flag = false;
                            for (auto s_bb : inst->get_parent()->get_succ_basic_blocks()) {
                                bb_queue.push(s_bb);
                            }
                            while (!bb_queue.empty()) {
                                auto f = bb_queue.front();
                                bb_queue.pop();
                                if (bb->get_rdoms().find(f) != bb->get_rdoms().end()) {
                                    if (!flag) {
                                        n_bb = f;
                                        flag = true;
                                        break;
                                    }
                                }
                                for (auto s_bb : f->get_succ_basic_blocks()) {
                                    bb_queue.push(s_bb);
                                }
                            }
                            auto bb_v = bb->get_succ_basic_blocks();
                            for (auto s_bb : bb_v) {
                                s_bb->remove_pre_basic_block(bb);
                                bb->remove_succ_basic_block(s_bb);
                            }
                            bb->add_succ_basic_block(n_bb);
                            n_bb->add_pre_basic_block(bb);
                            BranchInst::create_br(n_bb, bb);
                            bb->delete_instr(inst);
                        } else {
                            bb->delete_instr(inst);
                        }
                    }
                }
            }
        }
    }
}