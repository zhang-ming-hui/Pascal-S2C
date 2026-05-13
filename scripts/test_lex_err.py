#!/bin/python3
import os
import subprocess
import sys

def main():
    # 定义路径
    target_dir = "../tests/errorcases/lexer/"
    executable_path = "../build/pascal_s2c"

    # 路径检查
    if not os.path.isdir(target_dir):
        print(f"错误：目录 {target_dir} 不存在！", file=sys.stderr)
        sys.exit(1)
    if not os.path.isfile(executable_path):
        print(f"错误：可执行文件 {executable_path} 不存在！", file=sys.stderr)
        sys.exit(1)

    print(f"开始遍历目录：{target_dir}\n")

    # 遍历所有文件
    for filename in os.listdir(target_dir):
        file_path = os.path.join(target_dir, filename)
        
        if os.path.isfile(file_path):
            # ====================== 1. 输出文件名 ======================
            print(f"===== 文件名：{filename} =====")

            # ====================== 2. 输出文件内容 ======================
            print("----- 文件内容 -----")
            try:
                with open(file_path, "r", encoding="utf-8") as f:
                    content = f.read()
                    print(content)
            except Exception as e:
                print(f"读取文件失败：{e}")

            # ====================== 3. 运行命令 ======================
            print("----- 执行命令：../build/pascal_s2c -----")
            try:
                result = subprocess.run(
                    [executable_path, file_path],
                    capture_output=True,
                    text=True,
                    encoding='utf-8'
                )
                # 输出命令结果
                if result.stdout:
                    print("标准输出：")
                    print(result.stdout)
                if result.stderr:
                    print("标准错误：")
                    print(result.stderr)
                
                print(f"返回码：{result.returncode}\n")

            except Exception as e:
                print(f"执行命令异常：{e}\n")

if __name__ == "__main__":
    main()
