//
// Created by 吴骏 on 2023/1/1.
//
#include "ConstPropagation.h"

ConstantInt *ConstFolder::compute(Instruction::OpID op, ConstantInt *value1, ConstantInt *value2) {
    int const_value1 = value1->get_value();
    int const_value2 = value2->get_value();
    switch (op) {
        case Instruction::add:
            return ConstantInt::get(const_value1 + const_value2, module_);
        case Instruction::sub:
            return ConstantInt::get(const_value1 - const_value2, module_);
        case Instruction::mul:
            return ConstantInt::get(const_value1 * const_value2, module_);
        case Instruction::sdiv:
            return ConstantInt::get((int) (const_value1 / const_value2), module_);
        case Instruction::srem:
            return ConstantInt::get(const_value1 % const_value2, module_);
        default:
            return nullptr;
    }
}

ConstantFloat *ConstFolder::compute_f(Instruction::OpID op, ConstantFloat *value1, ConstantFloat *value2) {
    float const_value1 = value1->get_value();
    float const_value2 = value2->get_value();
    switch (op) {
        case Instruction::fadd:
            return ConstantFloat::get(const_value1 + const_value2, module_);
        case Instruction::fsub:
            return ConstantFloat::get(const_value1 - const_value2, module_);
        case Instruction::fmul:
            return ConstantFloat::get(const_value1 * const_value2, module_);
        case Instruction::fdiv:
            return ConstantFloat::get((float) (const_value1 / const_value2), module_);
        default:
            return nullptr;
    }
}

ConstantInt *ConstFolder::GetCmpValue(CmpInst::CmpOp op, int const_value1, int const_value2){
    ConstantInt* res;
    switch (op) {
        case CmpInst::EQ:
            res = ConstantInt::get((const_value1 == const_value2), module_);
            break;
        case CmpInst::LE:
            res = ConstantInt::get((const_value1 <= const_value2), module_);
            break;
        case CmpInst::LT:
            res = ConstantInt::get((const_value1 < const_value2), module_);
            break;
        case CmpInst::GE:
            res = ConstantInt::get((const_value1 >= const_value2), module_);
            break;
        case CmpInst::GT:
            res = ConstantInt::get((const_value1 > const_value2), module_);
            break;
        case CmpInst::NE:
            res = ConstantInt::get((const_value1 != const_value2), module_);
            break;
    }
    return res;
}

ConstantFloat *ConstFolder::GetFCmpValue(FCmpInst::CmpOp op, float const_value1, float const_value2){
    ConstantFloat* res;
    switch (op) {
        case FCmpInst::EQ:
            res = ConstantFloat::get((const_value1 == const_value2), module_);
            break;
        case FCmpInst::LE:
            res = ConstantFloat::get((const_value1 <= const_value2), module_);
            break;
        case FCmpInst::LT:
            res = ConstantFloat::get((const_value1 < const_value2), module_);
            break;
        case FCmpInst::GE:
            res = ConstantFloat::get((const_value1 >= const_value2), module_);
            break;
        case FCmpInst::GT:
            res = ConstantFloat::get((const_value1 > const_value2), module_);
            break;
        case FCmpInst::NE:
            res = ConstantFloat::get((const_value1 != const_value2), module_);
            break;
    }
    return res;
}

// 用来判断value是否为ConstantInt，如果不是则会返回nullptr
ConstantInt *cast_to_const_int(Value *value) {
    auto const_int_ptr = dynamic_cast<ConstantInt *>(value);
    if (const_int_ptr) {
        return const_int_ptr;
    } else {
        return nullptr;
    }
}

ConstantFloat *cast_to_const_float(Value *value) {
    auto const_float_ptr = dynamic_cast<ConstantFloat *>(value);
    if (const_float_ptr) {
        return const_float_ptr;
    } else {
        return nullptr;
    }
}

void ConstPropagation::execute() {
    module->set_print_name();

    auto c_folder = new ConstFolder(this->module);

    for (auto &func: this->module->get_functions()) {
        if (func->get_basic_blocks().empty()) {
            continue;
        } else {
            for (auto bb: func->get_basic_blocks()) {
                for (auto inst: bb->get_instructions()) {
                    if (inst->is_int_binary()) {
                        if (cast_to_const_int(inst->get_operand(0)) && cast_to_const_int(inst->get_operand(1))) {
                            auto const_value = c_folder->compute(inst->get_instr_type(),
                                                                 cast_to_const_int(inst->get_operand(0)),
                                                                 cast_to_const_int(inst->get_operand(1)));
                            auto rvalue = dynamic_cast<Value *>(inst);
                            auto cc_value = dynamic_cast<Value *>(const_value);
                            rvalue->replace_all_use_with(cc_value);
                        }
                    } else if (inst->is_float_binary()) {
                        if (cast_to_const_float(inst->get_operand(0)) && cast_to_const_float(inst->get_operand(1))) {
                            auto const_value = c_folder->compute_f(inst->get_instr_type(),
                                                                   cast_to_const_float(inst->get_operand(0)),
                                                                   cast_to_const_float(inst->get_operand(1)));
                            auto rvalue = dynamic_cast<Value *>(inst);
                            auto cc_value = dynamic_cast<Value *>(const_value);
                            rvalue->replace_all_use_with(cc_value);
                        }
                    } else if (inst->get_instr_type() == Instruction::cmp) {
                        if (cast_to_const_int(inst->get_operand(0)) && cast_to_const_int(inst->get_operand(1))) {
                            auto cmp_inst = dynamic_cast<CmpInst *>(inst);
                            int const_value1 = cast_to_const_int(inst->get_operand(0))->get_value();
                            int const_value2 = cast_to_const_int(inst->get_operand(1))->get_value();
                            auto rvalue = dynamic_cast<Value *>(inst);
                            rvalue->replace_all_use_with(c_folder->GetCmpValue(cmp_inst->get_cmp_op(), const_value1, const_value2));
                        }
                    } else if (inst->get_instr_type() == Instruction::fcmp) {
                        if (cast_to_const_float(inst->get_operand(0)) && cast_to_const_float(inst->get_operand(1))) {
                            auto cmp_inst = dynamic_cast<FCmpInst *>(inst);
                            float const_value1 = cast_to_const_float(inst->get_operand(0))->get_value();
                            float const_value2 = cast_to_const_float(inst->get_operand(1))->get_value();
                            auto rvalue = dynamic_cast<Value *>(inst);
                            rvalue->replace_all_use_with(c_folder->GetFCmpValue(cmp_inst->get_cmp_op(), const_value1, const_value2));
                        }
                    } else if (inst->get_instr_type() == Instruction::sitofp) {
                        if (cast_to_const_int(inst->get_operand(0))) {
                            auto rvalue = dynamic_cast<Value *>(inst);
                            auto const_f = float(cast_to_const_int(inst->get_operand(0))->get_value());
                            rvalue->replace_all_use_with(ConstantFloat::get(const_f, module));
                        }
                    } else if (inst->is_fptosi()) {
                        if (cast_to_const_float(inst->get_operand(0))) {
                            auto rvalue = dynamic_cast<Value *>(inst);
                            auto const_i = int(cast_to_const_float(inst->get_operand(0))->get_value());
                            rvalue->replace_all_use_with(ConstantInt::get(const_i, module));
                        }
                    } else if (inst->get_instr_type() == Instruction::zext) {
                        if (cast_to_const_int(inst->get_operand(0))) {
                            auto rvalue = dynamic_cast<Value *>(inst);
                            auto const_value = cast_to_const_int(inst->get_operand(0))->get_value();
                            rvalue->replace_all_use_with(ConstantInt::get(const_value, module));
                        }
                    }
                }
            }
        }
    }
}