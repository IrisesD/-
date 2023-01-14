@k = global i32 zeroinitializer
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
  store i32 3389, i32* @k
  %op1 = load i32, i32* @k
  %op2 = icmp slt i32 %op1, 10000
  %op3 = zext i1 %op2 to i32
  %op4 = icmp ne i32 %op3, 0
  br i1 %op4, label %label10, label %label13
label_ret:                                                ; preds = %label13
  ret i32 %op14
label10:                                                ; preds = %label_entry
  %op11 = load i32, i32* @k
  %op12 = add i32 %op11, 1
  store i32 %op12, i32* @k
  br label %label15
label13:                                                ; preds = %label_entry, %label27
  %op39 = phi i32 [ %op40, %label27 ], [ undef, %label_entry ]
  %op14 = load i32, i32* @k
  br label %label_ret
label15:                                                ; preds = %label10, %label38
  %op40 = phi i32 [ 112, %label10 ], [ %op41, %label38 ]
  %op17 = icmp sgt i32 %op40, 10
  %op18 = zext i1 %op17 to i32
  %op19 = icmp ne i32 %op18, 0
  br i1 %op19, label %label20, label %label27
label20:                                                ; preds = %label15
  %op22 = sub i32 %op40, 88
  %op24 = icmp slt i32 %op22, 1000
  %op25 = zext i1 %op24 to i32
  %op26 = icmp ne i32 %op25, 0
  br i1 %op26, label %label29, label %label38
label27:                                                ; preds = %label15
  call void @put_int(i32 %op40)
  br label %label13
label29:                                                ; preds = %label20
  %op32 = sub i32 %op22, 10
  %op35 = add i32 %op32, 11
  %op37 = add i32 %op35, 11
  br label %label38
label38:                                                ; preds = %label20, %label29
  %op41 = phi i32 [ %op22, %label20 ], [ %op37, %label29 ]
  br label %label15
}
