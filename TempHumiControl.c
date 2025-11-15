#include <stdio.h> // I/O 라이브러리 사용
#include <wiringPi.h> // LED 사용
#include <time.h>

// 온도 설정
#define TEMP_HOT 30 // 너무 뜨거운 온도(30도)
#define TEMP_COOL 25 // 너무 추운 온도(20도)

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
    // 디버그용 시간 배열 생성
    char time_buff[100];

    if(wiringPiSetupGpio() == -1){ // GPIO 초기화
        // 콘솔에만 출력
        fprintf(stderr,"[ERROR] GPIO 초기화 실패");
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
        
        time_t now = time(0);
        strftime (time_buff, 100, "%Y-%m-%d %H:%M:%S", localtime (&now)); // 현재 시간을 버퍼에 저장

        if(sensor.success){ // 만약 struct 안에 success가 1이면(값을 읽으면)
            //json 형대로 출력
            printf("{\"datetime\": \"%s\", \"temp_C\": %.2f, \"temp_F\": %.2f, \"humid_perc\": %.2f}\n", time_buff, sensor.celsius, sensor.fahrenheit, sensor.humidity); //JSON format
            fflush(stdout); // 버퍼 비우기

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
        } else {
            // 디버그용 데이터가 출력된 날짜를 출력한다.
            fprintf(stderr,"datetime: %s\n[ERROR] 온습도 수집 실패\n\n",time_buff);
        }

        delay(2000); // 2초 딜레이
    }
    
    fprintf(stderr,"[INFO] 프로그램이 정상 종료되었습니다.\n");
    return 0; // 정상 종료
}
