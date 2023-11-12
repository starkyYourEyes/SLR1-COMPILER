# 1.项目文法 
S’-> S
S-> CS 
S-> begin L end 
S-> A
**C-> if B then**
L-> S
L-> K S
K-> L;
A-> id:=E
E-> E + E
E-> E * E
E-> -E
E-> (E)
E-> id
B-> B or B
B-> B and B
B-> not B
B-> (B)
B-> E rop E
B-> true
B-> false
其中：S-语句,L复合语句,A赋值语句,E算术表达式,B布尔表达式,rop关系运算符
# 2.词法分析
词法分析程序对源程序的**字符流**进行扫描和分解，并识别出每一个单词。所有识别的单词都应该指出其**所属类别**，如整数、保留字、标识符等，并应指出哪些单词是**不合法**的。词法分析程序的结果经过简单处理将作为语法分析的输入流，执行语法分析程序。需要注意的是，当词法分析识别出不合法的单词时，应拒绝进行语法分析，否则语法分析的结果也是无意义的。
# 3.语法分析
语法分析将按照**SLR(1)**分析过程，根据输入串、状态栈、符号栈以及ACTION表和GOTO表进行语法分析，指出源程序是否为该文法合法的句子。若源程序无法被语法分析程序识别，则指出可能的错误。
# 4.具体实现 
## 4.1 词法分析程序
**词法分析**程序中需要有一个变量来记录当前扫描到的字符位置，这个变量贯穿整个词法分析过程。当没有扫描到字符串结尾时，逐个扫描字符串。
如果遇到空格、换行符等无意义字符时，直接将位置变量后移。


S'->S
S->CS 
S->begin L end 
S->A
C->if B then
L->S
L->K S
K->L;
A->id:=E
E->E+E
E->E*E
E->-E
E->(E)
E->id
B->B or B
B->B and B
B->not B
B->(B)
B->E rop E
B->true
B->false

S': S
S: if, begin, id
C: if
L: if, begin, id
K: if, begin, id
A: id
E:  -, (, id
B:  not, (, -, (, id, true, flase