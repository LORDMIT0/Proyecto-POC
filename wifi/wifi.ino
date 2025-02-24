#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <addons/TokenHelper.h>

#define WIFI_SSID "A14 de Manuel"
#define WIFI_PASSWORD "holahola"
#define API_KEY "AIzaSyABlCJSWYwDFEPOM-aN4wbgKT3HF1PVFqc"
#define FIREBASE_PROJECT_ID "proyecto-arduino2"
#define USER_EMAIL "manugg006@gmail.com"
#define USER_PASSWORD "power380&"

FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long dataMillis = 0;
int failedAttempts = 0;
const int maxFailedAttempts = 3;

void setup() {
  Serial.begin(9600);
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Conectando a WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nConectado a WiFi");
  Serial.print("Dirección IP: ");
  Serial.println(WiFi.localIP());

  config.api_key = API_KEY;
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;
  config.token_status_callback = tokenStatusCallback;
  
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  Serial.println("Esperando conexión a Firebase...");
  while (!Firebase.ready() && failedAttempts < maxFailedAttempts) {
    Serial.print(".");
    delay(1000);
    failedAttempts++;
  }

  if (Firebase.ready()) {
    Serial.println("\nConexión a Firebase establecida");
  } else {
    Serial.println("\nError al conectar a Firebase. Reiniciando...");
    ESP.restart();
  }
}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');
    if (input.startsWith("B:")) {
      String codigoBarras = input.substring(2);
      buscarProducto(codigoBarras);
    } else if (input.startsWith("C:")) {
      int separatorIndex = input.indexOf('|');
      if (separatorIndex != -1) {
        String codigoBarras = input.substring(2, separatorIndex);
        String cantidad = input.substring(separatorIndex + 1);
        agregarAlCarrito(codigoBarras, cantidad);
      }
    }
  }

  if (millis() - dataMillis > 5000 && Firebase.ready()) {
    dataMillis = millis();
    Serial.println("Firebase sigue conectado");
  }
}

void buscarProducto(String codigo) {
  if (Firebase.ready()) {
    String documentPath = "productos/" + codigo;

    if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str())) {
      FirebaseJson payload;
      payload.setJsonData(fbdo.payload().c_str());
      
      FirebaseJsonData jsonData;
      String nombreProducto = "";
      float precioProducto = 0.0;
      
      if (payload.get(jsonData, "fields/nombre/stringValue")) {
        nombreProducto = jsonData.stringValue;
      }
      
      if (payload.get(jsonData, "fields/precio/doubleValue")) {
        precioProducto = jsonData.floatValue;
      } else if (payload.get(jsonData, "fields/precio/integerValue")) {
        precioProducto = jsonData.intValue;
      }

      // Acá envia los datos al Arduino
      Serial.print("N:");
      Serial.print(nombreProducto);
      Serial.print("|P:");
      Serial.println(precioProducto);
    } else {
      Serial.println("N:Producto no encontrado|P:0");
      Serial.printf("Error de Firestore: %s\n", fbdo.errorReason().c_str());
    }
  } else {
    Serial.println("N:Error de conexión|P:0");
  }
}

//Acá agrega los datos al carrito1
void agregarAlCarrito(String codigo, String cantidad) {
  if (Firebase.ready()) {
    FirebaseJson content;
    //para agregarlos al carrito2 solo hay que cambiar el 1 por un 2 en la palabra carrito debajo de este comentario
    String documentPath = "carrito1/" + codigo;
    content.set("fields/cantidad/integerValue", cantidad);

    if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw())) {
      Serial.println("R:OK");
      Serial.printf("Producto agregado al carrito: %s\n", documentPath.c_str());
    } else {
      if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), content.raw(), "cantidad")) {
        Serial.println("R:OK");
        Serial.printf("Producto actualizado en el carrito: %s\n", documentPath.c_str());
      } else {
        Serial.println("R:ERROR");
        Serial.printf("Error de Firestore: %s\n", fbdo.errorReason().c_str());
      }
    }
  } else {
    Serial.println("R:ERROR_CONEXION");
  }
}