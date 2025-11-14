from fastapi import FastAPI # fastapi 사용
import subprocess # C로 만든 실행 파일 보기
import json # json으로 만들기

app = FastAPI() # FastAPI 인스턴스 생성

@app.get("/") # / 페이지 생성
def read_root():
    return{"message : Page Running"} # 실행되고 있는지 확인

@app.get("/sensor") # sensor 페이지로 이동
def read_sensor():
    # C 센서 프로그램 실행
    result = subprocess.run( # 만약 정상적으로 c파일이 실행 중 이면
        ["../TempHumiControl.out"], # 바이너리 파일 경로
        capture_output=True, 
        text=True
    )

    stdout = result.stdout.strip() # C 파일의 온습도 데이터 문자열 공백 제거
    stderr = result.stderr.strip() # C 파일의 로그 데이터 공백 제거

    # 실행 실패 시
    if result.returncode != 0:
        return {
            "success": False, # 실패시
            "error": "C_PROGRAM_ERROR",
            "returncode": result.returncode,
            "stderr": stderr,
            "stdout": stdout,
        }

    # JSON 파싱 시도
    try:
        data = json.loads(stdout)
        return {
            "success": True,
            "data": data
        }
    except json.JSONDecodeError:
        return {
            "success": False,
            "error": "JSON_PARSE_ERROR",
            "stdout": stdout,
            "stderr": stderr,
        }

# 가상환경 실행
# source venv/bin/activate
# 서버 실행
# uvicorn server:app --host 0.0.0.0 --port 8000