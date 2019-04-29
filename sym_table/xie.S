;这个是asm.S文件内容
AREA MY_MAIN, CODE, READONLY
ENTRY
CODE32
import my_fun
PRESERVE8

str lr, [sp, #-4]!

ldr r0, =0x4

bl	my_fun
str r4, =r1
str r5, =r1
ldr pc, [sp], #4


;这个是my_fun.c文件内容

void my_fun(int n, int ret)
{
	ret=0;
	for(int i=0;i<n;++i)
		ret*=i;
}



;第三题:0x10000000
