#include <Servo.h>
#include <math.h>

const int trigPin1 = 9;
const int echoPin1 = 10;
const int trigPin2 = 11;
const int echoPin2 = 12;
const int servoPin = 6;

const float ANCHO_BANDA = 11.0;
const float VELOCIDAD_BANDA = 7.26125;
const float INTERVALO_MEDICION = 0.1;
const float DELTA_X = VELOCIDAD_BANDA * INTERVALO_MEDICION;
const float UMBRAL_OBJETO = 2.5;
const float AREA_MINIMA_SERVO = 4.0; 
const float DISTANCIA_SENSORES_SERVO = 10.0;

const int SERVO_REPOSO = 0;
const int SERVO_RECHAZADAS = 110;
const int TIEMPO_SERVO_GRANDE = 10000; 

Servo servoDesvio;

int numFruta = 0;
float areaAcumulada = 0;
float ancho_anterior = 0;
bool primeraMedicion = true;
bool objetoEnProceso = false;
int conteoMediciones = 0;

float areas[50];
float sumaAreas = 0;
int conteoAreas = 0;

bool servoActivo = false;
unsigned long tiempoServoInicio = 0;
int tiempoServoActual = TIEMPO_SERVO_GRANDE;

void setup() {
  pinMode(trigPin1, OUTPUT);
  pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT);
  pinMode(echoPin2, INPUT);
  
  servoDesvio.attach(servoPin);
  servoDesvio.write(SERVO_REPOSO);
  
  Serial.begin(9600);
  
  Serial.println("=== SISTEMA DE MEDICION Y CLASIFICACION ===");
  Serial.println("Banda transportadora activa");
  Serial.print("Distancia sensores->servo: ");
  Serial.print(DISTANCIA_SENSORES_SERVO);
  Serial.println(" cm");
  Serial.print("Servo ESTATICO si area <= ");
  Serial.print(AREA_MINIMA_SERVO);
  Serial.println(" cm (ACEPTADAS)");
  Serial.print("Servo 110° si area > ");
  Serial.print(AREA_MINIMA_SERVO);
  Serial.println(" cm (RECHAZADAS)");
  Serial.println("Comando 'R' para reiniciar");
  Serial.println();
  Serial.println("Esperando objetos...");
  Serial.println();
}

float medirDistancia(int trigPin, int echoPin) {
  float suma = 0;
  int mediciones = 0;
  
  for(int i = 0; i < 3; i++) {
    digitalWrite(trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigPin, LOW);
    
    long duration = pulseIn(echoPin, HIGH, 40000);
    if (duration > 0) {
      suma += duration * 0.034 / 2;
      mediciones++;
    }
    delay(1);
  }
  
  if (mediciones == 0) return -1;
  return suma / mediciones;
}

void loop() {
  static unsigned long tiempoInicio = millis();
  float tiempo = (millis() - tiempoInicio) / 1000.0;
  
  float distancia1 = medirDistancia(trigPin1, echoPin1);
  delay(20);
  float distancia2 = medirDistancia(trigPin2, echoPin2);
  
  if (distancia1 < 0) distancia1 = ANCHO_BANDA;
  if (distancia2 < 0) distancia2 = ANCHO_BANDA;
  
  if (distancia1 > ANCHO_BANDA) distancia1 = ANCHO_BANDA;
  if (distancia2 > ANCHO_BANDA) distancia2 = ANCHO_BANDA;
  
  float ancho_actual = ANCHO_BANDA - distancia1 - distancia2;
  
  if (ancho_actual < 0) ancho_actual = 0;
  
  bool hayObjeto = (ancho_actual > UMBRAL_OBJETO);
  
  if (hayObjeto) {
    if (!objetoEnProceso) {
      numFruta++;
      areaAcumulada = 0;
      primeraMedicion = true;
      objetoEnProceso = true;
      conteoMediciones = 0;
      
      Serial.println("========================================");
      Serial.print(">>> FRUTA #");
      Serial.print(numFruta);
      Serial.println(" DETECTADA <<<");
      Serial.println("Tiempo(s),D1(cm),D2(cm),Ancho(cm),AreaParc(cm²),AreaTotal(cm²)");
    }
    
    conteoMediciones++;
    
    if (!primeraMedicion) {
      float areaParcial = ((ancho_anterior + ancho_actual) / 2.0) * DELTA_X;
      areaAcumulada += areaParcial;
      
      Serial.print(tiempo, 2); Serial.print(",");
      Serial.print(distancia1, 2); Serial.print(",");
      Serial.print(distancia2, 2); Serial.print(",");
      Serial.print(ancho_actual, 2); Serial.print(",");
      Serial.print(areaParcial, 4); Serial.print(",");
      Serial.println(areaAcumulada, 2);
    } else {
      Serial.print(tiempo, 2); Serial.print(",");
      Serial.print(distancia1, 2); Serial.print(",");
      Serial.print(distancia2, 2); Serial.print(",");
      Serial.print(ancho_actual, 2); Serial.print(",");
      Serial.println("0.0000,0.00");
      primeraMedicion = false;
    }
    
    ancho_anterior = ancho_actual;
    
  } else {
    if (objetoEnProceso) {
      Serial.println("----------------------------------------");
      Serial.print("*** TAMAÑO TOTAL DE LA FRUTA #");
      Serial.print(numFruta);
      Serial.print(": ");
      Serial.print(areaAcumulada, 2);
      Serial.print(" cm (");
      Serial.print(conteoMediciones);
      Serial.println(" mediciones) ***");

      if (areaAcumulada > AREA_MINIMA_SERVO && conteoMediciones >= 2) {
        Serial.println("AREA_MAYOR:1");
        Serial.print("[TAMAÑO > ");
        Serial.print(AREA_MINIMA_SERVO);
        Serial.println(" cm - RECHAZADA - ACTIVANDO SERVO 110°]");
        tiempoServoActual = TIEMPO_SERVO_GRANDE;
        servoActivo = true;
        tiempoServoInicio = millis();
        servoDesvio.write(SERVO_RECHAZADAS);
      }
      else if (areaAcumulada > 0 && conteoMediciones >= 2 && areaAcumulada <= AREA_MINIMA_SERVO) {
        Serial.println("AREA_MENOR:1");
        Serial.print("[TAMAÑO <= ");
        Serial.print(AREA_MINIMA_SERVO);
        Serial.println(" cm - ACEPTADA - SERVO ESTÁTICO (NO SE MUEVE)]");
      } else if (conteoMediciones < 2) {
        Serial.println("[OBJETO NO VÁLIDO - MENOS DE 2 MEDICIONES - SERVO NO ACTIVADO]");
      }
      
      if (conteoAreas < 50) {
        areas[conteoAreas] = areaAcumulada;
        sumaAreas += areaAcumulada;
        conteoAreas++;
        
        float promedio = sumaAreas / conteoAreas;
        Serial.print("    Promedio de ");
        Serial.print(conteoAreas);
        Serial.print(" frutas: ");
        Serial.print(promedio, 2);
        Serial.println(" cm");
        
        if (conteoAreas > 1) {
          float varianza = 0;
          for(int i = 0; i < conteoAreas; i++) {
            float dif = areas[i] - promedio;
            varianza += dif * dif;
          }
          varianza /= conteoAreas;
          float desviacion = sqrt(varianza);
          Serial.print("    Desviación estándar: ±");
          Serial.print(desviacion, 2);
          Serial.print(" cm (");
          Serial.print((desviacion/promedio)*100, 1);
          Serial.println("%)");
        }
      }
      
      Serial.println("========================================");
      Serial.println();
      
      objetoEnProceso = false;
      primeraMedicion = true;
      ancho_anterior = 0;
      conteoMediciones = 0;
    }
  }

  if (servoActivo && millis() - tiempoServoInicio >= tiempoServoActual) {
    servoDesvio.write(SERVO_REPOSO);
    servoActivo = false;
    Serial.println("[SERVO EN REPOSO]");
  }
  
  if (Serial.available() > 0) {
    char comando = Serial.read();
    if (comando == 'R' || comando == 'r') {
      numFruta = 0;
      areaAcumulada = 0;
      primeraMedicion = true;
      objetoEnProceso = false;
      ancho_anterior = 0;
      conteoMediciones = 0;
      tiempoInicio = millis();
      
      sumaAreas = 0;
      conteoAreas = 0;
      
      servoActivo = false;
      servoDesvio.write(SERVO_REPOSO);
      
      Serial.println();
      Serial.println("╔════════════════════════════════════╗");
      Serial.println("║    SISTEMA REINICIADO              ║");
      Serial.println("╚════════════════════════════════════╝");
      Serial.println();
      Serial.println("Esperando objetos...");
      Serial.println();
    }
  }
  
  delay(100);
}