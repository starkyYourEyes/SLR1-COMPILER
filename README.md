# SLR1-COMPILER
## **编译原理课设**，简单文法的SLR1分析，C语言实现

### 1.项目文法 
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
E-> E _ E  
E-> E * E  
E-> E / E 
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
> 其中:
S语句,L复合语句,A赋值语句,E算术表达式,B布尔表达式,rop关系运算符  
# add:
新增加了两条，E-> E / E(除法), E-> E $ E(减法--为了不改变原来的文法符号)
文法中S-> begin L end 中的L应该改为K比较好！

### 2.识别文法中的终结符和非终结符并计算first集和follow集
1. 扫描文法文件，识别非终结符和终结符。  
2. 分别计算first集和follow集并输出到文件first-follow-set.txt 

### 3.词法分析
词法分析程序按行对源代码文件进行扫描，并识别出每一个单词和符号。所识别出的每一项为一个二元组，如`(begin, keyword, line_number)`，其中第一项为这个符号，第二项为这个符号的类别，第三项为所在行号——方便报错定位。  
同时词法分析会识别出非法的符号，当词法分析识别出非法符号时，程序直接exit，并给出出错的行号列号。 

### 4.语法分析
语法分析根据词法分析的输出进行分析 *（语法分析之前先执行词法分析）*，此法输出的分析在lex_res.txt文件中。
语法分析按照**SLR(1)**分析过程，根据输入串、状态栈、符号栈以及ACTION表和GOTO表进行语法分析，指出源程序是否为该文法合法的句子。若源程序无法被语法分析程序识别，指出出错的行号。   

**移进-规约冲突** 采用 **计算优先级**的方式进行解决。

### 5.语义分析
