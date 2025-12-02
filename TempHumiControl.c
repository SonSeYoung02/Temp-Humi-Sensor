#include <stdio.h> // I/O 라이브러리 사용
#include <wiringPi.h> // 핀 설정 라이브러리
#include <time.h> // 시간 라이브러리
#include <stdint.h> 
#include <string.h> // 문자열 사용
#include <stdlib.h> 
#include <unistd.h>
#include "oled96.h"

// 온도 설정
#define TEMP_HOT 25 // 뜨거운 온도
#define TEMP_COOL 20 // 차가운 온도

// LED 설정
#define LED_PIN_BLUE 17 // 물리적 PIN 11번
#define LED_PIN_LED 22 // 물리적 PIN 15번

// 습도 설정
#define HUMI_LOW 50.0 // 습도 50% 설정
#define HUMI_ON_TIME 10000 // 가습기 작동 시작(10초)

// 핀설정
#define PIN_POS 23 // B-IA (+(Positive의 약자)로 전류 방출)
#define PIN_NEG 24 // B-IB (-(Negative의 약자)로 Ground 역할)

// 데이터 구조체
typedef struct {
    float humidity;     // 습도
    float celsius;      // 온도
    float fahrenheit;   // 화씨
    int success;        // 성공 여부(1: 성공, 0: 실패)
} DHT22Result;

DHT22Result readDHT22(void); // 함수 정의

int main(int argc, char *argv[]){
    
    // 디버그용 시간 배열 생성
    char time_buff[100];

    // GPIO 초기화
    if(wiringPiSetupGpio() == -1){
        // 콘솔에만 출력
        fprintf(stderr,"[ERROR] GPIO 초기화 실패");
        return -1; // 초기화 실패
    }

    // LED 핀 테이블
    const int LED_TABLE[2] = {
        LED_PIN_BLUE, // index = 0
        LED_PIN_LED // index = 1
    };
    // LED GPIO 핀 초기화
    for(int i = 0; i < 2; i++){
        pinMode(LED_TABLE[i], OUTPUT); // 핀 출력으로 설정
        digitalWrite(LED_TABLE[i], LOW); // 초기값 꺼짐
    }

    // 디스플레이 변수 선언
    int i, iChannel;
    int iOLEDAddr = 0x3c; // 디스플레이 주소 0x3c
    int iOLEDType = OLED_132x64; // 디스플레이 크기 설정(132 * 64 픽셀)
    int bFlip = 0, bInvert = 0; // 화면 상하 반전, 화면 배경 설정(흰 or 검)

    char buf_line_1[20]; // 디스플레이 1번째 줄
    char buf_line_2[20]; // 디스플레이 2번째 줄
    
    i = -1;
    iChannel = -1;
    // I2C 채널 0에서 2까지 시도해 보기(초기화)
    while (i != 0 && iChannel < 2)
    {
        iChannel++;
        i=oledInit(iChannel, iOLEDAddr, iOLEDType, bFlip, bInvert);
    }
    if (i == 0)
    {
        fprintf(stderr,"[INFO] I2C bus 채널 %d\n", iChannel);
        oledFill(0); // 화면 검은색으로 초기화
    }
    else
    {
        fprintf(stderr,"[ERROR] I2C 버스 0-2를 초기화할 수 없음.\n'i2cdetect -y <channel>로 주소 확인\n");
    }

    // 습도 변수 설정
    int humi_status = 0; // 가습기 상태 판별(0: 꺼짐, 1: 커짐)
    unsigned int humi_start_time = 0; // 가습기가 켜지는 시점 

    // 습도 핀 테이블
    const int HUMI_TABLE[2] = {
        PIN_POS, // index = 0
        PIN_NEG // index = 1
    };
    // 습도 GPIO 핀 초기화
    for(int i = 0; i < 2; i++){
        pinMode(HUMI_TABLE[i], OUTPUT); // 핀 출력으로 설정
        digitalWrite(HUMI_TABLE[i], LOW); // 초기값 꺼짐
    }

    while(1){
        DHT22Result sensor = readDHT22(); // DHT22값을 구조체를 리턴한 sensor값을 읽기
        
        time_t now = time(0);
        strftime (time_buff, 100, "%Y-%m-%d %H:%M:%S", localtime (&now)); // 현재 시간을 버퍼에 저장

        if(sensor.success){ // 만약 struct 안에 success가 1이면(값을 읽으면)
            // OLED 출력을 위해 배열에 온습도 값 저장
            snprintf(buf_line_1, sizeof(buf_line_1),"Temp: %.2f",sensor.celsius); 
            snprintf(buf_line_2, sizeof(buf_line_2),"Humi: %.2f",sensor.humidity);
            
            oledFill(0); // oled 잔상 삭제
            oledWriteString(3,3,buf_line_1,FONT_NORMAL); // X,Y 좌표 설정 | 폰트 사이즈
            oledWriteString(3,5,buf_line_2,FONT_NORMAL); // X,Y 좌표 설정 | 폰트 사이즈
    
            //json 형대로 출력
            printf("{\"datetime\": \"%s\", \"temp_C\": %.2f, \"temp_F\": %.2f, \"humid_per\": %.2f}\n", time_buff, sensor.celsius, sensor.fahrenheit, sensor.humidity); //JSON 형식
            fflush(stdout); // 버퍼 비우기

            // LED 제어
            if(sensor.celsius >= TEMP_HOT){
                digitalWrite(LED_TABLE[1], HIGH); // 빨강 LED 켜기
                // 로그와 콘솔을 분리
                fprintf(stderr,"[INFO] 온도가 너무 높습니다!\n\n"); // stderr로 콘솔에서만 보고 json에는 저장되지 않게 한다.
            }else if(sensor.celsius <= TEMP_COOL){
                digitalWrite(LED_TABLE[0], HIGH); // 파랑 LED 켜기
                // 로그와 콘솔을 분리
                fprintf(stderr,"[INFO] 온도가 너무 낮습니다!\n\n");
            }else{
                digitalWrite(LED_TABLE[0], LOW); // 파랑 LED 끄기
                digitalWrite(LED_TABLE[1], LOW); // 빨강 LED 끄기
                fprintf(stderr,"[INFO] 정상 온도\n\n");
            }
            
            unsigned int currentTime = millis(); // 현재 시스템 시간 가져오기

            if(sensor.humidity < HUMI_LOW && humi_start_time == 0){ // 만약 습도가 습도 이하 이면
                fprintf(stderr,"[INFO] 가습기 가동");
                // 전원 켜기
                digitalWrite(PIN_POS, HIGH); // L9110S 모듈의 + 극에 전원 공급 5v
                digitalWrite(PIN_NEG, LOW); // L9110S 모듈의 - 극에 전원 끄기 0v
                
                /* 
                2개의 핀의 전압차로 전기가 흐름
                [5v -> 0v] 
                전류가 높은 곳에서 낮은곳으로 흐르는 원리
                */
                
                humi_status = 1; // 가습기 ON
                humi_start_time = currentTime; // 켜진 상태 시스템 시간으로 기록
            }
            // 가습기가 켜져 있고 and [시스템 시간 - 작동 시작 시간 > 10초 가동] 이면
            if(humi_status == 1 && (currentTime - humi_start_time > HUMI_ON_TIME)){
                fprintf(stderr,"[INFO] 가습기 중지");

                digitalWrite(PIN_POS, LOW); // L9110S 모듈의 + 극 LOW로 꺼주기
                digitalWrite(PIN_NEG, LOW); // L9110S 모듈의 - 극 LOW로 꺼주기

                humi_status = 0; // 다시 가습기 상태 0으로 설정
                humi_start_time = 0; // 측정 시간 0으로 초기화
            }

            // 가습기가 켜진 상태에서 습도가 5% 높아지면
            if(humi_status == 1 && sensor.humidity >= HUMI_LOW + 5){
                fprintf(stderr,"[INFO] 습도 증가로 인한 가습기 중지");

                digitalWrite(PIN_POS, LOW); // L9110S 모듈의 + 극 LOW로 꺼주기
                digitalWrite(PIN_NEG, LOW); // L9110S 모듈의 - 극 LOW로 꺼주기

                humi_status = 0; // 다시 가습기 상태 0으로 설정
                humi_start_time = 0; // 측정 시간 0으로 초기화
            }
            
        } else {
            // 디버그용 데이터가 출력된 날짜를 출력한다.
            fprintf(stderr,"datetime: %s\n[ERROR] 온습도 수집 실패\n\n",time_buff);
        }
        delay(2000); // 2초 딜레이(DHT22 습도 센서 측정 속도 맞춰주기)
    }
    return 0;
}