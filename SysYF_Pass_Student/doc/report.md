# 实验报告

## 必做部分

### 支配树

#### B1-1

#### B1-2

#### B1-3

#### B1-4

#### B1-5

#### B1-6

### Mem2Reg

#### B2-1

```
void Mem2Reg::execute(){
    for(auto fun: module->get_functions()){
        if(fun->get_basic_blocks().empty())continue;
        func_ = fun;
        lvalue_connection.clear();
        no_union_set.clear();
        insideBlockForwarding();
        genPhi();
        module->set_print_name();
        valueDefineCounting();
        valueForwarding(func_->get_entry_block());
        removeAlloc();
    }
}
```

观察execute()函数可以知道，Mem2Reg优化首先遍历模块的每一个函数，对每一个函数依次执行以下操作：

* 初始化一些变量，判断是否有BasicBlocks
* 执行insideBlockForwarding()，遍历每个BasicBlocks，对其中每条指令进行遍历，将一些映射关系存入进几个map中，在最后对forward_list进行遍历，得到所有可被替换的操作数，并将其替换为value，之后删除那些无用的load和store，完成块内优化。（本函数完成的是块内的，不依赖其他块以及$\phi$函数的优化）
* 执行genPhi()，在找到跨多个程序块的活动变量名的集合（全局名字集合），然后对该集合中的名字插入$\phi$函数。
* 执行valueDefineCounting()，对需要重命名的变量进行计数，对该基本名的各个定义通过添加数字下标来进行区分，以达到静态单赋值的效果。
* 执行valueForwarding()，删除剩余的非数组元素且非global的变量相关的load和store指令，通过一些存储映射关系的map以及$\phi$指令，完成每个基本块内一些依赖于其他块的优化。
* 执行removeAlloc()，删除所有int、float和pointer的alloca指令（数组的alloca不会删除）

#### B2-2

具体的有两层嵌套结构的例子如下：

```
int main(){
    int a = 0;
    int b = 1;
    while(a < 5) {
        if(a != 5){
            b = b + 1;
        }
        else{
            b = b - 1;
        }
        a = a + 1;
    }
    return b;
}
```

不开-O优化时，编译产生的IR如下：

```
declare i32 @get_int()

declare float @get_float()

declare i32 @get_char()

declare i32 @get_int_array(i32*)

declare i32 @get_float_array(float*)

declare void @put_int(i32)

declare void @put_float(float)

declare void @put_char(i32)

declare void @put_int_array(i32, i32*)

declare void @put_float_array(i32, float*)

define i32 @main() {
label_entry:
  %op0 = alloca i32
  %op1 = alloca i32
  store i32 0, i32* %op1
  %op2 = alloca i32
  store i32 1, i32* %op2
  %op3 = alloca [1 x i32]
  %op4 = getelementptr [1 x i32], [1 x i32]* %op3, i32 0, i32 0
  store i32 0, i32* %op4
  br label %label6
label_ret:                                                ; preds = %label16
  %op5 = load i32, i32* %op0
  ret i32 %op5
label6:                                                ; preds = %label_entry, %label24
  %op7 = load i32, i32* %op1
  %op8 = icmp slt i32 %op7, 5
  %op9 = zext i1 %op8 to i32
  %op10 = icmp ne i32 %op9, 0
  br i1 %op10, label %label11, label %label16
label11:                                                ; preds = %label6
  %op12 = load i32, i32* %op1
  %op13 = icmp ne i32 %op12, 5
  %op14 = zext i1 %op13 to i32
  %op15 = icmp ne i32 %op14, 0
  br i1 %op15, label %label18, label %label21
label16:                                                ; preds = %label6
  %op17 = load i32, i32* %op2
  store i32 %op17, i32* %op0
  br label %label_ret
label18:                                                ; preds = %label11
  %op19 = load i32, i32* %op2
  %op20 = add i32 %op19, 1
  store i32 %op20, i32* %op2
  br label %label24
label21:                                                ; preds = %label11
  %op22 = load i32, i32* %op2
  %op23 = sub i32 %op22, 1
  store i32 %op23, i32* %op2
  br label %label24
label24:                                                ; preds = %label18, %label21
  %op25 = load i32, i32* %op1
  %op26 = add i32 %op25, 1
  store i32 %op26, i32* %op1
  br label %label6
}
```

开启-O优化，即启用Mem2Reg时，产生的IR如下：

```
declare i32 @get_int()

declare float @get_float()

declare i32 @get_char()

declare i32 @get_int_array(i32*)

declare i32 @get_float_array(float*)

declare void @put_int(i32)

declare void @put_float(float)

declare void @put_char(i32)

declare void @put_int_array(i32, i32*)

declare void @put_float_array(i32, float*)

define i32 @main() {
label_entry:
  %op3 = alloca [1 x i32]
  %op4 = getelementptr [1 x i32], [1 x i32]* %op3, i32 0, i32 0
  store i32 0, i32* %op4
  br label %label6
label_ret:                                                ; preds = %label16
  ret i32 %op27
label6:                                                ; preds = %label_entry, %label24
  %op27 = phi i32 [ 1, %label_entry ], [ %op29, %label24 ]
  %op28 = phi i32 [ 0, %label_entry ], [ %op26, %label24 ]
  %op8 = icmp slt i32 %op28, 5
  %op9 = zext i1 %op8 to i32
  %op10 = icmp ne i32 %op9, 0
  br i1 %op10, label %label11, label %label16
label11:                                                ; preds = %label6
  %op13 = icmp ne i32 %op28, 5
  %op14 = zext i1 %op13 to i32
  %op15 = icmp ne i32 %op14, 0
  br i1 %op15, label %label18, label %label21
label16:                                                ; preds = %label6
  br label %label_ret
label18:                                                ; preds = %label11
  %op20 = add i32 %op27, 1
  br label %label24
label21:                                                ; preds = %label11
  %op23 = sub i32 %op27, 1
  br label %label24
label24:                                                ; preds = %label18, %label21
  %op29 = phi i32 [ %op23, %label21 ], [ %op20, %label18 ]
  %op26 = add i32 %op28, 1
  br label %label6
}
```

Mem2Reg每个阶段的存储的内容以对IR的处理在B2-1中已经简略回答，下面回答B2-2的问题：

* `Mem2Reg`可能会删除的指令类型是哪些？对哪些分配(alloca)指令会有影响？

在isLocalVarOp()函数中，存在以下判断：

```
return !glob && !array_element_ptr;
```

而在Mem2Reg.cpp中，会删除所有isLocalVarOp()为true的指令。

在removeAlloca()函数中，删除所有int或float或pointer类型的alloca。

同时，上述优化后的IR中，只有数组元素相关的alloca和store存在，因此：

Mem2Reg可能删除的指令类型：非数组元素且非global的变量相关的load和store指令

对Alloca指令的影响：所有int或float或pointer类型的alloca指令（数组alloca不受影响）

* 在基本块内前进`insideBlockForwarding`时，对`store`指令处理时为什么`rvalue`在`forward_list`中存在时，就需要将`rvalue`替换成`forward_list`映射中的`->second`值？

rvalue在forward_list中存在时，说明rvalue对应的变量在此处store指令中可以被常量或其他变量替换，替换后可以取消对原本rvalue对应的变量的引用，从而可以在最终删除forward_list中指令时，保证其赋值的变量都不被其他指令引用。

* 在基本块内前进时，`defined_list`代表什么含义？

以下代码对defined_list进行了操作：

```cpp
if(defined_list.find(lvalue) != defined_list.end()){
	auto pair = defined_list.find(lvalue);
  delete_list.insert(pair->second);
  pair->second = inst;
}
else{
	defined_list.insert({lvalue, inst});
}
```

观察代码可知，defined_list的含义是一个变量到定义这个变量当前值的指令的映射。

如以下指令：

```
store i32 0, i32* %1			//a
store i32 1, i32* %1			//b
```

在执行完对指令a的处理后，defined_list中存储的是映射%1——>a

在执行完对指令b的处理后，defined_list中存储的是映射%1——>b

* 生成phi指令`genPhi`的第一步两层for循环在收集什么信息，这些信息在后面的循环中如何被利用生成Phi指令？

第一步两层for循环收集了该基本块中所有跨多个程序块的活动变量名的集合，即全局名字（global name）集合Globals，将load指令的变量加入globals，将store存储的位置和所属bb块加入defined_in_block，随着算法构建Globals集合，也为每个名字构造了一个列表，包含所有定义该名字的基本块，这些基本块列表充当了一个初始化的WorkList，对WorkList的每个基本块，在其支配边界中每个程序块的起始处插入$\phi$函数。

- `valueDefineCounting`为`define_var`记录了什么信息

为define_var记录了每个基本块的$\phi$指令和store指令中基本名静态单赋值形式的下标，将$\phi$指令和store指令视作对变量的define。每个全局名字对应一个栈，栈顶元素的值是当前静态单赋值形式名的下标。

* `valueForwarding`在遍历基本块时采用的什么方式

遍历基本块时，先遍历一遍，对$\phi$指令进行处理，之后再遍历一遍，判断是否为$\phi$指令或isLocalVarOp()为false，对其余的store和load指令进行处理。后续对该块的所有后继块递归地进行深度优先遍历。

* `valueForwarding`中为什么`value_status`需要对phi指令做信息收集

```
for(auto inst: bb->get_instructions()){
        if(inst->get_instr_type() != Instruction::OpID::phi)break;
        auto lvalue = dynamic_cast<PhiInst *>(inst)->get_lval();
        auto value_list = value_status.find(lvalue);
        if(value_list != value_status.end()){
            value_list->second.push_back(inst);
        }
        else{
            value_status.insert({lvalue, {inst}});
        }
    }
```

value_status中存放变量和其所有静态单赋值形式的值，因为变量可能存在来自其他基本块的静态单赋值形式的值，如果不对phi指令做信息收集就会忽略这些值，从而对后续优化产生影响。

* `valueForwarding`中第二个循环对`load`指令的替换是什么含义

```
Value* lvalue = static_cast<LoadInst *>(inst)->get_lval();
Value* new_value = *(value_status.find(lvalue)->second.end() - 1);
inst->replace_all_use_with(new_value);
```

首先找到load指令的lvalue，即需要读取的变量，之后获取所有静态单赋值形式中最近的值，即执行到当前load指令时该变量的值，对其进行替换，取消对需要读取的变量的引用，从而后续可以删除指令。

* `valueForwarding`中出现的`defined_var`和`value_status`插入条目之间有什么联系

defined_var记录了基本名所有静态单赋值形式的下标，value_status每个条目是变量到其所有静态单赋值形式的定义值的指令的映射。

例如：

```
a = 1;			//x
a = 2;			//y	
a = 3;			//z
```

因为编译后产生的IR是三条store指令，在valueDefineCounting()中，对这三条指令处理后，define_var.find(bb)->second为{a,a,a}（应该是指向a的指针，这里以及下面简写），分别对应a的三种静态单赋值形式$a_0,a_1,a_2$

而value_status在处理完三条指令后，存有{(a, x), (a, y), (a, z)}，存放a的三种静态单赋值形式$a_0,a_1,a_2$到其定义值的指令的映射。(x, y, z为指向三条指令的指针)

#### B2-3

为了说明以上例子要求能够体现Mem2Reg的效果，我们使用另外两个例子进行对比：

```
int main(){
    int a = 1;
    int b = 1;
    if(a < 5){
    }
    else{
    }
    return b;
}
```

该例子中存在分支结构，但是两个分支中都没有对变量的定值和引用，此时编译出的IR主体如下：

```
define i32 @main() {
label_entry:
  %op4 = icmp slt i32 1, 5
  %op5 = zext i1 %op4 to i32
  %op6 = icmp ne i32 %op5, 0
  br i1 %op6, label %label8, label %label9
label_ret:                                                ; preds = %label10
  ret i32 1
label8:                                                ; preds = %label_entry
  br label %label10
label9:                                                ; preds = %label_entry
  br label %label10
label10:                                                ; preds = %label8, %label9
  br label %label_ret
}
```

可以看到即使存在if-else分支结构，IR中也没有phi指令的存在，因此对变量在不同分支或迭代中的定值和引用的要求是为了体现Mem2Reg中插入phi指令的效果，否则生成出的IR中没有phi指令的存在。

使用多层嵌套，调用phiStatistic()打印phi指令信息时，出现如下信息：

```
phi find: %op27 = phi i32 [ 1, %label_entry ], [ %op29, %label24 ]
phi find: %op28 = phi i32 [ 0, %label_entry ], [ %op26, %label24 ]
phi find: %op29 = phi i32 [ %op23, %label21 ], [ %op20, %label18 ]
```

可以发现其中的phi指令存在由支配树定义的依赖关系。

而使用单层分支结构时不存在phi指令的依赖关系，因此无法检验支配树构建的效果。

综上，使用至少双层嵌套结构并对一个变量在不同分支或迭代中定值和引用可以体现Mem2Reg的效果。

### 活跃变量分析

#### B3-1

### 检查器

#### B4-1