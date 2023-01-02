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
            for (size_t opi = 0; opi < inst->get_operands().size() / 2; opi++)
            {
                auto val = inst->get_operands()[2 * opi];
                auto label = inst->get_operands()[2 * opi + 1];
                if (dynamic_cast<Constant *>(val) || dynamic_cast<Instruction *>(val) || dynamic_cast<Argument *>(val))
                {
                    for (auto bb : ((BasicBlock *)label)->get_rdom_frontier())
                    {
                        for (auto j : bb->get_instructions())
                        {
                            if (j->is_br())
                            {
                                mark_insts.insert(j);
                                worklist.insert(j);
                                break;
                            }
                        }
                    }
                }
            }
        }
        auto rdf = inst->get_parent()->get_rdom_frontier();
        for (auto bb : rdf) {
            for(auto br_inst : bb->get_instructions())
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
            auto func_bb = func->get_basic_blocks();
            for (auto bb: func_bb) {
                auto bb_insts = bb->get_instructions();
                for (auto inst: bb_insts) {
                    if (mark_insts.find(inst) == mark_insts.end()) {
                        if (inst->is_br() && dynamic_cast<BranchInst*>(inst)->is_cond_br()) {
                            std::queue<BasicBlock*> bb_queue;
                            std::map<BasicBlock *, std::set<Value *> > lastoftoBB;
                            BasicBlock* n_bb;
                            std::map<BasicBlock*, bool> vs;
                            bool flag = false;
                            for (auto s_bb : inst->get_parent()->get_succ_basic_blocks()) {
                                bb_queue.push(s_bb);
                            }
                            while (!bb_queue.empty()) {
                                auto f = bb_queue.front();
                                bb_queue.pop();
                                if(!vs[f]){
                                    vs[f] = true;
                                    if (bb->get_rdoms().find(f) != bb->get_rdoms().end()) {
                                        if (!flag) {
                                            n_bb = f;
                                            if(bb->get_name() == "label8")
                                            std::cout << "nbb: " << n_bb->get_name()  << std::endl;
                                            flag = true;
                                        }
                                    }
                                    for (auto s_bb : f->get_succ_basic_blocks()) {
                                        lastoftoBB[s_bb].insert(f);
                                        if(s_bb->get_name() == "label19")
                                            std::cout << "ltb: " << f->get_name()  << std::endl;
                                        bb_queue.push(s_bb);
                                    }
                                }
                            }
                            auto bb_v = bb->get_succ_basic_blocks();
                            for (auto s_bb : bb_v) {
                                s_bb->remove_pre_basic_block(bb);
                                bb->remove_succ_basic_block(s_bb);
                            }
                            bb->add_succ_basic_block(n_bb);
                            //n_bb->add_pre_basic_block(bb);
                            BranchInst::create_br(n_bb, bb);
                            auto r_tree = new RDominateTree(module);
                            r_tree->execute();
                                         for (auto inst : n_bb->get_instructions()){
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
                                                    if(lastoftoBB[n_bb].find(label) != lastoftoBB[n_bb].end())
                                                        inst->set_operand(2*i+1, bb);
                                                }
                                            }
                                         }
                            
                            bb->delete_instr(inst);

                        } else if(!inst->is_br()) {
                            bb->delete_instr(inst);
                        }
                    }
                }
            }
        }
    }
}
