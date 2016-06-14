#include <SPFD5408_Adafruit_GFX.h>    // Libreria para pintar sobre la pantalla
#include <SPFD5408_Adafruit_TFTLCD.h> // LIbraria donde se definen caracteristicas de las pantallas
#include <SPFD5408_TouchScreen.h>     // Libreria para poder reconocer el sensor Touch de las pantallas LCD
#include <EEPROM.h>                   // Libreria para modificar y guardar datos en la memoria EEPROM del Arduino Uno

// These are the pins for the shield!
#define YP A3  // must be an analog pin, use "An" notation!
#define XM A2  // must be an analog pin, use "An" notation!
#define YM 9   // can be a digital pin
#define XP 8   // can be a digital pin

TouchScreen ts = TouchScreen(XP, YP, XM, YM, 320);

#define LCD_CS A3
#define LCD_CD A2
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4  // Boton de reset que está definido al pin analógico 4 de la placa

// Asignacion de nombres de los colores de 16-bits
#define NEGRO     0x0000
#define AZUL      0x002F
#define ROJO      0xF800
#define VERDE     0x07E0
#define CYAN      0x07FF
#define MAGENTA   0xF81F
#define AMARILLO  0xFFE0
#define BLANCO    0xFFFF

#define PRECISIONMIN 10
#define PRECISIONMAX 1000

#define INTERVALO_PINTAR 50

#define DIRECCION_PUNTUACION 0

Adafruit_TFTLCD tft(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET);

int ala;
int fx, fy, cantidadCaida;
int posicionTubo, posicionLibre;
int puntuacion;
int puntuacionMax = 0;
bool corriendo = false;
bool choque = false;
bool pantallaPres = false;
long nextDrawLoopRunTime;

void setup(void) {
  tft.reset();
  tft.begin(0x9341);    // Codigo de identificacion, este depende del modelo de la pantalla.
  tft.setRotation(3);
  
  // Presionar superior derecha de la pantalla para limpiar puntuaciones
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);
  if (p.x < PRECISIONMIN && p.y < PRECISIONMAX)
    EEPROM.write(DIRECCION_PUNTUACION, 0);
  iniciarJuego();

}

void iniciarJuego() {
  fx=50;
  fy=125;
  cantidadCaida=1;
  posicionTubo = 320;
  posicionLibre = 60;
  choque = false;
  puntuacion = 0;
  puntuacionMax = EEPROM.read(DIRECCION_PUNTUACION);
  
  tft.fillScreen(AZUL);
  tft.setTextColor(BLANCO);
  tft.setTextSize(2);
  tft.setCursor(3, 3);
  tft.println("Flappy-duino:");
  tft.println("Toque para iniciar");
  
  tft.setTextColor(VERDE);
  tft.setCursor(3 , 55);
  tft.print("Puntuacion mas alta: ");
  tft.print(puntuacionMax);

  // Pintar piso 
  int ty = 230;
  for (int tx = 0; tx <= 300; tx +=20) {
    tft.fillTriangle(tx,ty, tx+10,ty, tx,ty+10, VERDE);
    tft.fillTriangle(tx+10,ty+10, tx+10,ty, tx,ty+10, AMARILLO);
    tft.fillTriangle(tx+10,ty, tx+20,ty, tx+10,ty+10, AMARILLO);
    tft.fillTriangle(tx+20,ty+10, tx+20,ty, tx+10,ty+10, VERDE);
  }

  nextDrawLoopRunTime = millis() + INTERVALO_PINTAR;
}

void loop(){
  if (millis() > nextDrawLoopRunTime && !choque) {
      drawLoop();
        revisarChoque();
      nextDrawLoopRunTime += INTERVALO_PINTAR;
  }
  
  // Lectura de Touch
  digitalWrite(13, HIGH);
  TSPoint p = ts.getPoint();
  digitalWrite(13, LOW);
  pinMode(XM, OUTPUT);
  pinMode(YP, OUTPUT);

  // Cuando el usuario presiona
  if (p.z > PRECISIONMIN && p.z < PRECISIONMAX && !pantallaPres) {
    if (choque) {
      // reiniciar juego
      iniciarJuego();
    }
    else if (!corriendo) {
      // Limpiar texto e iniciar juego
      tft.fillRect(0,0,320,80,AZUL);
      corriendo = true;
    }
    else
    {
      // Vuelo hacia arriba
      cantidadCaida = -8;
      pantallaPres = true;
    }
  }
  else if (p.z == 0 && pantallaPres) {
    // Deshabilitar cuano cae
    pantallaPres = false;
  }
}

void drawLoop() {
  // limpiar movimientos de objetos
  quitarTubo(posicionTubo, posicionLibre);
  quitarFlappy(fx, fy);

  // mover objetos
  if (corriendo) {
    fy += cantidadCaida;
    cantidadCaida++;
  
    posicionTubo -=5;
    if (posicionTubo == 0) {
      puntuacion++;
    }
    else if (posicionTubo < -50) {
      posicionTubo = 320;
      posicionLibre = random(20, 120);
    }
  }

  // pintar el movimiento de los objetos
  pintarTubo(posicionTubo, posicionLibre);
  dibujarFlappy(fx, fy);
  switch (ala) {
      case 0: case 1: pintarAla1(fx, fy); break;
      case 2: case 3: pintarAla2(fx, fy); break;
      case 4: case 5: pintarAla3(fx, fy); break;
  }
  ala++;
  if (ala == 6  ) ala = 0;
}

void revisarChoque() {
  // Choque con el suelo
  if (fy > 206) choque = true;
  
  // Choque con pilar
  if (fx + 34 > posicionTubo && fx < posicionTubo + 50)
    if (fy < posicionLibre || fy + 24 > posicionLibre + 90)
      choque = true;
  
  if (choque) {
    tft.setTextColor(ROJO);
    tft.setTextSize(2);
    tft.setCursor(75, 75);
    tft.print("Juego terminado!");
    tft.setCursor(75, 125);
    tft.print("Puntuacion:");
    tft.setCursor(220, 125);
    tft.print(puntuacion);
    
    if (puntuacion > puntuacionMax) {
      puntuacionMax = puntuacion;
      EEPROM.write(DIRECCION_PUNTUACION, puntuacionMax);
      tft.setCursor(75, 175);
      tft.print("Nuevo record!");
    }

    // Detener juego
    corriendo = false;
    
    // Tiempo a esperar para reiniciar el juego automatico
    delay(1000);
  }
}

void pintarTubo(int x, int gap) {
  tft.fillRect(x+2, 2, 46, gap-4, VERDE);
  tft.fillRect(x+2, gap+92, 46, 136-gap, VERDE);
  
  tft.drawRect(x,0,50,gap,NEGRO);
  tft.drawRect(x+1,1,48,gap-2,NEGRO);
  tft.drawRect(x, gap+90, 50, 140-gap, NEGRO);
  tft.drawRect(x+1,gap+91 ,48, 138-gap, NEGRO);
}

void quitarTubo(int x, int gap) {
  tft.fillRect(x+45, 0, 5, gap, AZUL);
  tft.fillRect(x+45, gap+90, 5, 140-gap, AZUL);
}

void quitarFlappy(int x, int y) {
 tft.fillRect(x, y, 34, 24, AZUL); 
}

void dibujarFlappy(int x, int y) {
  // Cuerpo
  tft.fillRect(x+2, y+8, 2, 10, NEGRO);
  tft.fillRect(x+4, y+6, 2, 2, NEGRO);
  tft.fillRect(x+6, y+4, 2, 2, NEGRO);
  tft.fillRect(x+8, y+2, 4, 2, NEGRO);
  tft.fillRect(x+12, y, 12, 2, NEGRO);
  tft.fillRect(x+24, y+2, 2, 2, NEGRO);
  tft.fillRect(x+26, y+4, 2, 2, NEGRO);
  tft.fillRect(x+28, y+6, 2, 6, NEGRO);
  tft.fillRect(x+10, y+22, 10, 2, NEGRO);
  tft.fillRect(x+4, y+18, 2, 2, NEGRO);
  tft.fillRect(x+6, y+20, 4, 2, NEGRO);
  
  // Pintar cuerpo
  tft.fillRect(x+12, y+2, 6, 2, AMARILLO);
  tft.fillRect(x+8, y+4, 8, 2, AMARILLO);
  tft.fillRect(x+6, y+6, 10, 2, AMARILLO);
  tft.fillRect(x+4, y+8, 12, 2, AMARILLO);
  tft.fillRect(x+4, y+10, 14, 2, AMARILLO);
  tft.fillRect(x+4, y+12, 16, 2, AMARILLO);
  tft.fillRect(x+4, y+14, 14, 2, AMARILLO);
  tft.fillRect(x+4, y+16, 12, 2, AMARILLO);
  tft.fillRect(x+6, y+18, 12, 2, AMARILLO);
  tft.fillRect(x+10, y+20, 10, 2, AMARILLO);
  
  // Ojo
  tft.fillRect(x+18, y+2, 2, 2, NEGRO);
  tft.fillRect(x+16, y+4, 2, 6, NEGRO);
  tft.fillRect(x+18, y+10, 2, 2, NEGRO);
  tft.fillRect(x+18, y+4, 2, 6, BLANCO);
  tft.fillRect(x+20, y+2, 4, 10, BLANCO);
  tft.fillRect(x+24, y+4, 2, 8, BLANCO);
  tft.fillRect(x+26, y+6, 2, 6, BLANCO);
  tft.fillRect(x+24, y+6, 2, 4, NEGRO);
  
  // Ala
  tft.fillRect(x+20, y+12, 12, 2, NEGRO);
  tft.fillRect(x+18, y+14, 2, 2, NEGRO);
  tft.fillRect(x+20, y+14, 12, 2, ROJO);
  tft.fillRect(x+32, y+14, 2, 2, NEGRO);
  tft.fillRect(x+16, y+16, 2, 2, NEGRO);
  tft.fillRect(x+18, y+16, 2, 2, ROJO);
  tft.fillRect(x+20, y+16, 12, 2, NEGRO);
  tft.fillRect(x+18, y+18, 2, 2, NEGRO);
  tft.fillRect(x+20, y+18, 10, 2, ROJO);
  tft.fillRect(x+30, y+18, 2, 2, NEGRO);
  tft.fillRect(x+20, y+20, 10, 2, NEGRO);
}

// Ala abajo
void pintarAla1(int x, int y) {
  tft.fillRect(x, y+14, 2, 6, NEGRO);
  tft.fillRect(x+2, y+20, 8, 2, NEGRO);
  tft.fillRect(x+2, y+12, 10, 2, NEGRO);
  tft.fillRect(x+12, y+14, 2, 2, NEGRO);
  tft.fillRect(x+10, y+16, 2, 2, NEGRO);
  tft.fillRect(x+2, y+14, 8, 6, BLANCO);
  tft.fillRect(x+8, y+18, 2, 2, NEGRO);
  tft.fillRect(x+10, y+14, 2, 2, BLANCO);
}

// Ala en medio
void pintarAla2(int x, int y) {
  tft.fillRect(x+2, y+10, 10, 2, NEGRO);
  tft.fillRect(x+2, y+16, 10, 2, NEGRO);
  tft.fillRect(x, y+12, 2, 4, NEGRO);
  tft.fillRect(x+12, y+12, 2, 4, NEGRO);
  tft.fillRect(x+2, y+12, 10, 4, BLANCO);
}

// Ala arriba
void pintarAla3(int x, int y) {
  tft.fillRect(x+2, y+6, 8, 2, NEGRO);
  tft.fillRect(x, y+8, 2, 6, NEGRO);
  tft.fillRect(x+10, y+8, 2, 2, NEGRO);
  tft.fillRect(x+12, y+10, 2, 4, NEGRO);
  tft.fillRect(x+10, y+14, 2, 2, NEGRO);
  tft.fillRect(x+2, y+14, 2, 2, NEGRO);
  tft.fillRect(x+4, y+16, 6, 2, NEGRO);
  tft.fillRect(x+2, y+8, 8, 6, BLANCO);
  tft.fillRect(x+4, y+14, 6, 2, BLANCO);
  tft.fillRect(x+10, y+10, 2, 4, BLANCO);
}

