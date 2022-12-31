#include "Check.h"
#include "Module.h"

void Check::execute() {
    //TODO write your IR Module checker here.
    //Check all the item from minor to major
    CheckDef();
    CheckDefUse();
    //Those two are belonging to the operator level
    CheckPhi();
    CheckBrPos();
    CheckRetPos();
    CheckIsBrOrRet();
    //Those four are belonging to the instruction level
    CheckParent();
    CheckSuccAndPreds();
    //Those two are belonging to the basicblock level
}
void Check::CheckDef(){
    std::set<Value*> def;
    for(auto &glob : this->module->get_global_variable()){
        //All GlobalVariables
        def.insert(glob);
    }
    for(auto &func : this->module->get_functions()){
        //All functions
        def.insert(func);
        for(auto &arg : func->get_args())
            //All function arguments
            def.insert(arg);
        for(auto &bb : func->get_basic_blocks()){
            //All bb
            def.insert(bb);
            for(auto &inst : bb->get_instructions()){
                if(!inst->is_void()){
                    //All return value
                    def.insert(inst);
                }
            }
        }
    }
    for(auto &func : this->module->get_functions()){
        for(auto &bb : func->get_basic_blocks()){
            for(auto &inst : bb->get_instructions()){
                for(auto &opeartors : inst->get_operands()){
                    //All use place
                    if(def.find(opeartors) == def.end() && !dynamic_cast<Constant* >(opeartors)){
                        std::cout << "Error in Inst: " << inst->get_name() << std::endl;
                        for(auto &opeartors : inst->get_operands())
                            std::cout << opeartors->get_name() << std::endl;
                        std::cout << "Operator not defined" << std::endl;
                        exit(6);
                    }
                }
            }
        }
    }
}
void Check::CheckDefUse(){
    std::set<Value* > def;
    for(auto &glob : this->module->get_global_variable()){
        //Check GlobalVariables use
        for(auto &param : glob->get_use_list()){
            if(!dynamic_cast<User* >(param.val_)){
                std::cout << "Error in: " << glob->get_name() << std::endl;
                std::cout << "Operator not defined" << std::endl;
                exit(6);
            }
        }
    }
    for(auto &func : this->module->get_functions()){
        //Check functions use
        for(auto &param : func->get_use_list()){
            if(!dynamic_cast<User* >(param.val_)){
                std::cout << "Error in: " << func->get_name() << std::endl;
                std::cout << "Operator not defined" << std::endl;
                exit(6);
            }
        }
    }
    for(auto &func : this->module->get_functions()){
        for(auto &arg : func->get_args()){
            //Check function arguments use
            for(auto &param : arg->get_use_list()){
                if(!dynamic_cast<User* >(param.val_)){
                    std::cout << "Error in: " << arg->get_name() << std::endl;
                    std::cout << "Operator not defined" << std::endl;
                    exit(6);
                }
            }
        }
    }
    for(auto &func : this->module->get_functions()){
        for(auto &bb : func->get_basic_blocks()){
            //Check BasicBlocks use
            for(auto &param : bb->get_use_list()){
                if(!dynamic_cast<User* >(param.val_)){
                    std::cout << "Error in: " << bb->get_name() << std::endl;
                    std::cout << "Operator not defined" << std::endl;
                    exit(6);
                }
            }
        }
    }
    for(auto &func : this->module->get_functions()){
        for(auto &bb : func->get_basic_blocks()){
            for(auto &inst : bb->get_instructions()){
                //Check inst return value use
                for(auto &param : inst->get_use_list()){
                    if(!dynamic_cast<User* >(param.val_)){
                        std::cout << "Error in: " << inst->get_name() << std::endl;
                        std::cout << "Operator not defined" << std::endl;
                        exit(6);
                    }
                }
                for(auto &opeartors : inst->get_operands()){
                    //Check inst operators use
                    for(auto &param : inst->get_use_list()){
                        if(!dynamic_cast<User* >(param.val_)){
                            std::cout << "Error in: " << opeartors->get_name() << std::endl;
                            std::cout << "Operator not defined" << std::endl;
                            exit(6);
                        }
                    }
                }
            }
        }
    }
}
void Check::CheckPhi(){
    for(auto &func : this->module->get_functions()){
        if (func->get_basic_blocks().empty())
            continue;
        for(auto &bb : func->get_basic_blocks()){
            int flag = 1;
            for(auto &inst : bb->get_instructions()){
                ////Check whether phi insts are at the begining of a bb
                if(!inst->is_phi()) flag = 0;
                else if(flag == 0){
                    std::cout << "Error in Inst: " << std::endl;
                    inst->print();
                    std::cout << "Position error." << std::endl;
                    exit(4);
                }
            }
        }
    }
}

void Check::CheckBrPos(){
    for(auto &func : this->module->get_functions()){
        if (func->get_basic_blocks().empty())
            continue;
        for(auto &bb : func->get_basic_blocks()){
            for(auto &inst : bb->get_instructions()){
                //Check whether ret insts are terminators
                if(inst->is_br() && inst != bb->get_terminator()){
                    std::cout << "Error in Inst: " << std::endl;
                    inst->print();
                    std::cout << "Position error." << std::endl;
                    exit(4);
                }
            }
        }
    }
}

void Check::CheckRetPos(){
    for(auto &func : this->module->get_functions()){
        if (func->get_basic_blocks().empty())
            continue;
        for(auto &bb : func->get_basic_blocks()){
            for(auto &inst : bb->get_instructions()){
                //Check whether ret insts are terminators
                if(inst->is_ret() && inst != bb->get_terminator()){
                    std::cout << "Error in Inst: " << std::endl;
                    inst->print();
                    std::cout << "Position error." << std::endl;
                    exit(4);
                }
            }
        }
    }
}

void Check::CheckIsBrOrRet(){
    for(auto &func : this->module->get_functions()){
        if (func->get_basic_blocks().empty())
            continue;
        for(auto &bb : func->get_basic_blocks()){
            //Check whether bb's terminor is ret or br
            if(bb->get_terminator()->is_br() || bb->get_terminator()->is_ret())
                continue;
            std::cout << "Error in BB: " << bb->get_name() << std::endl;
            std::cout << "Terminator inst error." << std::endl;
            exit(5);
        }
    }
}

void Check::CheckParent(){
    for(auto &func : this->module->get_functions()){
        if (func->get_basic_blocks().empty())
            continue;
        for(auto &bb : func->get_basic_blocks()){
            if(bb->get_parent() == nullptr){
                //Check if bb->get_parent returns null
                std::cout << "Error in BB: " << bb->get_name() << std::endl;
                std::cout << "Has no parent." << std::endl;
                std::cout << "Parent should be: " << func->get_name() << std::endl;
                exit(1);
            }
            else if(bb->get_parent() != func){   
                //Check bb's parent
                std::cout << "Error in BB: " << bb->get_name() << std::endl;
                std::cout << "Parent: " << bb->get_parent()->get_name() << std::endl;
                std::cout << "Parent should be: " << func->get_name() << std::endl;
                exit(2);
            }
            else{
                for(auto &inst : bb->get_instructions()){
                    if(inst->get_parent() == nullptr){
                        //Check if inst->get_parent returns null
                        std::cout << "Error in Inst: " << std::endl;
                        inst->print();
                        std::cout << "Has no parent." << std::endl;
                        std::cout << "Parent should be: " << bb->get_name() << std::endl;
                        exit(1);
                    }
                    else if(inst->get_parent() != bb){   
                        //Check insts' parent
                        std::cout << "Error in Inst: " << std::endl;
                        inst->print();
                        std::cout << "Parent: " << inst->get_parent()->get_name() << std::endl;
                        std::cout << "Parent should be: " << bb->get_name() << std::endl;
                        exit(2);
                    }
                    else if(inst->get_function() != func){
                        //Check if inst belongs to inst->get_function()
                        std::cout << "Error in Inst: "  << std::endl;
                        inst->print();
                        std::cout << "Func: " << inst->get_function()->get_name() << std::endl;
                        std::cout << "Func should be: " << func->get_name() << std::endl;
                        exit(3);
                    }
                }
            }
        }
    }
}





void Check::CheckSuccAndPreds(){
    for(auto &func : this->module->get_functions()){
        if (func->get_basic_blocks().empty())
            continue;
        for(auto &bb : func->get_basic_blocks()){
            //Check all of bb's succ bb
            for(auto &sucbb : bb->get_succ_basic_blocks()){
                int flag = 0;
                for(auto &pre_sucbb : sucbb->get_pre_basic_blocks()){
                    if(pre_sucbb == bb){
                        flag = 1;
                        break;
                    }
                }
                if(!flag){
                    //bb not in its suc's pre list
                    std::cout << "Relation error in BB: " << sucbb->get_name() << std::endl;
                    std::cout << "BB: " << bb->get_name() << "not in " << sucbb->get_name() << "'s preds list." << std::endl;
                    exit(7);
                }
            }
            //Check all of bb's preds bb
            for(auto &prebb : bb->get_pre_basic_blocks()){
                int flag = 0;
                for(auto &suc_prebb : prebb->get_succ_basic_blocks()){
                    if(suc_prebb == bb){
                        flag = 1;
                        break;
                    }
                }
                if(!flag){
                    //bb not in its pre's suc list
                    std::cout << "Relation error in BB: " << prebb->get_name() << std::endl;
                    std::cout << "BB: " << bb->get_name() << "not in " << prebb->get_name() << "'s succ list." << std::endl;
                    exit(7);
                }
            }
        }
    }
}