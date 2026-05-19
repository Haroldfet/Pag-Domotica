# EcoSolar — Sistema Domótico Solar ESP32

Sistema de monitoreo y control de energía solar basado en **ESP32**. Todo el firmware y las páginas web están consolidados en un único sketch de Arduino.

---

## Archivos del Proyecto

```
Pag Domotica/
├── Final/
│   ├── Final.ino          ← Firmware principal (compilar y cargar esto)
│   └── html_pages.h       ← Páginas HTML embebidas en flash (PROGMEM)
├── 1. configuracion/
│   └── Config.html        ← Referencia: página de configuración WiFi
├── 2. Pag proncipal/
│   └── Inicio.html        ← Referencia: dashboard principal
├── 3. Eliminar wif/
│   └── Mark 1.html        ← Referencia: página de reset WiFi
└── README.md
```

> Los archivos HTML en las carpetas de referencia son la fuente de edición. Para actualizar el firmware hay que regenerar `html_pages.h` a partir de ellos.

---

## Hardware

### Pines del ESP32

| Señal            | GPIO | Descripción                       |
|------------------|------|-----------------------------------|
| Voltaje panel    | 35   | ADC — voltaje del panel solar     |
| Corriente panel  | 34   | ADC — corriente del panel solar   |
| Voltaje batería  | 33   | ADC — voltaje de la batería       |
| Nivel batería    | 32   | ADC — nivel de carga (%)          |
| Relé Luz 1       | 25   | HIGH = encendida                  |
| Relé Luz 2       | 26   | HIGH = encendida                  |
| Relé Luz 3       | 27   | HIGH = encendida                  |

### Componentes

- **ESP32** (Dual-core, WiFi integrado, 4 MB flash)
- **Panel solar** — máximo 100 V (escalar divisor de voltaje según panel real)
- **Batería** — cualquier química; ajustar fórmulas de nivel en `readSensors()`
- **3 relés** o transistores para las luces

---

## Instalación

### Requisitos

- Arduino IDE 2.x
- Board: **ESP32 by Espressif** v2.0+
  - Agregar en Preferencias → URLs adicionales:
    ```
    https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
    ```
- Librería: **ArduinoJson** v6.19+

### Cargar el firmware

1. Abrir `Final/Final.ino` en Arduino IDE
2. Herramientas → Placa → `ESP32 Dev Module`
3. Herramientas → Puerto → COM correspondiente
4. Herramientas → Upload Speed → `115200`
5. Clic en **Cargar** (→)

`html_pages.h` debe estar en la misma carpeta que `Final.ino`; el IDE lo incluye automáticamente.

---

## Primer Arranque — Configurar WiFi

1. Al encender por primera vez (sin credenciales guardadas), el ESP32 crea un Access Point:
   - **SSID:** `EcoSolar`
   - **Contraseña:** `12345678`
2. Conectar desde celular o PC a esa red
3. Abrir `http://192.168.4.1`
4. Ingresar SSID y contraseña de la red WiFi local → **Conectar**
5. El ESP32 guarda las credenciales en EEPROM y reinicia
6. Conectar el celular/PC de vuelta a la red WiFi local
7. Abrir la IP que muestra el Monitor Serial → aparece el dashboard

---

## Flujo de Páginas

```
Sin WiFi guardado
  └── GET /  →  Config.html  (formulario SSID + contraseña)
        └── POST /guardar  →  guarda en EEPROM → reinicia

Con WiFi guardado
  └── GET /  →  Inicio.html  (dashboard principal)
        └── Configuracion → botón "Eliminar"
              └── POST /api/forget-wifi  →  Mark 1.html (reset)
                    └── 5 s → redirige a /  →  Config.html
```

---

## API REST

Base URL: `http://<IP_ESP32>`

### GET `/api/datos`

Devuelve el estado actual de todos los sensores y luces.

```json
{
  "panelVoltage":   23.5,
  "panelCurrent":   12.0,
  "panelPower":    282.0,
  "batteryVoltage": 12.2,
  "batteryLevel":   76,
  "energyToday":    0.056,
  "energyMonth":    0.017,
  "connected":      true,
  "luz1": false,
  "luz2": true,
  "luz3": false
}
```

| Campo           | Unidad | Descripción                                   |
|-----------------|--------|-----------------------------------------------|
| `panelVoltage`  | V      | Voltaje del panel solar                       |
| `panelCurrent`  | A      | Corriente del panel solar                     |
| `panelPower`    | W      | Potencia instantánea (V × I)                  |
| `batteryVoltage`| V      | Voltaje de la batería                         |
| `batteryLevel`  | %      | Nivel de carga (0–100)                        |
| `energyToday`   | kWh    | Energía acumulada hoy (estimada)              |
| `energyMonth`   | kWh    | Energía acumulada este mes (estimada)         |
| `connected`     | bool   | `true` si el panel supera 5 V                 |
| `luz1/2/3`      | bool   | `true` = encendida                            |

---

### POST `/api/luz/{id}/toggle`

Alterna el estado de una luz (1, 2 o 3).

```
POST /api/luz/1/toggle
```

Respuesta exitosa:
```json
{ "success": true }
```

---

### POST `/api/forget-wifi`

Borra las credenciales WiFi de la EEPROM y reinicia el ESP32 en modo AP.

Respuesta: página HTML de reset (Mark 1), luego reinicio tras ~2 s.

---

## Configuración Técnica

### Almacenamiento WiFi (EEPROM)

| Dato              | Dirección | Tamaño |
|-------------------|-----------|--------|
| SSID              | 0         | 64 B   |
| Contraseña        | 65        | 64 B   |
| Flag configurado  | 130       | 1 B (valor `99` = configurado) |

### Fórmulas de sensores (`readSensors()`)

```cpp
panelVoltage   = (adc / 4095.0) * 100.0   // Ajustar multiplicador al divisor real
panelCurrent   = (adc / 4095.0) * 50.0
panelPower     = panelVoltage * panelCurrent
batteryVoltage = (adc / 4095.0) * 20.0
batteryLevel   = map(adc, 0, 4095, 0, 100)
connected      = (panelVoltage > 5.0)
```

Ajustar los multiplicadores según los divisores de voltaje reales del circuito.

---

## Solución de Problemas

| Síntoma | Acción |
|---------|--------|
| No aparece el AP "EcoSolar" | Verificar que la EEPROM no tiene flag 99 en addr 130; borrar con `eraseWiFiConfig()` |
| Dashboard no carga datos | Abrir consola del navegador (F12); verificar que `/api/datos` responde |
| Luces no conmutan | Verificar GPIO 25/26/27 con multímetro; comprobar relés |
| Valores de sensores incorrectos | Ajustar multiplicadores en `readSensors()` con lecturas reales |
| Error de compilación | Verificar que `html_pages.h` está en la misma carpeta que `Final.ino` |

---

## Modificar las Páginas HTML

1. Editar el archivo HTML correspondiente en su carpeta (ej. `2. Pag proncipal/Inicio.html`)
2. Regenerar `html_pages.h` ejecutando el script de PowerShell (ver sesión de desarrollo) o copiar manualmente el contenido actualizado
3. Recompilar y cargar `Final.ino`

---

**EcoSolar v2.0** | ESP32 · Arduino · Mayo 2026
