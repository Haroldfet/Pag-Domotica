#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>

// -------- CONFIGURACIÓN --------
const int LED_PIN = 14;

// -------- OBJETOS GLOBALES --------
WebServer server(80);
Preferences preferences;

// -------- VARIABLES --------
String ssidGuardado = "";
String passGuardado = "";

// -------- PROTOTIPOS --------
void conectarWiFi();
void iniciarModoConfig();
void handleRoot();
void handleGuardar();
void handlePanel();
void handleOn();
void handleOff();
void handleResetWiFi();

//  SETUP

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("Iniciando ESP32...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  preferences.begin("wifi", false);
  ssidGuardado = preferences.getString("ssid", "");
  passGuardado = preferences.getString("pass", "");

  if (ssidGuardado != "") {
    Serial.println("WiFi guardado encontrado");
    conectarWiFi();
  } else {
    Serial.println("No hay WiFi guardado");
    iniciarModoConfig();
  }
}



//  LOOP


void loop() {
  server.handleClient();
}



//  CONEXIÓN AUTOMÁTICA


void conectarWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssidGuardado.c_str(), passGuardado.c_str());

  Serial.println("Conectando...");

  int intentos = 0;
  while (WiFi.status() != WL_CONNECTED && intentos < 40) {
    delay(500);
    Serial.print(".");
    intentos++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nConectado!");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    server.on("/",          handlePanel);
    server.on("/panel",     handlePanel);
    server.on("/on",        handleOn);
    server.on("/off",       handleOff);
    server.on("/resetwifi", handleResetWiFi);

    server.begin();

  } else {
    Serial.println("\nFallo al conectar. Activando modo configuración.");
    iniciarModoConfig();
  }
}



//  MODO CONFIGURACIÓN (ACCESS POINT)


void iniciarModoConfig() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("Domotica");

  Serial.println("Modo configuración activo");
  Serial.print("IP AP: ");
  Serial.println(WiFi.softAPIP());

  server.on("/",        handleRoot);
  server.on("/guardar", HTTP_POST, handleGuardar);

  server.begin();
}



//  PÁGINA DE CONFIGURACIÓN WIFI


void handleRoot() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>HACK_WIFI :: SETUP</title>
  <style>

    /* FUENTES EXTERNAS */
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@700&display=swap');

    /* VARIABLES GLOBALES DE COLOR */
    :root {
      --neon-green:  #00ff41;   /* verde neón principal */
      --neon-cyan:   #00fff9;   /* cian para detalles y etiquetas */
      --neon-purple: #bf00ff;   /* morado para el glitch y footer */
      --bg:          #050505;   /* fondo negro  */
      --card-bg:     #07100a;   /* fondo oscuro la tarjeta */
    }

    /*  RESET Y CUERPO */
    * { margin: 0; padding: 0; box-sizing: border-box; }

    body {
      background: var(--bg);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      font-family: 'Share Tech Mono', monospace;
      overflow: hidden;
    }

    /* SCANLINES (EFECTO MONITOR VIEJO)  */
    body::before {
      content: '';
      position: fixed;
      inset: 0;
      background: repeating-linear-gradient(
        0deg,
        transparent,
        transparent 2px,
        rgba(0, 255, 65, 0.03) 2px,
        rgba(0, 255, 65, 0.03) 4px
      );
      pointer-events: none; /* no interfiere con clics */
      z-index: 999;          /* encima de todo */
    }

    /* GLITCH DE PÁGINA COMPLETA */
    .page-glitch {
      position: fixed;
      inset: 0;
      pointer-events: none;
      z-index: 998;
    }
    /* Capa cian - se mueve hacia la izquierda */
    .page-glitch::before {
      content: '';
      position: absolute;
      inset: 0;
      background: rgba(0, 255, 249, 0.04);
      animation: page-glitch-cyan 5s infinite;
      clip-path: polygon(0 0, 100% 0, 100% 0, 0 0); 
    }
    /* Capa morada - se mueve hacia la derecha */
    .page-glitch::after {
      content: '';
      position: absolute;
      inset: 0;
      background: rgba(191, 0, 255, 0.04);
      animation: page-glitch-purple 5s infinite;
      clip-path: polygon(0 0, 100% 0, 100% 0, 0 0);
    }
    /* glitch cian: recorta franjas y las desplaza */
    @keyframes page-glitch-cyan {
      0%,89%,100% { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
      90%  { clip-path: polygon(0 15%,100% 15%,100% 22%,0 22%); transform: translate(-4px, 0); }
      91%  { clip-path: polygon(0 55%,100% 55%,100% 60%,0 60%); transform: translate(4px, 0); }
      92%  { clip-path: polygon(0 30%,100% 30%,100% 38%,0 38%); transform: translate(-2px, 0); }
      93%  { clip-path: polygon(0 70%,100% 70%,100% 80%,0 80%); transform: translate(3px, 0); }
      94%  { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
    }
    /* glitch morado: recorta franjas */
    @keyframes page-glitch-purple {
      0%,91%,100% { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
      92%  { clip-path: polygon(0 40%,100% 40%,100% 48%,0 48%); transform: translate(4px, 0); }
      93%  { clip-path: polygon(0 20%,100% 20%,100% 28%,0 28%); transform: translate(-4px, 0); }
      94%  { clip-path: polygon(0 65%,100% 65%,100% 72%,0 72%); transform: translate(2px, 0); }
      95%  { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
    }

    /* FLASH DE DISTORSIÓN */
    @keyframes page-flash {
      0%,94%,100% { opacity: 0; }
      95% { opacity: 1; }
      96% { opacity: 0; }
      97% { opacity: 0.5; }/* destello */
      98% { opacity: 0; }
    }
    .page-flash {
      position: fixed;
      inset: 0;
      background: linear-gradient(135deg,
        rgba(0,255,65,0.03),
        rgba(0,255,249,0.03),
        rgba(191,0,255,0.03)
      );
      pointer-events: none;
      z-index: 997;
      animation: page-flash 5s infinite;
    }

    /*         TARJETA CENTRAL 
       Contenedor principal del formulario  */
    .card {
      position: relative;
      z-index: 10;
      background: #050505ee;   /* fondo sólido, tapa el canvas */
      border: 1px solid rgba(0, 255, 65, 0.3);
      border-radius: 4px;
      padding: 40px 35px;
      width: 90%;
      max-width: 420px;
      box-shadow:
        0 0 20px rgba(0, 255, 65, 0.15),
        0 0 60px rgba(0, 255, 65, 0.05),
        inset 0 0 20px rgba(0, 255, 65, 0.03);
    }

    /*  ESQUINAS DECORATIVAS DE LA TARJETA */
    .card::before, .card::after {
      content: '';
      position: absolute;
      width: 15px; height: 15px;
      border-color: var(--neon-cyan);
      border-style: solid;
    }
    .card::before { top: -1px; left: -1px; border-width: 2px 0 0 2px; } /* esquina sup-izq */
    .card::after  { bottom: -1px; right: -1px; border-width: 0 2px 2px 0; } /* esquina inf-der */

    /*   ENCABEZADO DE LA TARJETA */
    .header { text-align: center; margin-bottom: 30px; }

    /* Etiqueta pequeña encima del título */
    .tag {
      font-size: 11px;
      color: var(--neon-cyan);
      letter-spacing: 3px;
      margin-bottom: 12px;
      opacity: 0.7;
    }

    /* TÍTULO CON EFECTO GLITCH   */
    .glitch {
      font-family: 'Orbitron', monospace;
      font-size: 22px;
      color: var(--neon-green);
      text-transform: uppercase;
      letter-spacing: 4px;
      position: relative;
      text-shadow: 0 0 10px var(--neon-green), 0 0 30px rgba(0,255,65,0.4);
      animation: flicker 4s infinite;
    }
    .glitch::before, .glitch::after {
      content: attr(data-text); /* duplica el texto del atributo data-text */
      position: absolute;
      top: 0; left: 0;
      width: 100%;
    }
    /* Copia cian - solo franja superior */
    .glitch::before {
      color: var(--neon-cyan);
      animation: glitch-1 3s infinite;
      clip-path: polygon(0 0, 100% 0, 100% 40%, 0 40%);
    }
    /* Copia morada - solo franja inferior */
    .glitch::after {
      color: var(--neon-purple);
      animation: glitch-2 3s infinite;
      clip-path: polygon(0 60%, 100% 60%, 100% 100%, 0 100%);
    }
    @keyframes glitch-1 {
      0%,94%,100% { transform: translate(0); opacity: 0; }
      95% { transform: translate(-3px, 1px); opacity: 0.8; }
      97% { transform: translate(3px, -1px); opacity: 0.8; }
    }
    @keyframes glitch-2 {
      0%,94%,100% { transform: translate(0); opacity: 0; }
      96% { transform: translate(3px, 2px); opacity: 0.8; }
      98% { transform: translate(-3px, -2px); opacity: 0.8; }
    }
    /* Parpadeo del texto principal */
    @keyframes flicker {
      0%,95%,100% { opacity: 1; }
      96% { opacity: 0.6; }
      97% { opacity: 1; }
      98% { opacity: 0.4; }
      99% { opacity: 1; }
    }

    /*  EFECTO TYPEWRITER (TEXTO QUE SE ESCRIBE SOLO) */
    .typewriter {
      font-size: 12px;
      color: rgba(0, 255, 65, 0.6);
      margin-top: 8px;
      overflow: hidden;
      white-space: nowrap;
      border-right: 2px solid var(--neon-green); /* cursor */
      width: 0;
      animation:
        type  2s steps(30) 0.5s forwards, /* escribe el texto */
        blink 0.8s step-end infinite;      /* parpadea el cursor */
    }
    @keyframes type  { to { width: 100%; } }
    @keyframes blink { 50% { border-color: transparent; } }

    /*  FORMULARIO */
    .form-group { margin-bottom: 22px; }

    label {
      display: block;
      font-size: 11px;
      color: var(--neon-cyan);
      letter-spacing: 2px;
      margin-bottom: 8px;
      text-transform: uppercase;
    }

    /* Contenedor con símbolo ">" estilo terminal a la izquierda */
    .input-wrap {
      position: relative;
      display: flex;
      align-items: center;
    }
    .input-wrap::before {
      content: '>';
      position: absolute;
      left: 12px;
      color: var(--neon-green);
      font-size: 14px;
      pointer-events: none;
    }

    input {
      width: 100%;
      padding: 12px 12px 12px 30px;
      background: rgba(0, 255, 65, 0.05);
      border: 1px solid rgba(0, 255, 65, 0.3);
      border-radius: 2px;
      color: var(--neon-green);
      font-family: 'Share Tech Mono', monospace;
      font-size: 14px;
      outline: none;
      transition: all 0.3s;
    }
    input::placeholder { color: rgba(0, 255, 65, 0.25); }
    /* Al hacer foco cambia a cian con brillo */
    input:focus {
      border-color: var(--neon-cyan);
      background: rgba(0, 255, 249, 0.05);
      box-shadow: 0 0 15px rgba(0, 255, 249, 0.2),
                  inset 0 0 10px rgba(0, 255, 249, 0.05);
      color: var(--neon-cyan);
    }

    /*        BOTÓN DE ENVÍO     */
    .btn {
      width: 100%;
      padding: 14px;
      background: transparent;
      border: 1px solid var(--neon-green);
      color: var(--neon-green);
      font-family: 'Orbitron', monospace;
      font-size: 13px;
      letter-spacing: 3px;
      text-transform: uppercase;
      cursor: pointer;
      position: relative;
      overflow: hidden;
      transition: all 0.3s;
      border-radius: 2px;
      text-shadow: 0 0 8px var(--neon-green);
      box-shadow: 0 0 15px rgba(0,255,65,0.2);
      animation: btn-pulse 2s ease-in-out infinite;
    }
    @keyframes btn-pulse {
      0%,100% { box-shadow: 0 0 15px rgba(0,255,65,0.2); }
      50%      { box-shadow: 0 0 30px rgba(0,255,65,0.5), 0 0 60px rgba(0,255,65,0.1); }
    }
    /* Destello que cruza de izquierda a derecha al hacer hover */
    .btn::before {
      content: '';
      position: absolute;
      top: 0; left: -100%;
      width: 100%; height: 100%;
      background: linear-gradient(90deg, transparent, rgba(0,255,65,0.15), transparent);
      transition: left 0.4s;
    }
    .btn:hover { background: rgba(0,255,65,0.1); color: #fff; }
    .btn:hover::before { left: 100%; }

    /*        FOOTER     */
    .footer {
      margin-top: 25px;
      text-align: center;
      font-size: 10px;
      color: rgba(0, 255, 65, 0.25);
      letter-spacing: 2px;
    }
    .footer span { color: var(--neon-purple); }

    /* CANVAS DE FONDO (CORRUPCIÓN DE DATOS)   */
    .corrupt-zone {
      position: fixed;
      pointer-events: none;
      z-index: 1;
      opacity: 0.6;
    }
    .fullscreen { top: 0; left: 0; width: 100vw; height: 100vh; }
    .fullscreen canvas { width: 100vw; height: 100vh; }

  </style>
</head>
<body>

  <!-- CAPAS DE GLITCH SOBRE TODA LA PÁGINA -->
  <div class="page-glitch"></div>  <!-- distorsión de franjas cian/morado -->
  <div class="page-flash"></div>   <!-- flash de parpadeo general -->

  <!-- CANVAS DE FONDO: bloques pixelados y números cayendo -->
  <div class="corrupt-zone fullscreen">
    <canvas id="canvas-bg"></canvas>
  </div>

  <script>
    /*     FUNCIÓN drawCorrupt      */
    function drawCorrupt(canvasId, colorMain, colorAlt) {

      const canvas = document.getElementById(canvasId);

      // Ajustar canvas al tamaño de la ventana
      canvas.width  = window.innerWidth;
      canvas.height = window.innerHeight;
      window.addEventListener('resize', () => {
        canvas.width  = window.innerWidth;
        canvas.height = window.innerHeight;
      });

      const ctx   = canvas.getContext('2d');
      const W     = canvas.width;
      const H     = canvas.height;
      const BLOCK = 6; // tamaño en píxeles de cada bloque

      const cols = Math.floor(W / BLOCK);
      const rows = Math.floor(H / BLOCK);

      // Grilla de bloques: 1=visible, 0=oculto (8% de densidad inicial)
      const blocks = [];
      for (let r = 0; r < rows; r++) {
        blocks[r] = [];
        for (let c = 0; c < cols; c++) {
          blocks[r][c] = Math.random() < 0.08 ? 1 : 0;
        }
      }

      // 25 números flotantes con posición, valor y velocidad aleatorios
      const nums = [];
      for (let i = 0; i < 25; i++) {
        nums.push({
          x:       Math.random() * W,
          y:       Math.random() * H,
          val:     Math.floor(Math.random() * 9999999999),
          speed:   0.4 + Math.random() * 0.8,
          opacity: 0.15 + Math.random() * 0.35
        });
      }

      // Loop de animación (~60fps)
      function draw() {
        ctx.clearRect(0, 0, W, H);

        // --- DIBUJAR BLOQUES ---
        for (let r = 0; r < rows; r++) {
          for (let c = 0; c < cols; c++) {
            // Cambio aleatorio de estado
            if (Math.random() < 0.005)
              blocks[r][c] = Math.random() < 0.08 ? 1 : 0;

            if (blocks[r][c]) {
              const rand = Math.random();
              if (rand < 0.5)      ctx.fillStyle = colorMain;  // verde
              else if (rand < 0.8) ctx.fillStyle = colorAlt;   // cian
              else                 ctx.fillStyle = '#ffffff22'; // blanco tenue
              ctx.fillRect(c * BLOCK, r * BLOCK, BLOCK - 1, BLOCK - 1);
            }
          }
        }

        // --- DIBUJAR NÚMEROS ---
        ctx.font = '9px Share Tech Mono, monospace';
        nums.forEach(n => {
          ctx.globalAlpha = n.opacity;
          ctx.fillStyle   = colorMain;
          ctx.fillText(n.val, n.x, n.y);

          n.y += n.speed; // caída hacia abajo

          // Reiniciar si sale de pantalla
          if (n.y > H + 10) {
            n.y   = -10;
            n.x   = Math.random() * (W - 80);
            n.val = Math.floor(Math.random() * 9999999999);
          }
          // Cambiar valor aleatoriamente
          if (Math.random() < 0.03)
            n.val = Math.floor(Math.random() * 9999999999);
        });

        ctx.globalAlpha = 1;
        requestAnimationFrame(draw); // siguiente frame
      }

      draw();
    }

    // Iniciar animación con verde neón y cian
    drawCorrupt('canvas-bg', '#00ff41cc', '#00fff977');
  </script>

  <!-- TARJETA CENTRAL DEL FORMULARIO -->
  <div class="card">

    <!-- Encabezado con título glitch y typewriter -->
    <div class="header">
      <p class="tag">[ SISTEMA DOMÓTICO v1.0 ]</p>
      <h1 class="glitch" data-text="Configuracion de Wifi">Configuracion de Wifi</h1>
      <p class="typewriter">// Ingresa credenciales de red...</p>
    </div>

    <!-- Formulario POST al ESP32 -->
    <form action="/guardar" method="POST">

      <!-- Campo nombre de red -->
      <div class="form-group">
        <label>// Nombre de red (SSID)</label>
        <div class="input-wrap">
          <input type="text" name="ssid" placeholder="nombre_red" required>
        </div>
      </div>

      <!-- Campo contraseña -->
      <div class="form-group">
        <label>// Contraseña</label>
        <div class="input-wrap">
          <input type="password" name="pass" placeholder="**********" required>
        </div>
      </div>

      <button type="submit" class="btn">[ CONECTAR ]</button>

    </form>

    <div class="footer">
      <span>© 2026 :: Proyecto Ing. Software :: ESP32</span>
    </div>

  </div>

</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
}



//  GUARDAR Y REINICIAR


void handleGuardar() {
  ssidGuardado = server.arg("ssid");
  passGuardado = server.arg("pass");

  if (ssidGuardado == "" || passGuardado == "") {
    server.send(400, "text/html",
      "<h2 style='text-align:center; margin-top:40px; color:red;'>"
      "Error: SSID o contraseña vacíos.</h2>");
    return;
  }

  preferences.putString("ssid", ssidGuardado);
  preferences.putString("pass", passGuardado);
  preferences.end();

  server.send(200, "text/html",
    "<h2 style='text-align:center; margin-top:40px;'>"
    "WiFi guardado! Reiniciando...</h2>");

  delay(2000);
  ESP.restart();
}



//  PANEL DE CONTROL


void handlePanel() {
  String estado = digitalRead(LED_PIN) ? "ENCENDIDO" : "APAGADO";

  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Panel Domótico</title>
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
      font-family: 'Segoe UI', Tahoma;
    }
    body {
      background: #111;
      color: white;
      display: flex;
      justify-content: center;
      align-items: center;
      height: 100vh;
    }
    .card {
      background: #222;
      padding: 40px;
      border-radius: 20px;
      text-align: center;
      box-shadow: 0 20px 60px rgba(0, 0, 0, 0.5);
    }
    h1 { margin-bottom: 10px; }
    p  { margin-bottom: 25px; color: #aaa; }
    button {
      padding: 15px 30px;
      margin: 10px;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: bold;
      color: white;
      cursor: pointer;
    }
    .on    { background: #28a745; }
    .off   { background: #dc3545; }
    .reset { background: #ff9800; margin-top: 20px; }
  </style>
</head>
<body>
  <div class="card">
    <h1>Panel Domótico</h1>
    <p>Estado actual: %ESTADO%</p>

    <a href="/on"><button  class="on">ENCENDER</button></a>
    <a href="/off"><button class="off">APAGAR</button></a>

    <br>

    <a href="/resetwifi">
      <button class="reset"
        onclick="return confirm('¿Eliminar WiFi guardado?');">
        ELIMINAR WIFI
      </button>
    </a>
  </div>
</body>
</html>
)rawliteral";

  html.replace("%ESTADO%", estado);
  server.send(200, "text/html", html);
}



//  BORRAR WIFI Y REINICIAR


void handleResetWiFi() {
  preferences.clear();
  preferences.end();

  server.send(200, "text/html", R"rawliteral(
<!DOCTYPE html>
<html lang="es">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <meta http-equiv="refresh" content="5;url=/">
  <title>CRITICAL ERROR :: RESET</title>
  <style>
    @import url('https://fonts.googleapis.com/css2?family=Share+Tech+Mono&family=Orbitron:wght@700&display=swap');

    /*     VARIABLES - paleta rojo alerta */
    :root {
      --red:        #ff0033;
      --red-dim:    #aa0022;
      --red-glow:   rgba(255, 0, 51, 0.4);
      --orange:     #ff4400;
      --bg:         #050000;
    }

    * { margin: 0; padding: 0; box-sizing: border-box; }

    body {
      background: var(--bg);
      min-height: 100vh;
      display: flex;
      justify-content: center;
      align-items: center;
      font-family: 'Share Tech Mono', monospace;
      overflow: hidden;
    }

    /*     SCANLINES rojas */
    body::before {
      content: '';
      position: fixed;
      inset: 0;
      background: repeating-linear-gradient(
        0deg,
        transparent,
        transparent 2px,
        rgba(255, 0, 51, 0.03) 2px,
        rgba(255, 0, 51, 0.03) 4px
      );
      pointer-events: none;
      z-index: 999;
    }

    /*   GLITCH DE PÁGINA - franjas rojas y naranja */
    .page-glitch {
      position: fixed;
      inset: 0;
      pointer-events: none;
      z-index: 998;
    }
    .page-glitch::before {
      content: '';
      position: absolute;
      inset: 0;
      background: rgba(255, 0, 51, 0.05);
      animation: page-glitch-red 4s infinite;
      clip-path: polygon(0 0, 100% 0, 100% 0, 0 0);
    }
    .page-glitch::after {
      content: '';
      position: absolute;
      inset: 0;
      background: rgba(255, 68, 0, 0.04);
      animation: page-glitch-orange 4s infinite;
      clip-path: polygon(0 0, 100% 0, 100% 0, 0 0);
    }
    @keyframes page-glitch-red {
      0%,85%,100% { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
      86%  { clip-path: polygon(0 10%,100% 10%,100% 18%,0 18%); transform: translate(-5px, 0); }
      87%  { clip-path: polygon(0 50%,100% 50%,100% 57%,0 57%); transform: translate(5px, 0); }
      88%  { clip-path: polygon(0 75%,100% 75%,100% 82%,0 82%); transform: translate(-3px, 0); }
      89%  { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
    }
    @keyframes page-glitch-orange {
      0%,87%,100% { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
      88%  { clip-path: polygon(0 35%,100% 35%,100% 42%,0 42%); transform: translate(5px, 0); }
      89%  { clip-path: polygon(0 60%,100% 60%,100% 68%,0 68%); transform: translate(-4px, 0); }
      90%  { clip-path: polygon(0 0,100% 0,100% 0,0 0); transform: translate(0); }
    }

    /*  FLASH DE ALERTA - parpadeo rojo más agresivo */
    .page-flash {
      position: fixed;
      inset: 0;
      background: rgba(255, 0, 51, 0.04);
      pointer-events: none;
      z-index: 997;
      animation: alert-flash 4s infinite;
    }
    @keyframes alert-flash {
      0%,90%,100% { opacity: 0; }
      91% { opacity: 1; }
      92% { opacity: 0; }
      93% { opacity: 0.7; }
      94% { opacity: 0; }
      95% { opacity: 0.4; }
      96% { opacity: 0; }
    }

    /* TARJETA - fondo oscuro con borde rojo */
    .card {
      position: relative;
      z-index: 10;
      background: #0a0000ee;
      border: 1px solid rgba(255, 0, 51, 0.5);
      border-radius: 4px;
      padding: 45px 40px;
      width: 90%;
      max-width: 440px;
      text-align: center;
      color: white;
      box-shadow:
        0 0 30px rgba(255, 0, 51, 0.2),
        0 0 80px rgba(255, 0, 51, 0.05),
        inset 0 0 30px rgba(255, 0, 51, 0.03);
    }

    /* Esquinas decorativas en rojo */
    .card::before, .card::after {
      content: '';
      position: absolute;
      width: 18px; height: 18px;
      border-color: var(--red);
      border-style: solid;
    }
    .card::before { top: -1px; left: -1px; border-width: 2px 0 0 2px; }
    .card::after  { bottom: -1px; right: -1px; border-width: 0 2px 2px 0; }

    /* ICONO DE ALERTA - triángulo pulsante */
    .alert-icon {
      font-size: 55px;
      margin-bottom: 20px;
      display: block;
      animation: icon-pulse 1s ease-in-out infinite;
      filter: drop-shadow(0 0 15px var(--red));
    }
    @keyframes icon-pulse {
      0%,100% { transform: scale(1);    filter: drop-shadow(0 0 10px var(--red)); }
      50%      { transform: scale(1.1); filter: drop-shadow(0 0 25px var(--red)); }
    }

    /* ETIQUETA DE ESTADO */
    .status-tag {
      font-size: 11px;
      color: var(--red);
      letter-spacing: 4px;
      margin-bottom: 15px;
      opacity: 0.8;
      animation: blink-tag 1.5s step-end infinite;
    }
    @keyframes blink-tag {
      0%,100% { opacity: 0.8; }
      50%      { opacity: 0.2; }
    }

    /* TÍTULO CON GLITCH ROJO */
    .glitch {
      font-family: 'Orbitron', monospace;
      font-size: 20px;
      color: var(--red);
      text-transform: uppercase;
      letter-spacing: 4px;
      position: relative;
      text-shadow: 0 0 10px var(--red), 0 0 30px var(--red-glow);
      animation: flicker 3s infinite;
      margin-bottom: 10px;
    }
    .glitch::before, .glitch::after {
      content: attr(data-text);
      position: absolute;
      top: 0; left: 0;
      width: 100%;
    }
    .glitch::before {
      color: #ff6600;
      animation: glitch-1 3s infinite;
      clip-path: polygon(0 0, 100% 0, 100% 40%, 0 40%);
    }
    .glitch::after {
      color: #ff0080;
      animation: glitch-2 3s infinite;
      clip-path: polygon(0 60%, 100% 60%, 100% 100%, 0 100%);
    }
    @keyframes glitch-1 {
      0%,92%,100% { transform: translate(0); opacity: 0; }
      93% { transform: translate(-4px, 1px); opacity: 0.9; }
      95% { transform: translate(4px, -1px); opacity: 0.9; }
    }
    @keyframes glitch-2 {
      0%,93%,100% { transform: translate(0); opacity: 0; }
      94% { transform: translate(4px, 2px); opacity: 0.9; }
      96% { transform: translate(-4px, -2px); opacity: 0.9; }
    }
    @keyframes flicker {
      0%,93%,100% { opacity: 1; }
      94% { opacity: 0.5; }
      95% { opacity: 1; }
      96% { opacity: 0.3; }
      97% { opacity: 1; }
    }

    /* Subtítulo */
    .subtitle {
      font-size: 13px;
      color: rgba(255, 0, 51, 0.6);
      margin-bottom: 30px;
      letter-spacing: 1px;
    }

    /* BARRA DE PROGRESO
       Se llena en 5 segundos (mismo tiempo que el meta refresh)*/
    .progress-label {
      font-size: 10px;
      color: rgba(255, 0, 51, 0.5);
      letter-spacing: 2px;
      margin-bottom: 8px;
      text-align: left;
    }
    .progress-wrap {
      background: rgba(255, 0, 51, 0.1);
      border: 1px solid rgba(255, 0, 51, 0.2);
      border-radius: 2px;
      height: 8px;
      overflow: hidden;
      margin-bottom: 20px;
    }
    .progress-bar {
      height: 100%;
      width: 0%;
      border-radius: 2px;
      background: linear-gradient(90deg, var(--red-dim), var(--red), var(--orange));
      animation: fill-bar 5s linear forwards;
      box-shadow: 0 0 10px var(--red), 0 0 20px var(--red-glow);
    }
    @keyframes fill-bar {
      0%   { width: 0%; }
      100% { width: 100%; }
    }

    /* NOTA FINAL */
    .note {
      font-size: 11px;
      color: rgba(255, 0, 51, 0.35);
      letter-spacing: 2px;
    }

    /* FOOTER */
    .footer {
      margin-top: 25px;
      font-size: 10px;
      letter-spacing: 2px;
    }
    .footer span { color: var(--red); opacity: 0.5; }

  </style>
</head>
<body>

  <!-- CAPAS DE GLITCH -->
  <div class="page-glitch"></div>
  <div class="page-flash"></div>

  <!-- TARJETA DE ERROR -->
  <div class="card">

    <!-- Icono y etiqueta de estado -->
    <span class="alert-icon">⚠️</span>
    <p class="status-tag">[ ERROR CRÍTICO :: SISTEMA ]</p>

    <!-- Título con glitch -->
    <h1 class="glitch" data-text="WIFI ELIMINADO">WIFI ELIMINADO</h1>
    <p class="subtitle">// Reiniciando dispositivo...</p>

    <!-- Barra de progreso que se llena en 5s -->
    <p class="progress-label">// REINICIANDO_SISTEMA.exe</p>
    <div class="progress-wrap">
      <div class="progress-bar"></div>
    </div>

    <p class="note">Serás redirigido automáticamente</p>

    <!-- Footer -->
    <div class="footer">
      <span>© 2026 :: Proyecto Ing. Software :: ESP32</span>
    </div>

  </div>

</body>
</html>
)rawliteral");

  delay(2000);
  ESP.restart();
}



//  CONTROL DEL LED


void handleOn() {
  digitalWrite(LED_PIN, LOW);
  server.sendHeader("Location", "/panel");
  server.send(303);
}

void handleOff() {
  digitalWrite(LED_PIN, HIGH);
  server.sendHeader("Location", "/panel");
  server.send(303);
}
