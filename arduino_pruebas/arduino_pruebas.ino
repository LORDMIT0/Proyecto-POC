#include <SoftwareSerial.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

SoftwareSerial scannerSerial(A0, A1);
LiquidCrystal_I2C lcd(0x27, 16, 2);

const int btnSi = 2;
const int btnNo = 3;
const int btnNumeros[10] = {4, 5, 6, 7, 8, 9, 10, 11, 12, 13};

String barcode = "";
unsigned long lastCharTime = 0;
const unsigned long charTimeout = 50;

String nombreProducto = "";
float precioProducto = 0.0;
bool esperandoRespuesta = false;

String inputBuffer = "";

bool ingresandoCantidad = false;
String cantidadBuffer = "";
String codigoActual = "";

void setup() {
  Serial.begin(9600); 
  scannerSerial.begin(9600);
  
  lcd.init();
  lcd.backlight();
  inicializarPines();
  mostrarInicio();

  Serial.println("Sistema iniciado");
}

void loop() {
  if (!ingresandoCantidad) {
    manejarLecturaCodigoBarras();
  } else {
    manejarIngresoNumerico();
  }

  manejarBotonSi();
  manejarBotonNo();
  manejarRespuestaESP();
}

void inicializarPines() {
  pinMode(btnSi, INPUT_PULLUP);
  pinMode(btnNo, INPUT_PULLUP);
  for (int i = 0; i < 10; i++) {
    pinMode(btnNumeros[i], INPUT_PULLUP);
  }
}

void manejarLecturaCodigoBarras() {
  if (scannerSerial.available()) {
    char c = scannerSerial.read();
    if (c != '\n' && c != '\r') {
      barcode += c;
      lastCharTime = millis();
    }
  }

  if (barcode.length() > 0 && (millis() - lastCharTime > charTimeout)) {
    procesarCodigo(barcode);
    barcode = "";
  }
}

void manejarIngresoNumerico() {
  for (int i = 0; i < 10; i++) {
    if (digitalRead(btnNumeros[i]) == LOW) {
      if (cantidadBuffer.length() < 3) {
        cantidadBuffer += String(i);
        actualizarLCDCantidad();
      }
      delay(200); 
    }
  }
}

void manejarBotonSi() {
  if (digitalRead(btnSi) == LOW) {
    if (!ingresandoCantidad && nombreProducto.length() > 0) {
      iniciarIngresoCantidad();
    } else if (ingresandoCantidad) {
      finalizarIngresoCantidad();
    } else if (inputBuffer.length() > 0) {
      procesarCodigo(inputBuffer);
      inputBuffer = "";
    }
    delay(200); 
  }
}

void manejarBotonNo() {
  if (digitalRead(btnNo) == LOW) {
    if (ingresandoCantidad) {
      cantidadBuffer = "";
      actualizarLCDCantidad();
    } else {
      mostrarInicio();
      inputBuffer = "";
    }
    delay(200); 
  }
}

void manejarRespuestaESP() {
  if (Serial.available() && esperandoRespuesta) {
    String response = Serial.readStringUntil('\n');
    procesarRespuestaESP(response);
    esperandoRespuesta = false;
  }
}

void procesarCodigo(String codigo) {
  codigoActual = codigo;
  mostrarMensajeTemporario("Buscando...", 0);
  Serial.println("B:" + codigo);
  esperandoRespuesta = true;
}

void procesarRespuestaESP(String response) {
  if (response.startsWith("R:")) {
    if (response.substring(2) == "OK") {
      mostrarMensajeTemporario("Agregado al carrito", 2000);
    } else {
      mostrarMensajeTemporario("Error al agregar", 2000);
    }
    return;
  }

  int separatorIndex = response.indexOf('|');
  if (separatorIndex != -1 && response.startsWith("N:")) {
    nombreProducto = response.substring(2, separatorIndex);
    precioProducto = response.substring(separatorIndex + 3).toFloat();
    actualizarLCD();
  } else {
    mostrarMensajeTemporario("Producto no encontrado", 2000);
  }
}

void iniciarIngresoCantidad() {
  ingresandoCantidad = true;
  cantidadBuffer = "";
  actualizarLCDCantidad();
}

void finalizarIngresoCantidad() {
  if (cantidadBuffer.length() > 0) {
    Serial.println("C:" + codigoActual + "|" + cantidadBuffer);
    esperandoRespuesta = true;
    mostrarMensajeTemporario("Agregando...", 0);
  } else {
    mostrarMensajeTemporario("Cantidad invalida", 2000);
  }
  ingresandoCantidad = false;
  cantidadBuffer = "";
}

void actualizarLCD() {
  lcd.clear();
  if (nombreProducto.length() > 0) {
    lcd.setCursor(0, 0);
    lcd.print(nombreProducto.substring(0, 16));
    lcd.setCursor(0, 1);
    lcd.print("$" + String(precioProducto, 2) + " Agregar?");
  } else if (inputBuffer.length() > 0) {
    lcd.setCursor(0, 0);
    lcd.print("Codigo:");
    lcd.setCursor(0, 1);
    lcd.print(inputBuffer);
  } else {
    mostrarInicio();
  }
}

void actualizarLCDCantidad() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Cantidad:");
  lcd.setCursor(0, 1);
  lcd.print(cantidadBuffer + " Unidades");
}

void mostrarInicio() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Listo para");
  lcd.setCursor(0, 1);
  lcd.print("escanear codigo");
}

void mostrarMensajeTemporario(String mensaje, int duracion) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(mensaje);
  if (duracion > 0) {
    delay(duracion);
    mostrarInicio();
  }
}

