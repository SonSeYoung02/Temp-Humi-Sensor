from fastapi import FastAPI, HTTPException
import subprocess
import threading
import json

app = FastAPI()

c_proc = None                   # C 프로세스 객체
latest_data = None              # 가장 최근 센서 데이터
data_lock = threading.Lock()    # 동시 접근 보호용 락


def reader_thread():
    """
    C 프로그램(stdout)을 계속 읽으면서
    JSON 한 줄씩 파싱해서 latest_data에 저장하는 스레드
    """
    global latest_data, c_proc  # 전역변수

    if c_proc is None:  # c_proc이 실행되지 않으면
        return          # 종료

    for line in c_proc.stdout:  # 한줄씩 printf 내용 읽기
        line = line.strip()     # 공백 제거
        if not line:            # 만약 line을 다 읽으면
            continue            # 계속하기

        # stderr 디버그용 출력
        print("[C 출력]", line)

        try:
            data = json.loads(line)     # data에 json 파일을 줄단위로 로드
        except json.JSONDecodeError:    # json 파일에서 오류 난 경우
            # JSON 아닐 경우 무시
            print("[ERROR] JSON 변환 오류, line:", line)
            continue

        with data_lock:
            latest_data = data


@app.on_event("startup")
def start_c_program():
    """
    FastAPI 서버 시작 시 C 프로그램 실행 + reader_thread 시작
    """
    global c_proc
    # TempHumiControl.out 경로는 server.py 기준 상대경로
    c_proc = subprocess.Popen(      # 프로세스 관리 모듈
        ["../TempHumiControl.out"], # 사용할 파일 입력
        stdout=subprocess.PIPE,     # 표준 출력에 연결
        stderr=subprocess.PIPE,     # 표준 에러에 연결
        text=True,                  # 텍스트 모드
        bufsize=1,                  # 버퍼 크기 1
    )
    print("[INFO] C 프로그램 시작, pid =", c_proc.pid)

    # stdout 읽는 스레드 시작
    t = threading.Thread(
        target=reader_thread,   # 스레드 함수명
        daemon=True             # 데몬 스레드 여부(true)
    )
    t.start()   #Thread 시작


@app.on_event("shutdown")
def stop_c_program():
    """
    FastAPI 서버 종료 시 C 프로그램도 같이 종료
    """
    global c_proc
    if c_proc and c_proc.poll() is None:
        print("[INFO] C 프로그램 종료")
        c_proc.terminate() # 외부 프로세스 종료
        try:
            c_proc.wait(timeout=3)  # 3초 이상 기다릴 경우
        except subprocess.TimeoutExpired:
            c_proc.kill()           # 프로세스 종료
        print("[INFO] C 프로그램 종료")


@app.get("/") # 메인 화면
def root():
    return {"message": "페이지 실행"}


@app.get("/sensor")
def read_sensor():
    """
    가장 최근에 C 프로그램이 준 JSON 데이터를 반환
    """
    global latest_data

    with data_lock:
        if latest_data is None:
            raise HTTPException(
                status_code=503, 
                detail="센서 데이터가 준비되지 않았습니다."
            )
        return latest_data