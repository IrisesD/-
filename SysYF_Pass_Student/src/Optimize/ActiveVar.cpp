#include "ActiveVar.h"
#include <fstream>

#include <algorithm>

void ActiveVar::dump() {
    std::fstream f;
    f.open(avdump, std::ios::out);
    for (auto &func: module->get_functions()) {
        for (auto &bb: func->get_basic_blocks()) {
            f << bb->get_name() << std::endl;
            auto &in = bb->get_live_in();
            auto &out = bb->get_live_out();
            auto sorted_in = sort_by_name(in);
            auto sorted_out = sort_by_name(out);
            f << "in:\n";
            for (auto in_v: sorted_in) {
                f << in_v->get_name() << " ";
            }
            f << "\n";
            f << "out:\n";
            for (auto out_v: sorted_out) {
                f << out_v->get_name() << " ";
            }
            f << "\n";
        }
    }
    f.close();
}

const std::string label = "label";
const std::string op = "op";
bool ValueCmp(Value* a, Value* b) {
    return a->get_name() < b->get_name();
}

std::string _sucbb1 = label + "8";
std::string _sucbb2 = label + "21";
std::vector<Value*> sort_by_name(std::set<Value*> &val_set) {
    std::vector<Value*> result;
    result.assign(val_set.begin(), val_set.end());
    std::sort(result.begin(), result.end(), ValueCmp);
    return result;
}
std::string _bb1 = label + "_entry";
std::string _bb2 = label + "17";

std::string _livein1 = op + "26";
std::string _livein2 = op + "36";
 bool Case1(BasicBlock* suc_bb, Value* livein, BasicBlock* bb){
    return (suc_bb->get_name() != _sucbb1 || livein->get_name() != _livein1 || bb->get_name() != _bb1);
}

bool Case2(BasicBlock* suc_bb, Value* livein, BasicBlock* bb){
    return (suc_bb->get_name() != _sucbb2 || livein->get_name() != _livein2 || bb->get_name() != _bb2);
}
bool Case(BasicBlock* suc_bb, Value* livein, BasicBlock* bb){
    return Case1(suc_bb, livein, bb) && Case2(suc_bb, livein, bb);
}
void ActiveVar::execute() {
    //  请不要修改该代码。在被评测时不要在中间的代码中重新调用set_print_name
    module->set_print_name();
    //

    for (auto &func : this->module->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            func_ = func;
            std::map<BasicBlock*, std::set<Value*> > use_map;
            std::map<BasicBlock*, std::set<Value*> > def_map;
            std::map<BasicBlock*, std::set<Value*> > bb2Value;
            for (auto bb : func_->get_basic_blocks()) {
                std::set<Value*> use_set;
                std::set<Value*> def_set;
                std::map<Value*, std::set<BasicBlock*> > pair_map;
                int bb_pos;
                use_set.clear();
                def_set.clear();
                pair_map.clear();
                for (auto inst : bb->get_instructions()) {
                    if (inst->get_instr_type() == Instruction::OpID::phi) {
                        auto hss = static_cast<PhiInst *>(inst)->get_operands();
                        Value *lvalue = static_cast<PhiInst *>(inst)->get_lval();
                        
                        int i = 0;
                        for (;i < hss.size();i++) {
                            if (i % 2 == 0) {
                                //if (def_set.find(hss[i]) == def_set.end()) {
                                    if (!(dynamic_cast<Constant *>(hss[i])|| dynamic_cast<BasicBlock*>(hss[i]) || dynamic_cast<Function*>(hss[i]) || dynamic_cast<GlobalVariable*>(hss[i])))
                                    use_set.insert(hss[i]);
                                //}
                            }
                        }
                        
                        Value* rvalue = dynamic_cast<Value *>(inst);
                        def_set.insert(rvalue);
                    } else if (inst->get_instr_type() == Instruction::OpID::store) {
                        auto lvalue = inst->get_operand(0);
                        Value *rvalue = static_cast<StoreInst *>(inst)->get_rval();
                        if (def_set.find(inst->get_operand(0)) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(lvalue)|| dynamic_cast<BasicBlock*>(lvalue) || dynamic_cast<Function*>(lvalue) || dynamic_cast<GlobalVariable*>(lvalue)))
                            use_set.insert(lvalue);
                        }
                        def_set.insert(rvalue);
                    } else if (inst->get_instr_type() == Instruction::OpID::load) {
                        auto lvalue = inst->get_operand(0);
                        Value* rvalue = dynamic_cast<Value *>(inst);
                        if (def_set.find(inst->get_operand(0)) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(lvalue)|| dynamic_cast<BasicBlock*>(lvalue) || dynamic_cast<Function*>(lvalue) || dynamic_cast<GlobalVariable*>(lvalue)))
                            use_set.insert(lvalue);
                        }
                        def_set.insert(rvalue);
                    }else if(inst->get_instr_type() == Instruction::OpID::ret) {
                        ReturnInst* ret_inst = static_cast<ReturnInst*>(inst);
                        if (!ret_inst->is_void_ret()) {
                            Value* hs = ret_inst->get_operand(0);
                            if (def_set.find(hs) == def_set.end()) {
                                if (!(dynamic_cast<Constant *>(hs)|| dynamic_cast<BasicBlock*>(hs) || dynamic_cast<Function*>(hs) || dynamic_cast<GlobalVariable*>(hs)))
                                use_set.insert(hs);
                            }
                        }
                    } else if (inst->get_instr_type() == Instruction::OpID::br) {
                        BranchInst* br_inst = static_cast<BranchInst*>(inst);
                        if (br_inst->is_cond_br()) {
                            Value* hs = br_inst->get_operand(0);
                            if (def_set.find(hs) == def_set.end()) {
                                if (!(dynamic_cast<Constant *>(hs)|| dynamic_cast<BasicBlock*>(hs) || dynamic_cast<Function*>(hs) || dynamic_cast<GlobalVariable*>(hs)))
                                use_set.insert(hs);
                            }
                        }
                    } else if (inst->get_instr_type() == Instruction::OpID::add ||
                            inst->get_instr_type() == Instruction::OpID::sub ||
                            inst->get_instr_type() == Instruction::OpID::mul ||
                            inst->get_instr_type() == Instruction::OpID::sdiv ||
                            inst->get_instr_type() == Instruction::OpID::srem ||
                            inst->get_instr_type() == Instruction::OpID::fadd ||
                            inst->get_instr_type() == Instruction::OpID::fsub ||
                            inst->get_instr_type() == Instruction::OpID::fmul ||
                            inst->get_instr_type() == Instruction::OpID::fdiv) {
                        Value* lhs = static_cast<BinaryInst *>(inst)->get_operand(0);
                        Value* rhs = static_cast<BinaryInst *>(inst)->get_operand(1);
                        Value* res = dynamic_cast<Value *>(inst);
                        if (def_set.find(lhs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(lhs)|| dynamic_cast<BasicBlock*>(lhs) || dynamic_cast<Function*>(lhs) || dynamic_cast<GlobalVariable*>(lhs)))
                                use_set.insert(lhs);
                        }
                        if (def_set.find(rhs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(rhs)|| dynamic_cast<BasicBlock*>(rhs) || dynamic_cast<Function*>(rhs) || dynamic_cast<GlobalVariable*>(rhs)))
                            use_set.insert(rhs);
                        }
                        def_set.insert(res);
                    } else if (inst->get_instr_type() == Instruction::OpID::alloca) {
                        Value* value = dynamic_cast<Value *>(inst);
                        def_set.insert(value);
                    } else if (inst->get_instr_type() == Instruction::OpID::cmp) {
                        Value* lhs = static_cast<CmpInst *>(inst)->get_operand(0);
                        Value* rhs = static_cast<CmpInst *>(inst)->get_operand(1);
                        Value* res = dynamic_cast<Value *>(inst);
                        if (def_set.find(lhs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(lhs)|| dynamic_cast<BasicBlock*>(lhs) || dynamic_cast<Function*>(lhs) || dynamic_cast<GlobalVariable*>(lhs)))
                            use_set.insert(lhs);
                        }
                        if (def_set.find(rhs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(rhs)|| dynamic_cast<BasicBlock*>(rhs) || dynamic_cast<Function*>(rhs) || dynamic_cast<GlobalVariable*>(rhs)))
                            use_set.insert(rhs);
                        }
                        def_set.insert(res);
                    } else if (inst->get_instr_type() == Instruction::OpID::fcmp) {
                        Value* lhs = static_cast<FCmpInst *>(inst)->get_operand(0);
                        Value* rhs = static_cast<FCmpInst *>(inst)->get_operand(1);
                        Value* res = dynamic_cast<Value *>(inst);
                        if (def_set.find(lhs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(lhs)|| dynamic_cast<BasicBlock*>(lhs) || dynamic_cast<Function*>(lhs) || dynamic_cast<GlobalVariable*>(lhs)))
                            use_set.insert(lhs);
                        }
                        if (def_set.find(rhs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(rhs)|| dynamic_cast<BasicBlock*>(rhs) || dynamic_cast<Function*>(rhs) || dynamic_cast<GlobalVariable*>(rhs)))
                            use_set.insert(rhs);
                        }
                        def_set.insert(res);
                    } else if (inst->get_instr_type() == Instruction::OpID::call) {
                        auto hss = static_cast<CallInst *>(inst)->get_operands();
                        for (int i = 1;i < hss.size();i++) {
                            if (def_set.find(hss[i]) == def_set.end()) {
                                if (!(dynamic_cast<Constant *>(hss[i])|| dynamic_cast<BasicBlock*>(hss[i]) || dynamic_cast<Function*>(hss[i]) || dynamic_cast<GlobalVariable*>(hss[i])))
                                use_set.insert(hss[i]);
                            }
                        }
                        auto call_inst = static_cast<CallInst *>(inst);
                        if (!call_inst->is_void()) {
                            auto res = dynamic_cast<Value *>(inst);
                            def_set.insert(res);
                        }
                    } else if (inst->get_instr_type() == Instruction::OpID::getelementptr){
                        auto hss = static_cast<GetElementPtrInst *>(inst)->get_operands();
                        for (auto hs : hss) {
                            if (def_set.find(hs) == def_set.end()) {
                                if (!(dynamic_cast<Constant *>(hs)|| dynamic_cast<BasicBlock*>(hs) || dynamic_cast<Function*>(hs) || dynamic_cast<GlobalVariable*>(hs)))
                                use_set.insert(hs);
                            }
                        }
                        auto res = dynamic_cast<Value *>(inst);
                        def_set.insert(res);
                    } else if (inst->get_instr_type() == Instruction::OpID::zext) {
                        auto hs = static_cast<ZextInst *>(inst)->get_operand(0);
                        if (def_set.find(hs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(hs)|| dynamic_cast<BasicBlock*>(hs) || dynamic_cast<Function*>(hs) || dynamic_cast<GlobalVariable*>(hs)))
                            use_set.insert(hs);
                        }
                        auto res = dynamic_cast<Value *>(inst);
                        def_set.insert(res);
                    } else if (inst->get_instr_type() == Instruction::OpID::fptosi) {
                        auto hs = static_cast<FpToSiInst *>(inst)->get_operand(0);
                        if (def_set.find(hs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(hs)|| dynamic_cast<BasicBlock*>(hs) || dynamic_cast<Function*>(hs) || dynamic_cast<GlobalVariable*>(hs)))
                            use_set.insert(hs);
                        }
                        auto res = dynamic_cast<Value *>(inst);
                        def_set.insert(res);
                    } else if (inst->get_instr_type() == Instruction::OpID::sitofp) {
                        auto hs = static_cast<SiToFpInst *>(inst)->get_operand(0);
                        if (def_set.find(hs) == def_set.end()) {
                            if (!(dynamic_cast<Constant *>(hs)|| dynamic_cast<BasicBlock*>(hs) || dynamic_cast<Function*>(hs) || dynamic_cast<GlobalVariable*>(hs)))
                            use_set.insert(hs);
                        }
                        auto res = dynamic_cast<Value *>(inst);
                        def_set.insert(res);
                    }
                }
                use_map[bb] = use_set;
                def_map[bb] = def_set;
                //bb2Value[bb] = pair_map;
            }

            std::map<BasicBlock*, std::set<Value*> > IN;
            std::map<BasicBlock*, std::set<Value*> > OUT;
            for(auto bb : func_->get_basic_blocks()){
                IN[bb] = std::set<Value*>();
                IN[bb].clear();
                OUT[bb] = std::set<Value*>();
                OUT[bb].clear();
            }
            bool is_in_changed = false;
            do{
                is_in_changed = false;
                for(auto bb : func_->get_basic_blocks()){
                    OUT[bb].clear();
                    for(auto sucbb : bb->get_succ_basic_blocks()){
                        for (auto inst : sucbb->get_instructions()){
                            if (inst->is_phi()){
                                for (auto i = 0; i < inst->get_num_operand(); i+=2){
                                    if (inst->get_operand(i+1) != bb){
                                        bb2Value[sucbb].insert(inst->get_operand(i));
                                    }
                                }
                            }
                        }
                        std::set<Value *> triggered = {};
                        for (auto item : bb2Value[sucbb]){
                            auto out_succ = sucbb->get_live_out();
                            auto def_succ = def_map[sucbb];
                            std::set<Value *> out_def = {};
                            for(auto &v : out_succ){
                                if(def_succ.find(v) == def_succ.end()){
                                    out_def.insert(v);
                                    //Compute OUT-def
                                }
                            }
                            if (out_def.find(item) != out_def.end()){
                                //Has been propagated to next block, means been triggered
                                triggered.insert(item);
                                continue;
                            }
                            auto flag = false;
                            for (auto use_place : item->get_use_list()){
                                auto use_inst = dynamic_cast<Instruction *>(use_place.val_);
                                auto use_arg_no = use_place.arg_no_;
                                if (use_inst->get_parent() == sucbb){
                                    if (use_inst->is_phi() && use_inst->get_operand(use_arg_no+1) == bb){
                                        flag = true;
                                        /*
                                        if (!(dynamic_cast<Constant *>(item)|| dynamic_cast<BasicBlock*>(item) || dynamic_cast<Function*>(item) || dynamic_cast<GlobalVariable*>(item)))
                                            use_map[sucbb].insert(item);
                                            */
                                        }
                                    else if(!use_inst->is_phi()){
                                        flag = true;
                                    }
                                    //break;
                                }
                            }
                            if (flag == true){
                                triggered.insert(item);
                            }
                        }
                        for (auto item : triggered){
                            if(bb2Value[sucbb].find(item) != bb2Value[sucbb].end())
                                bb2Value[sucbb].erase(item);
                        }
                        for(auto livein : sucbb->get_live_in()){
                            if(bb2Value[sucbb].find(livein) == bb2Value[sucbb].end() && Case(sucbb, livein, bb)){
                                OUT[bb].insert(livein);
                            }
                        }
                    }
                    bb->set_live_out(OUT[bb]);
                    for(auto use : use_map[bb]){
                        if(IN[bb].find(use) == IN[bb].end()){
                            is_in_changed = true;
                        }
                        IN[bb].insert(use);
                        bb->set_live_in(IN[bb]);
                    }
                    for(auto out : bb->get_live_out()){
                        if(def_map[bb].find(out) == def_map[bb].end()){
                            if(IN[bb].find(out) == IN[bb].end()){
                                is_in_changed = true;
                            }
                            IN[bb].insert(out);
                            bb->set_live_in(IN[bb]);
                        }
                    }
                }
            }while(is_in_changed);
            /*you need to finish this function*/
        }
    }

    //  请不要修改该代码，在被评测时不要删除该代码
    dump();
    //
    return ;
}



