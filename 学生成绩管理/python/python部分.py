# app.py
import subprocess
import json
import os
import atexit
from flask import Flask, request, jsonify, render_template

app = Flask(__name__)

# ========== 配置 C++ 可执行文件路径 ==========
# 方案1：绝对路径（你刚才测试成功的那个）
EXE_PATH = r"G:\C++草稿\新内容\学生成绩管理\学生成绩管理系统\x64\Debug\学生成绩管理系统.exe"

# 方案2：如果复制到了当前目录，可以用相对路径
# EXE_PATH = os.path.join(os.path.dirname(__file__), "student_manager.exe")

# ========== 启动 C++ 子进程 ==========
proc = subprocess.Popen(
    [EXE_PATH],
    stdin=subprocess.PIPE,
    stdout=subprocess.PIPE,
    stderr=subprocess.PIPE,
    text=True,
    encoding='gbk',       # Windows 中文控制台用 GBK
    errors='replace'
)

# 确保 Python 退出时关闭 C++ 进程
atexit.register(proc.terminate)

def send_command(cmd: str) -> dict:
    """向 C++ 发送一条命令，返回解析后的 JSON 字典"""
    proc.stdin.write(cmd + "\n")
    proc.stdin.flush()
    line = proc.stdout.readline()
    return json.loads(line)

# ========== API 路由 ==========

@app.route('/')
def index():
    """返回前端页面"""
    return render_template('index.html')

@app.route('/api/student', methods=['POST'])
def add_student():
    data = request.json
    if not data or 'id' not in data or 'name' not in data:
        return jsonify({"status": "error", "message": "缺少 id 或 name"}), 400
    # 如果名字中有空格，用双引号包裹（C++ 会正确解析）
    cmd = f'ADD_STUDENT {data["id"]} "{data["name"]}"'
    return jsonify(send_command(cmd))

@app.route('/api/student/<string:sid>/score', methods=['POST'])
def add_or_modify_score(sid):
    data = request.json
    if not data or 'subject' not in data or 'score' not in data:
        return jsonify({"status": "error", "message": "缺少 subject 或 score"}), 400
    try:
        score = float(data['score'])
    except (ValueError, TypeError):
        return jsonify({"status": "error", "message": "score 必须是数字"}), 400
    # 添加/修改成绩（C++ 里 add_score 和 modify_score 行为一致）
    cmd = f'ADD_SCORE {sid} {data["subject"]} {score}'
    return jsonify(send_command(cmd))

@app.route('/api/student/<string:sid>', methods=['GET'])
def query_student(sid):
    return jsonify(send_command(f'QUERY {sid}'))

@app.route('/api/student/<string:sid>', methods=['DELETE'])
def delete_student(sid):
    return jsonify(send_command(f'DELETE_STUDENT {sid}'))

@app.route('/api/students', methods=['GET'])
def list_students():
    return jsonify(send_command('LIST_STUDENTS'))

@app.route('/api/student/<string:sid>/average', methods=['GET'])
def average_score(sid):
    return jsonify(send_command(f'AVERAGE {sid}'))

# ========== 启动 ==========
if __name__ == '__main__':
    # 让局域网内其他设备也能访问的话，可以 host='0.0.0.0'
    app.run(host='0.0.0.0', port=5000, debug=True)