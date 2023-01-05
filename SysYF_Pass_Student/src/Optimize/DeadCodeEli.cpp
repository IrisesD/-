#include "DeadCodeEli.h"
#include <RDominateTree.h>

void DeadCodeEli::execute() {
    // Start from here!
    module->set_print_name();
    auto r_tree = new RDominateTree(module);
    r_tree->execute();
    mark();
    sweep();
    Clean();
    delete_bb();
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
        if (inst->is_phi()){
            for (int i = 0; i < inst->get_num_operand(); i++){
                if(i % 2 == 0){
                    auto val = inst->get_operand(i);
                    auto label = inst->get_operand(i+1);
                    if (dynamic_cast<Constant *>(val) || dynamic_cast<Instruction *>(val) || dynamic_cast<Argument *>(val)){
                        for (auto bb : ((BasicBlock *)label)->get_rdom_frontier()){
                            if (bb->get_terminator()->is_br()){
                                mark_insts.insert(bb->get_terminator());
                                worklist.insert(bb->get_terminator());
                            }
                        }
                    }
                }
            }
        }
        auto rdf = inst->get_parent()->get_rdom_frontier();
        for (auto bb : rdf) {
            if (bb->get_terminator()->is_br()) {
                if (mark_insts.find(bb->get_terminator()) == mark_insts.end()) {
                    mark_insts.insert(bb->get_terminator());
                    worklist.insert(bb->get_terminator());
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
                            //bfs to find the nearest postdom
                            std::queue<BasicBlock*> bb_queue;
                            std::map<BasicBlock *, std::set<Value *> > direct_pre_bb;
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
                                        direct_pre_bb[s_bb].insert(f);
                                        bb_queue.push(s_bb);
                                    }
                                }
                            }
                            auto bb_v = bb->get_succ_basic_blocks();
                            for (auto s_bb : bb_v) {
                                s_bb->remove_pre_basic_block(bb);
                                bb->remove_succ_basic_block(s_bb);
                            }
                            bb->add_succ_basic_block(nearest_bb);
                            //nearest_bb->add_pre_basic_block(bb);
                            BranchInst::create_br(nearest_bb, bb);
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
                                        if(direct_pre_bb[nearest_bb].find(label) != direct_pre_bb[nearest_bb].end()){
                                            inst->set_operand(2*i+1, bb);
                                        }
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

void DeadCodeEli::delete_bb() {
    for (auto func : this->module->get_functions()) {
        if (func->get_basic_blocks().empty())
            continue;
        std::map<BasicBlock*, bool> marked;
        std::queue<BasicBlock*> worklist;
        worklist.push(func->get_entry_block());
        while (!worklist.empty()) {
            auto front_bb = worklist.front();
            worklist.pop();
            if (!marked[front_bb]){
                marked[front_bb] = true;
                for (auto next: front_bb->get_succ_basic_blocks())
                    worklist.push(next);
            }
        }
        auto BB = func->get_basic_blocks();
        for (auto bb : BB){
            if (!marked[bb]){
                for (auto succ : bb->get_succ_basic_blocks())
                    succ->remove_pre_basic_block(bb);
                func->get_basic_blocks().remove(bb);
            }
        }
    }
}

void DeadCodeEli::Clean() {
    while (Onepass());
}

bool DeadCodeEli::Case3(BasicBlock* bb){
    return (bb->get_succ_basic_blocks().size() == 1 && bb->get_succ_basic_blocks().front()->get_pre_basic_blocks().size() == 1);
}

bool DeadCodeEli::Case4(BasicBlock* bb, BasicBlock* pre_bb){
    return (bb->get_succ_basic_blocks().size() > 1 && !dynamic_cast<BranchInst*>(pre_bb->get_terminator())->is_cond_br());
}

bool DeadCodeEli::Onepass() {
    bool is_change = false;
    for (auto func : this->module->get_functions()) {
        if (func->get_basic_blocks().empty())
            continue;
        for (auto bb : func->get_basic_blocks()) {
            if(bb->get_name() == "label_entry" || bb->get_name() == "label_ret" || bb->get_instructions().size() > 1)
                continue;
            if(bb->get_pre_basic_blocks().size() == 1 ){
                auto pre_bb = bb->get_pre_basic_blocks().front();
                auto suc_bb = bb->get_succ_basic_blocks().front();
                if(Case3(bb)){
                
                    pre_bb->remove_succ_basic_block(bb);
                    bb->remove_pre_basic_block(pre_bb);

                    suc_bb->remove_pre_basic_block(bb);
                    bb->remove_succ_basic_block(suc_bb);

                    pre_bb->add_succ_basic_block(suc_bb);
                    suc_bb->add_pre_basic_block(pre_bb);
                    
                    auto tt_pre_bb_inst = pre_bb->get_terminator();
                    if (dynamic_cast<BranchInst *>(tt_pre_bb_inst)->is_cond_br()){
                        if (tt_pre_bb_inst->get_operand(1) == bb){
                            tt_pre_bb_inst->set_operand(1, suc_bb);
                        }
                        else if(tt_pre_bb_inst->get_operand(2) == bb){
                            tt_pre_bb_inst->set_operand(2, suc_bb);
                        }
                    }
                    else{
                        tt_pre_bb_inst->set_operand(0, suc_bb);
                    }
                    ModifyPhi(suc_bb, bb, pre_bb);
                    is_change = true;
                }
                else if(Case4(bb, pre_bb)){
                    auto t_pre_bb_inst = pre_bb->get_terminator();
                    pre_bb->delete_instr(t_pre_bb_inst);
                    pre_bb->add_instruction(bb->get_terminator());

                    pre_bb->remove_succ_basic_block(bb);
                    bb->remove_pre_basic_block(pre_bb);
                    
                    for (auto suc_bb : bb->get_succ_basic_blocks())
                    {
                        suc_bb->remove_pre_basic_block(bb);
                        bb->remove_succ_basic_block(suc_bb);
                        pre_bb->add_succ_basic_block(suc_bb);
                        suc_bb->add_pre_basic_block(pre_bb);
                    }
                    ModifyPhi(suc_bb, bb, pre_bb);
                    is_change = true;
                }
            }
        }
    }
    return is_change;
}
void DeadCodeEli::ModifyPhi(BasicBlock* suc_bb, BasicBlock* bb, BasicBlock* pre_bb){
    for (auto inst : suc_bb->get_instructions()){
        if (inst->is_phi()){
            for (int i = 0; i < inst->get_num_operand(); i++){
                if(i % 2 == 0){
                    if (inst->get_operand(i + 1) == bb){
                        auto val = inst->get_operand(i);
                        inst->remove_operands(i, i + 1);
                        inst->add_operand(val);
                        inst->add_operand(pre_bb);
                        break;
                    }
                }
            }
        }
    }
}
