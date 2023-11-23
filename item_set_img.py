import os

from graphviz import Digraph
from IPython.display import Image, display

with open('files/slr1_item_set.txt', 'r', encoding='utf-8') as fp:
    lines = fp.readlines()

# 创建一个有向图
dot = Digraph(comment='项目集图', format='png')
dot.attr('node', shape='rectangle')

# 添加节点
arcs = []   # 边先保存起来
num = 0
item = []
item_sets = []
for line in lines:
    if line.strip() == "":
        continue
    if line.strip().isdigit():
        if len(item):
            item_set = "".join(item)
            item_set = item_set.strip()
            item_sets.append({
                'id': num,
                'item_set': item_set
            })
            item = []
        num = line.strip()
    elif line[0] == "(":
        arcs.append(num + ' ' + line.strip())
    else:
        tmp = line.split('~')
        loc = int(tmp[0])
        prod = tmp[1][:loc:] + '·' + tmp[1][loc::]
        item.append(prod.strip() + r'\l\n')
else:
    if len(item):
        item_set = "".join(item)
        item_set = item_set.strip()
        item_sets.append({
            'id': num,
            'item_set': item_set
        })

for item in item_sets:
    no = 'I' + item['id'] + r':\l\n'
    st = item['item_set']
    dot.node(no.strip(r"\r\n\l:I"), no + st)

# 添加边
for arc in arcs:
    ele = arc.split()
    start = ele[0]
    ele = ele[1::]
    edge = ele[::2]
    ends = ele[1::2]
    for i in range(len(edge)):
        dot.edge(start, ends[i].strip('), '), label=edge[i].strip('(, '))

# 保存为PNG
output_path = 'files/item_sets'
dot.attr('graph', nodesep='0.1', ranksep='0.1',size='200,100',rankdir='LR')
dot.attr('node', shape='box',  style='filled', width='3')

dot.render(output_path, format='png', view=False, engine='dot')

