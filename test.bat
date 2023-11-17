@echo off
::后续命令使用的是：UTF-8编码
chcp 65001  
echo 中文
echo file.bat: %0
echo grammar.txt: %1
gcc .\st.c -o st.exe
gcc .\slr1.c -o slr1.exe
gcc .\Lexical.c -o lex.exe
.\st.exe %1
.\lex.exe code.txt
.\slr1.exe %1
