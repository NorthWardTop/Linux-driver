


/*
原子变量：
i--->内存barrier
原子变量通常用来做自加 自减

//定义设置atomic原子变量
void atomic_set(atomic_t *aval,int value);
atomic_t aval =ATOMIC_INIT(0);

//获取原子变量值
atomic_read(atomic_t *aval);

//原子变量自加自减
void atomic_add(int i, atomic_t *aval);
void atomic_sub(int i, atomic_t *aval);
void atomic_inc(atomic_t *aval);
void atomic_dec(atomic_t *aval);

//原子变量的操作测试
//测试结果为0 返回ture 否则返回false
//先测试后操作
int atomic_inc_and_test(atomic_t *aval);
int atomic_dec_and_test(atomic_t *aval);
int atomic_sub_and_test(int i, atomic_t *aval);

//先操作后返回  返回为新的值
int atomic_add_return(int i, atomic_t *aval);
int atomic_sub_return(int i, atomic_t *aval);
int atomic_inc_return(atomic_t *aval);
int atomic_dec_return(atomic_t *aval);

//demo 使用原子变量实现 当前LKM只能被一个进程打开

*/

int main(int argc, char const *argv[])
{
    

    return 0;
}
