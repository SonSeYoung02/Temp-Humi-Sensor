from fastapi import FastAPI, HTTPException
import subprocess
import threading
import json

app = FastAPI()

c_proc = None          # C 프로세스 객체
latest_data = None     # 가장 최근 센서 데이터
data_lock = threading.Lock()  # 동시 접근 보호용 락


def reader_thread():
    """
    C 프로그램(stdout)을 계속 읽으면서
    JSON 한 줄씩 파싱해서 latest_data에 저장하는 스레드
    """
    global latest_data, c_proc

    if c_proc is None:
        return

    for line in c_proc.stdout:
        line = line.strip()
        if not line:
            continue

        # stderr 디버그용 출력
        print("[C OUT]", line)

        try:
            data = json.loads(line)
        except json.JSONDecodeError:
            # JSON 아닐 경우 무시
            print("[WARN] JSON decode error, line:", line)
            continue

        with data_lock:
            latest_data = data


@app.on_event("startup")
def start_c_program():
    """
    FastAPI 서버 시작 시 C 프로그램 실행 + reader_thread 시작
    """
    global c_proc
    # TempHumiControl.out 경로는 server.py 기준 상대경로로 맞춰줘
    c_proc = subprocess.Popen(
        ["../TempHumiControl.out"],  # 필요하면 "./TempHumiControl.out" 등으로 수정
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1,
    )
    print("[INFO] C program started, pid =", c_proc.pid)

    # C stderr 출력도 보고 싶으면 별도 스레드로 읽어도 됨 (선택)

    # stdout 읽는 스레드 시작
    t = threading.Thread(target=reader_thread, daemon=True)
    t.start()


@app.on_event("shutdown")
def stop_c_program():
    """
    FastAPI 서버 종료 시 C 프로그램도 같이 종료
    """
    global c_proc
    if c_proc and c_proc.poll() is None:
        print("[INFO] Terminating C program...")
        c_proc.terminate()
        try:
            c_proc.wait(timeout=3)
        except subprocess.TimeoutExpired:
            c_proc.kill()
        print("[INFO] C program stopped")


@app.get("/")
def root():
    return {"message": "Page Running"}


@app.get("/sensor")
def read_sensor():
    """
    가장 최근에 C 프로그램이 준 JSON 데이터를 반환
    """
    global latest_data

    with data_lock:
        if latest_data is None:
            # 아직 C가 아무 데이터도 안 뿌린 상태
            raise HTTPException(status_code=503, detail="Sensor data not ready")
        return latest_data

# 가상환경 실행
# source venv/bin/activate
# 서버 실행
# uvicorn server:app --host 0.0.0.0 --port 8000