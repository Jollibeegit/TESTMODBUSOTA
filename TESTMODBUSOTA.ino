#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include <ModbusRTU.h>

#define RS485_TX 17
#define RS485_RX 16
#define RS485_DE 12

const char* ssid = "mndsystem_2.4G";
const char* password = "01099574832";

// 📌 현재 펌웨어 버전
#define FIRMWARE_VERSION "1.0.0"

// GitHub에서 OTA 체크용 경로
const char* versionURL  = "https://raw.githubusercontent.com/Jollibeegit/TESTMODBUSOTA/main/version.txt";
const char* firmwareURL = "https://raw.githubusercontent.com/Jollibeegit/TESTMODBUSOTA/main/firmware.ino.bin";

// 📦 Modbus 객체 및 레지스터
ModbusRTU mb;
uint16_t inputRegs[3] = {111, 222, 333}; // 30001 ~ 30003용 데이터

void setup() {
  Serial.begin(9600);
  Serial1.begin(9600, SERIAL_8N1, RS485_RX, RS485_TX);
  pinMode(RS485_DE, OUTPUT);
  digitalWrite(RS485_DE, LOW);

  // Modbus 슬레이브 초기화
  mb.begin(&Serial1, RS485_DE);
  mb.slave(1); // 슬레이브 주소
  mb.addIreg(0x0000, inputRegs[0]); // 30001
  mb.addIreg(0x0001, inputRegs[1]); // 30002
  mb.addIreg(0x0002, inputRegs[2]); // 30003

  // Wi-Fi 연결
  WiFi.begin(ssid, password);
  Serial.println("\n📡 Wi-Fi 연결 중...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\n✅ Wi-Fi 연결됨!");
  Serial.print("IP 주소: ");
  Serial.println(WiFi.localIP());

  checkForUpdate(); // OTA 체크
}

void checkForUpdate() {
  HTTPClient http;
  http.begin(versionURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    String newVersion = http.getString();
    newVersion.trim();
    Serial.printf("🔍 최신 버전: %s\n", newVersion.c_str());

    if (newVersion != FIRMWARE_VERSION) {
      Serial.println("🚀 업데이트 시작!");
      performOTA();
    } else {
      Serial.println("✅ 현재 최신 버전입니다.");
    }
  } else {
    Serial.printf("❌ 버전 확인 실패 (%d)\n", httpCode);
  }

  http.end();
}

void performOTA() {
  HTTPClient http;
  http.begin(firmwareURL);
  int httpCode = http.GET();

  if (httpCode == HTTP_CODE_OK) {
    int len = http.getSize();
    WiFiClient& client = http.getStream();

    if (Update.begin(len)) {
      size_t written = Update.writeStream(client);
      if (written == len && Update.end(true)) {
        Serial.println("✅ 업데이트 완료! 재부팅합니다.");
        ESP.restart();
      } else {
        Serial.println("❌ 업데이트 실패!");
      }
    } else {
      Serial.println("❌ Update.begin 실패");
    }
  } else {
    Serial.printf("❌ 펌웨어 다운로드 실패 (%d)\n", httpCode);
  }

  http.end();
}

void loop() {
  mb.task();  // Modbus 처리
  yield();
  delay(100);
}
