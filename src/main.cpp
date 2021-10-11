#include <Arduino.h>
#include "DHT.h"
#include <TimeLib.h>
#include <TimeAlarms.h>

#define BUTTON_PIN    2   // Pino do botão
#define DHT_PIN       3   // DHT Data  
#define MEM_SIZE      60  // Tamanho da memória
#define REPORT_SIZE   10  // Tamanho do report

// Struct usada para armazenar os dados obtidos pelo sensor
struct record
{
    float temperatura;
    float umidade;
};
typedef struct record Record;

int mem_position = 0; // armazena a próxima posição livre na memória
int mem_full = false; // sinalizada se toda a memómeria livre já foi utilizada
Record records[MEM_SIZE];// armazena os MEM_SIZE dados lidos do sensor 
DHT dht(DHT_PIN, DHT11); // define o pino e o tipo de DHT

// Função auxiliar que calcula o módulo, inclusive de número negativos, para facilitar a navegação no array
int mod(int x, int y){
  return x < 0 ? ((x + 1) % y) + y - 1 : x % y;
}

// Função que efetua a leitura dos sensores e cria um Record
Record record_DHT(){
  Record rec;
  rec.temperatura = dht.readTemperature(); // lê a temperatura em Celsius
  rec.umidade = dht.readHumidity(); // lê a umidade
  return rec;
}

// Função que armazena um Record na memória e atualiza o ponteiro para a próxima posição a ser ocupada
void write_Record(Record rec) {
  records[mem_position] = rec;
  mem_position++;
  if (mem_position == MEM_SIZE) {
    mem_position = 0;
    mem_full = true;
  }
}

// Função que gera o relatório a ser exibido no terminal
void create_report() {
  int limit;
  int registers_in_memory;

  if (mem_full) {
    limit = REPORT_SIZE;
    registers_in_memory = MEM_SIZE;
  } else {
    limit = min(mem_position, REPORT_SIZE);
    registers_in_memory = mem_position;
  }

  Serial.print("Registro(s) em memória: ");
  Serial.print(registers_in_memory);
  Serial.print(" - Exibindo o(s) ");
  Serial.print(limit);
  Serial.println(" último(s) registro(s)");
  Serial.println("= = = = = = = = = = = = = = = = = = = = = = = = = =");
  
  int start_pos = mem_position - limit;
  for(int i = 0; i < limit; i++) {
    Serial.print(i + 1);
    Serial.print("\t  Temperatura: ");
    Record rec = records[mod(start_pos + i, MEM_SIZE)];
    Serial.print(rec.temperatura);
    Serial.print(" °C - Umidade: ");
    Serial.print(rec.umidade);
    Serial.println(" %");
  }
  Serial.println("= = = = = = = = = = = = = = = = = = = = = = = = = =\n");
}

// Função para piscar o led builtin
void blink_builtin(int quantidade, int intervalo){
  for(int i = 0; i < quantidade; i++) {
    digitalWrite(LED_BUILTIN, HIGH);
    Alarm.delay(intervalo);
    digitalWrite(LED_BUILTIN, LOW);
    Alarm.delay(intervalo);
  }
}

// Função que coordena o processo de leitura e gravação dos dados e pisca o led
void read_DHT() {
  Record rec = record_DHT();
  write_Record(rec);
  blink_builtin(2, 100);
}

// Função chamada quando o botão é pressionado e dispara  criação do relatório
void interrupt_report() {
  static unsigned long last_interrupt_time = 0;
  unsigned long interrupt_time = millis();
  // Controle do efeito bounce
  if (interrupt_time - last_interrupt_time > 300) 
  {
    create_report();
  }
  last_interrupt_time = interrupt_time;
}

void setup() {
  // Inicia e configura a Serial
  Serial.begin(9600); // 9600bps
  while(!Serial);

  // Configuração dos modos dos pinos utilizados
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  

  digitalWrite(LED_BUILTIN, LOW); // desliga o led builtin

  setTime(0,0,0,1,1,21); // set time to 0:00:00am Jan 1 2021  
  Alarm.timerRepeat(10, read_DHT); // configura timer periódico (10s)
  Alarm.timerOnce(1, read_DHT); // configura timer único

  // Habilita a interrução
  attachInterrupt(digitalPinToInterrupt(BUTTON_PIN), interrupt_report, RISING);

  dht.begin(); // inicializa o sensor DHT
}

void loop() {
  Alarm.delay(1000);
}