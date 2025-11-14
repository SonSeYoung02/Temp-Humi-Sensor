#include <stdio.h> // I/O 라이브러리 사용
#include <wiringPi.h> // LED 사용

// 온도 설정
#define TEMP_HOT 30 // 너무 뜨거운 온도(30도)
#define TEMP_COOL 20 // 너무 추운 온도(20도)

// LED 설정
#define LED_PIN_BLUE 17 // 물리적 PIN 11번
#define LED_PIN_LED 22 // 물리적 PIN 15번

typedef struct { // 데이터 구조체로 넘겨주기
    float humidity; // 습도
    float celsius; // 온도
    float fahrenheit;
    int success; // 성공 여부
} DHT22Result;

DHT22Result readDHT22(void); // 함수 정의

int main(void){
    if(wiringPiSetupGpio() == -1){ // GPIO 초기화
        printf("[Error] GPIO 초기화 실패");
        return -1; // 초기화 실패
    }
    const int LED_TABLE[2] = {LED_PIN_BLUE, LED_PIN_LED}; // 핀 테이블

    // LED 핀 출력 설정
    for(int i = 0; i < 2; i++){
        pinMode(LED_TABLE[i], OUTPUT); // 핀 출력으로 설정
        digitalWrite(LED_TABLE[i], LOW); // 초기값 꺼짐
    }

    while(1){
        DHT22Result sensor = readDHT22(); // DHT22값을 구조체를 리턴한 sensor값을 읽기

        if(sensor.success){ // 만약 struct 안에 success가 1이면(값을 읽으면)
            printf("온도: %6.2f *C (%6.2f *F) | 습도: %6.2f \n\n", sensor.celsius, sensor.fahrenheit, sensor.humidity);

            if(sensor.celsius >= TEMP_HOT){
                digitalWrite(LED_TABLE[1], HIGH); // 빨강 LED 켜기
                printf("[info] 온도가 너무 높습니다!\n\n");
            }else if(sensor.celsius <= TEMP_COOL){
                digitalWrite(LED_TABLE[0], HIGH); // 파랑 LED 켜기
                printf("[info] 온도가 너무 낮습니다!\n\n");
            }else{
                digitalWrite(LED_TABLE[0], LOW); // 파랑 LED 끄기
                digitalWrite(LED_TABLE[1], LOW); // 빨강 LED 끄기
            }
        } else {
            printf("[Error] 올바른 데이터가 아닙니다. 다시 시도해주세요.\n\n");
        }
        delay(2000); // 2초 딜레이
    }

    printf("[info] 프로그램이 정상 종료되었습니다.\n");
    return 0; // 정상 종료
}
