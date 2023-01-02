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
            if(def_inst){
                if (mark_insts.find(def_inst) == mark_insts.end()) {
                    mark_insts.insert(def_inst);
                    worklist.insert(def_inst);
                }
            }
        }
        if (inst->is_phi())
        {
            std::vector<std::pair<Value*, BasicBlock*> >pair;
            auto hss = static_cast<PhiInst *>(inst)->get_operands();
            for (int i = 0; i < hss.size(); i++) {
                if (i % 2 == 0) {
                    pair.push_back({hss[i], (BasicBlock*)hss[i+1]});
                }
            }
            for(int i = 0; i < pair.size(); i++){
                auto value = pair[i].first;
                auto label = pair[i].second;
                if (dynamic_cast<Constant *>(value) || dynamic_cast<Instruction *>(value) || dynamic_cast<Argument *>(value)){
                    for (auto bb : label->get_rdom_frontier()){
                        auto term_inst = bb->get_terminator();
                        if (term_inst->is_br()){
                            mark_insts.insert(term_inst);
                            worklist.insert(term_inst);
                        }
                    }
                }
            }
        }
        auto rdf = inst->get_parent()->get_rdom_frontier();
        for (auto bb : rdf) {
            auto term_inst = bb->get_terminator();
            if (term_inst->is_br()){
                mark_insts.insert(term_inst);
                worklist.insert(term_inst);
            }
        }
    }
}

void DeadCodeEli::sweep() {
    for (auto &func: this->module->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            auto func_bb = func->get_basic_blocks();
            for (auto bb: func_bb) {
                auto bb_insts = bb->get_instructions();
                for (auto inst: bb_insts) {
                    if (mark_insts.find(inst) == mark_insts.end()) {
                        if (inst->is_br() && dynamic_cast<BranchInst*>(inst)->is_cond_br()) {
                            //bfs to find nearest postdom
                            std::queue<BasicBlock*> bb_queue;
                            std::map<BasicBlock *, std::set<Value *> > direct_preds_bb;
                            BasicBlock* nearest_bb;
                            std::map<BasicBlock*, bool> visited;
                            bool flag = false;
                            for (auto s_bb : inst->get_parent()->get_succ_basic_blocks()) {
                                bb_queue.push(s_bb);
                            }
                            while (!bb_queue.empty()) {
                                auto f = bb_queue.front();
                                bb_queue.pop();
                                if(!visited[f]){
                                    visited[f] = true;
                                    if (bb->get_rdoms().find(f) != bb->get_rdoms().end()) {
                                        if (!flag) {
                                            nearest_bb = f;
                                            flag = true;
                                        }
                                    }
                                    for (auto s_bb : f->get_succ_basic_blocks()) {
                                        direct_preds_bb[s_bb].insert(f);
                                        bb_queue.push(s_bb);
                                    }
                                }
                            }
                            auto bb_suc_list = bb->get_succ_basic_blocks();
                            for (auto s_bb : bb_suc_list) {
                                s_bb->remove_pre_basic_block(bb);
                                bb->remove_succ_basic_block(s_bb);
                            }
                            bb->add_succ_basic_block(nearest_bb);
                            BranchInst::create_br(nearest_bb, bb);
                            //Recompute the RdomTree
                            auto r_tree = new RDominateTree(module);
                            r_tree->execute();

                            for (auto inst : nearest_bb->get_instructions()){
                                //Change Phi Insts
                                if (inst->is_phi()){
                                    std::vector<std::pair<Value*, BasicBlock*> >pair;
                                    auto hss = static_cast<PhiInst *>(inst)->get_operands();
                                    for (int i = 0; i < hss.size(); i++) {
                                        if (i % 2 == 0) {
                                            pair.push_back({hss[i], (BasicBlock*)hss[i+1]});
                                        }
                                    }
                                    for(int i = 0; i < pair.size(); i++){
                                        auto label = pair[i].second;
                                        if(direct_preds_bb[nearest_bb].find(label) != direct_preds_bb[nearest_bb].end())
                                            inst->set_operand(2*i+1, bb);
                                    }
                                }
                            }
                            bb->delete_instr(inst);
                        } 
                        else if(!inst->is_br()) {
                            bb->delete_instr(inst);
                        }
                    }
                }
            }
        }
    }
}