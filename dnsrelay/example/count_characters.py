def count_characters(file_path):
    character_count = {}
    
    # 打开文件
    with open(file_path, 'r') as file:
        # 逐行读取文件内容
        for line in file:
            # 去除行尾换行符
            line = line.strip()
            
            # 遍历每个字符
            for char in line:
                # 更新字符次数
                if char in character_count:
                    character_count[char] += 1
                else:
                    character_count[char] = 1
    
    return character_count

# 替换为你的文件路径
file_path = 'dnsrelay.txt'
result = count_characters(file_path)

# 输出每个字符的次数
for char, count in result.items():
    print(f'{char}: {count}')