import sys
from tabulate import tabulate
with open("files/slr1_first_follow_set.txt", "r", encoding='utf-8') as fp:
    ff = fp.readlines()
res_dict: [str, tuple] = {'Vn': ['first set', 'follow set']}
for f in ff:
    if f.strip() == "":
        continue
    try:
        res_dict[f.split(':')[0]].append('{' + ", ".join(f.strip().split(':')[1].split()) + '}')
    except Exception as e:
        res_dict[f.split(':')[0]] = ['{' + ", ".join(f.strip().split(':')[1].split()) + '}']

table = []
for i in res_dict:
    tmp = []
    tmp.append(i)
    for s in res_dict[i]:
        tmp.append(s)
    table.append(tmp)


print('\033[32mfirst&follow sets:\033[0m')
print(tabulate(table, tablefmt='fancy_grid'))

with open(sys.argv[1], 'r', encoding='utf-8') as fp:
    code = fp.read()
print('\n\033[32msource file:\033[0m')
print(tabulate([[code]], tablefmt='fancy_grid'))


with open("files/lex_res.txt", "r" , encoding='utf-8') as fp:
    lex = fp.readlines()
print('\n\033[32mresult of lexical:\033[0m')
table = []
table_header = ['value', 'type', 'line']
for i in lex:
    table.append(i.strip('()\r\n').split(', '))
print(tabulate(table, headers=table_header, tablefmt='fancy_grid'))

with open("files/slr1_table.txt", "r", encoding='utf-8') as fp:
    table = fp.readlines()
print('\n\033[32mSLR1 analyse table:\033[0m')
item_name = table[0]
table = table[1::]
item_name = 'stat ' + item_name
res_table = [item_name.split()]
for i in table:
    tmp = i.split('|')[:-1:]
    tmp[0] = '(' + tmp[0] + ')'
    res_table.append(tmp)
print(tabulate(res_table, tablefmt='grid'))

print('\n\033[32mSLR1 analyse process:\033[0m')
with open("files/slr1_process.txt", "r", encoding='utf-8') as fp:
    process = fp.readlines()
table = [['step', 'stat-stk', 'char-stk', 'current input', 'ACTION', 'GOTO']]
for p in process:
    table.append(p.strip().split('|')[1:-1])
print(tabulate(table, tablefmt='fancy_grid'))

with open(sys.argv[1], 'r', encoding='utf-8') as fp:
    code = fp.read()
print('\n\033[32msource file:\033[0m')
print(tabulate([[code]], tablefmt='fancy_grid'))


print('\n\033[32mquads:\033[0m')
with open("files/quads.txt", "r", encoding='utf-8') as fp:
    quads = fp.readlines()
table = [['no', 'quad']]
for q in quads:
    table.append(q.strip().split('.'))
print(tabulate(table, tablefmt='fancy_grid'))
