#include <Wire.h>
#include <WiFi.h>
#include <WebServer.h>

// ======================================================
// MOTIONCORE
// Stark-style ESP32 + MPU6050 Motion Interface
// ======================================================

// ---------------- WiFi ----------------

const char* ssid = "MotionCore";
const char* password = "12345678";

WebServer server(80);

// ---------------- MPU6050 ----------------

#define MPU_ADDR 0x68

#define SDA_PIN 26
#define SCL_PIN 27

// Raw values
int16_t AcX, AcY, AcZ;
int16_t GyX, GyY, GyZ;

// Converted values
float ax, ay, az;
float gx, gy, gz;

// Gyro offsets
float gxOffset = 0;
float gyOffset = 0;
float gzOffset = 0;

// ======================================================
// MPU6050 FUNCTIONS
// ======================================================

void writeMPU(byte reg, byte data) {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}


int16_t read16(byte reg) {

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.endTransmission(false);

  Wire.requestFrom(MPU_ADDR, 2, true);

  int16_t value =
    (Wire.read() << 8) |
    Wire.read();

  return value;
}


// ======================================================
// READ MPU6050
// ======================================================

void readMPU() {

  AcX = read16(0x3B);
  AcY = read16(0x3D);
  AcZ = read16(0x3F);

  GyX = read16(0x43);
  GyY = read16(0x45);
  GyZ = read16(0x47);


  // Accelerometer
  ax = AcX / 16384.0;
  ay = AcY / 16384.0;
  az = AcZ / 16384.0;


  // Gyroscope
  gx = (GyX / 131.0) - gxOffset;
  gy = (GyY / 131.0) - gyOffset;
  gz = (GyZ / 131.0) - gzOffset;
}


// ======================================================
// CALIBRATION
// ======================================================

void calibrateGyro() {

  long sumX = 0;
  long sumY = 0;
  long sumZ = 0;

  const int samples = 500;

  Serial.println("Calibration started...");
  Serial.println("Keep MPU6050 completely still.");

  for (int i = 0; i < samples; i++) {

    sumX += read16(0x43);
    sumY += read16(0x45);
    sumZ += read16(0x47);

    delay(5);
  }

  gxOffset =
    (sumX / (float)samples) / 131.0;

  gyOffset =
    (sumY / (float)samples) / 131.0;

  gzOffset =
    (sumZ / (float)samples) / 131.0;

  Serial.println("Calibration complete.");
}


// ======================================================
// WEBPAGE
// ======================================================

const char webpage[] PROGMEM = R"rawliteral(

<!DOCTYPE html>

<html>

<head>

<meta name="viewport"
content="width=device-width, initial-scale=1">

<title>MOTIONCORE</title>


<style>

/* =====================================================
   GLOBAL
   ===================================================== */

* {
    box-sizing: border-box;
}

html,
body {

    margin: 0;

    padding: 0;

    width: 100%;

    height: 100%;

    overflow: hidden;

    background: #020609;

    color: #00d9ff;

    font-family:
        "Courier New",
        monospace;
}


/* =====================================================
   BACKGROUND
   ===================================================== */

body {

    background:

        radial-gradient(
            circle at center,
            rgba(0,130,180,0.18),
            transparent 35%
        ),

        linear-gradient(
            #020609,
            #000
        );
}


/* Grid */

body::before {

    content: "";

    position: fixed;

    inset: 0;

    background-image:

        linear-gradient(
            rgba(0,180,255,0.06) 1px,
            transparent 1px
        ),

        linear-gradient(
            90deg,
            rgba(0,180,255,0.06) 1px,
            transparent 1px
        );

    background-size: 45px 45px;

    pointer-events: none;

    opacity: 0.5;
}


/* =====================================================
   HEADER
   ===================================================== */

.header {

    position: absolute;

    top: 20px;

    left: 30px;

    right: 30px;

    display: flex;

    justify-content: space-between;

    align-items: center;

    z-index: 10;
}


.logo {

    font-size: 25px;

    font-weight: bold;

    letter-spacing: 6px;

    text-shadow:
        0 0 8px #00d9ff,
        0 0 20px #0088aa;
}


.version {

    font-size: 11px;

    letter-spacing: 2px;

    opacity: 0.65;
}


/* =====================================================
   MAIN HUD
   ===================================================== */

.hud {

    position: absolute;

    inset: 80px 25px 25px 25px;

    display: grid;

    grid-template-columns:
        260px
        1fr
        260px;

    gap: 20px;
}


/* =====================================================
   PANELS
   ===================================================== */

.panel {

    border: 1px solid rgba(0,210,255,0.35);

    background:
        rgba(0,20,30,0.45);

    backdrop-filter:
        blur(4px);

    box-shadow:
        0 0 20px
        rgba(0,180,255,0.08);

    padding: 18px;

    position: relative;
}


.panel::before {

    content: "";

    position: absolute;

    top: -1px;

    left: 15px;

    width: 55px;

    height: 2px;

    background: #00d9ff;

    box-shadow:
        0 0 10px #00d9ff;
}


.panel-title {

    font-size: 12px;

    letter-spacing: 3px;

    margin-bottom: 20px;

    color: #75eaff;
}


/* =====================================================
   LEFT TELEMETRY
   ===================================================== */

.telemetry {

    display: flex;

    flex-direction: column;

    gap: 17px;
}


.metric {

    font-size: 12px;
}


.metric-header {

    display: flex;

    justify-content: space-between;

    margin-bottom: 6px;
}


.bar {

    height: 5px;

    background:
        rgba(0,200,255,0.08);

    border: 1px solid
        rgba(0,200,255,0.2);

    overflow: hidden;
}


.bar-fill {

    height: 100%;

    width: 0%;

    background: #00d9ff;

    box-shadow:
        0 0 8px #00d9ff;

    transition:
        width 0.1s;
}


/* =====================================================
   CENTER
   ===================================================== */

.center {

    display: flex;

    flex-direction: column;

    align-items: center;

    justify-content: center;

    position: relative;
}


/* =====================================================
   ARC CORE
   ===================================================== */

.core {

    position: absolute;

    width: 350px;

    height: 350px;

    border-radius: 50%;

    border:
        1px solid
        rgba(0,210,255,0.15);

    box-shadow:
        0 0 50px
        rgba(0,180,255,0.08);
}


.core::before {

    content: "";

    position: absolute;

    inset: 30px;

    border-radius: 50%;

    border:
        1px dashed
        rgba(0,210,255,0.35);

    animation:
        spin 12s linear infinite;
}


.core::after {

    content: "";

    position: absolute;

    inset: 70px;

    border-radius: 50%;

    border:
        2px solid
        rgba(0,210,255,0.2);

    animation:
        spinReverse 8s linear infinite;
}


@keyframes spin {

    from {
        transform: rotate(0deg);
    }

    to {
        transform: rotate(360deg);
    }
}


@keyframes spinReverse {

    from {
        transform: rotate(360deg);
    }

    to {
        transform: rotate(0deg);
    }
}


/* =====================================================
   3D CUBE
   ===================================================== */

.scene {

    width: 190px;

    height: 190px;

    perspective: 700px;

    position: relative;

    z-index: 3;
}


.cube {

    width: 160px;

    height: 160px;

    position: absolute;

    left: 15px;

    top: 15px;

    transform-style:
        preserve-3d;

    transform:
        rotateX(0deg)
        rotateY(0deg)
        rotateZ(0deg);
}


.face {

    position: absolute;

    width: 160px;

    height: 160px;

    border:
        2px solid
        #00d9ff;

    background:
        rgba(0,180,255,0.04);

    display: flex;

    align-items: center;

    justify-content: center;

    color: #00eaff;

    font-size: 14px;

    text-shadow:
        0 0 8px #00d9ff;

    box-shadow:
        inset 0 0 25px
        rgba(0,180,255,0.08),

        0 0 15px
        rgba(0,180,255,0.3);
}


.front {
    transform:
        translateZ(80px);
}

.back {
    transform:
        rotateY(180deg)
        translateZ(80px);
}

.right {
    transform:
        rotateY(90deg)
        translateZ(80px);
}

.left {
    transform:
        rotateY(-90deg)
        translateZ(80px);
}

.top {
    transform:
        rotateX(90deg)
        translateZ(80px);
}

.bottom {
    transform:
        rotateX(-90deg)
        translateZ(80px);
}


/* =====================================================
   OBJECT LABEL
   ===================================================== */

.object-label {

    margin-top: 35px;

    font-size: 11px;

    letter-spacing: 3px;

    z-index: 4;

    opacity: 0.8;
}


/* =====================================================
   ORIENTATION
   ===================================================== */

.orientation {

    position: absolute;

    bottom: 20px;

    left: 20px;

    right: 20px;

    display: grid;

    grid-template-columns:
        repeat(3,1fr);

    gap: 10px;

    z-index: 5;
}


.angle {

    border:
        1px solid
        rgba(0,210,255,0.25);

    padding: 10px;

    text-align: center;

    background:
        rgba(0,30,40,0.5);
}


.angle-value {

    font-size: 17px;

    margin-top: 5px;

    color: #fff;

    text-shadow:
        0 0 8px #00d9ff;
}


/* =====================================================
   RIGHT SYSTEM PANEL
   ===================================================== */

.system-line {

    display: flex;

    justify-content: space-between;

    padding: 9px 0;

    border-bottom:
        1px solid
        rgba(0,200,255,0.08);

    font-size: 11px;
}


.online {

    color: #00ff99;

    text-shadow:
        0 0 8px #00ff99;
}


/* =====================================================
   MOTION CORE
   ===================================================== */

.motion-core {

    margin-top: 30px;

    text-align: center;
}


.core-value {

    font-size: 35px;

    color: white;

    text-shadow:
        0 0 15px #00d9ff;
}


.core-label {

    font-size: 10px;

    letter-spacing: 3px;

    opacity: 0.6;
}


/* =====================================================
   BUTTONS
   ===================================================== */

button {

    width: 100%;

    margin-top: 10px;

    padding: 11px;

    background:
        rgba(0,150,200,0.08);

    border:
        1px solid
        rgba(0,210,255,0.5);

    color: #00d9ff;

    font-family:
        "Courier New";

    letter-spacing: 2px;

    cursor: pointer;

    transition: 0.2s;
}


button:hover {

    background:
        rgba(0,200,255,0.18);

    box-shadow:
        0 0 15px
        rgba(0,200,255,0.3);
}


/* =====================================================
   STARTUP SCREEN
   ===================================================== */

#boot {

    position: fixed;

    inset: 0;

    background: #000;

    z-index: 100;

    display: flex;

    align-items: center;

    justify-content: center;

    flex-direction: column;
}


.boot-logo {

    font-size: 32px;

    letter-spacing: 8px;

    text-shadow:
        0 0 15px #00d9ff;
}


.boot-text {

    margin-top: 25px;

    font-size: 12px;

    line-height: 2;

    width: 300px;

    text-align: left;

    color: #00d9ff;
}


/* =====================================================
   RESPONSIVE
   ===================================================== */

@media(max-width:900px) {

    .hud {

        grid-template-columns: 1fr;

        overflow-y: auto;

        inset: 70px 10px 10px;
    }

    .panel {

        min-height: 250px;
    }

    .center {

        min-height: 500px;
    }
}

</style>

</head>


<body>


<!-- =================================================
     BOOT SCREEN
     ================================================= -->

<div id="boot">

    <div class="boot-logo">
        MOTIONCORE
    </div>

    <div class="boot-text"
         id="bootText">
    </div>

</div>


<!-- =================================================
     HEADER
     ================================================= -->

<div class="header">

    <div class="logo">
        MOTIONCORE
    </div>

    <div class="version">
        MK-I // MOTION INTERFACE
    </div>

</div>


<!-- =================================================
     HUD
     ================================================= -->

<div class="hud">


<!-- =================================================
     LEFT PANEL
     ================================================= -->

<div class="panel">

    <div class="panel-title">
        LIVE TELEMETRY
    </div>


    <div class="telemetry">


        <div class="metric">

            <div class="metric-header">

                <span>ACCEL X</span>

                <span id="ax">
                    0.00
                </span>

            </div>

            <div class="bar">
                <div
                    class="bar-fill"
                    id="axBar">
                </div>
            </div>

        </div>


        <div class="metric">

            <div class="metric-header">

                <span>ACCEL Y</span>

                <span id="ay">
                    0.00
                </span>

            </div>

            <div class="bar">
                <div
                    class="bar-fill"
                    id="ayBar">
                </div>
            </div>

        </div>


        <div class="metric">

            <div class="metric-header">

                <span>ACCEL Z</span>

                <span id="az">
                    0.00
                </span>

            </div>

            <div class="bar">
                <div
                    class="bar-fill"
                    id="azBar">
                </div>
            </div>

        </div>


        <div class="metric">

            <div class="metric-header">

                <span>GYRO X</span>

                <span id="gx">
                    0.00
                </span>

            </div>

            <div class="bar">
                <div
                    class="bar-fill"
                    id="gxBar">
                </div>
            </div>

        </div>


        <div class="metric">

            <div class="metric-header">

                <span>GYRO Y</span>

                <span id="gy">
                    0.00
                </span>

            </div>

            <div class="bar">
                <div
                    class="bar-fill"
                    id="gyBar">
                </div>
            </div>

        </div>


        <div class="metric">

            <div class="metric-header">

                <span>GYRO Z</span>

                <span id="gz">
                    0.00
                </span>

            </div>

            <div class="bar">
                <div
                    class="bar-fill"
                    id="gzBar">
                </div>
            </div>

        </div>


    </div>

</div>


<!-- =================================================
     CENTER PANEL
     ================================================= -->

<div class="panel center">


    <div class="core"></div>


    <div class="scene">

        <div
            class="cube"
            id="cube">

            <div class="face front">
                MOTION
            </div>

            <div class="face back">
                CORE
            </div>

            <div class="face right">
                X
            </div>

            <div class="face left">
                Y
            </div>

            <div class="face top">
                Z
            </div>

            <div class="face bottom">
                MK-I
            </div>

        </div>

    </div>


    <div class="object-label">

        OBJECT TRACKING:
        <span id="tracking">
            LOCKED
        </span>

    </div>


    <div class="orientation">


        <div class="angle">

            PITCH

            <div
                class="angle-value"
                id="pitch">
                0°
            </div>

        </div>


        <div class="angle">

            YAW

            <div
                class="angle-value"
                id="yaw">
                0°
            </div>

        </div>


        <div class="angle">

            ROLL

            <div
                class="angle-value"
                id="roll">
                0°
            </div>

        </div>


    </div>


</div>


<!-- =================================================
     RIGHT PANEL
     ================================================= -->

<div class="panel">

    <div class="panel-title">
        SYSTEM STATUS
    </div>


    <div class="system-line">

        <span>ESP32 CORE</span>

        <span class="online">
            ONLINE
        </span>

    </div>


    <div class="system-line">

        <span>MPU6050</span>

        <span class="online">
            ONLINE
        </span>

    </div>


    <div class="system-line">

        <span>I²C LINK</span>

        <span class="online">
            STABLE
        </span>

    </div>


    <div class="system-line">

        <span>NETWORK</span>

        <span class="online">
            ACTIVE
        </span>

    </div>


    <div class="system-line">

        <span>TRACKING</span>

        <span
            class="online"
            id="trackStatus">
            LOCKED
        </span>

    </div>


    <div class="motion-core">

        <div class="core-value"
             id="motion">
            0%
        </div>

        <div class="core-label">
            MOTION INTENSITY
        </div>

    </div>


    <button onclick="calibrate()">
        CALIBRATE CORE
    </button>


    <button onclick="resetCube()">
        RESET ORIENTATION
    </button>


    <button onclick="speakStatus()">
        SYSTEM VOICE
    </button>


    <div
        style="
        margin-top:25px;
        text-align:center;
        font-size:10px;
        opacity:.55;
        "
        id="gesture">

        AWAITING GESTURE

    </div>


</div>


</div>


<script>

/* =====================================================
   VARIABLES
   ===================================================== */

let cube =
    document.getElementById("cube");

let rotX = 0;

let rotY = 0;

let rotZ = 0;

let lastTime = Date.now();

let lastGesture =
    "NONE";


/* =====================================================
   BOOT SEQUENCE
   ===================================================== */

const bootMessages = [

    "> INITIALIZING MOTIONCORE...",

    "> LOADING SENSOR INTERFACE...",

    "> MPU6050 LINK ........ OK",

    "> I2C CHANNEL .......... OK",

    "> ESP32 CORE ........... OK",

    "> MOTION ENGINE ........ OK",

    "> OBJECT TRACKING ...... OK",

    "> SYSTEM READY."

];


let bootIndex = 0;


function bootSequence() {

    if (
        bootIndex <
        bootMessages.length
    ) {

        document.getElementById(
            "bootText"
        ).innerHTML +=
            bootMessages[bootIndex] +
            "<br>";

        bootIndex++;

        setTimeout(
            bootSequence,
            300
        );

    }

    else {

        setTimeout(
            function() {

                document.getElementById(
                    "boot"
                ).style.display =
                    "none";

                speak(
                    "MotionCore online."
                );

            },
            700
        );

    }

}


bootSequence();


/* =====================================================
   VOICE
   ===================================================== */

function speak(text) {

    if (
        "speechSynthesis"
        in window
    ) {

        let message =
            new SpeechSynthesisUtterance(
                text
            );

        message.rate = 0.9;

        message.pitch = 0.7;

        message.volume = 0.8;

        speechSynthesis.speak(
            message
        );
    }

}


function speakStatus() {

    speak(
        "MotionCore is online. " +
        "Motion tracking is active. " +
        "All systems are stable."
    );

}


/* =====================================================
   SENSOR BAR
   ===================================================== */

function setBar(
    id,
    value,
    max
) {

    let percent =
        Math.min(
            Math.abs(value) /
            max *
            100,
            100
        );

    document.getElementById(
        id
    ).style.width =
        percent + "%";

}


/* =====================================================
   GESTURE DETECTION
   ===================================================== */

function detectGesture(data) {

    let threshold = 80;

    let gesture =
        "STABLE";


    if (
        Math.abs(data.gx) >
        threshold
    ) {

        gesture =
            data.gx > 0 ?
            "ROTATE X +" :
            "ROTATE X -";

    }

    else if (
        Math.abs(data.gy) >
        threshold
    ) {

        gesture =
            data.gy > 0 ?
            "ROTATE Y +" :
            "ROTATE Y -";

    }

    else if (
        Math.abs(data.gz) >
        threshold
    ) {

        gesture =
            data.gz > 0 ?
            "ROTATE Z +" :
            "ROTATE Z -";

    }

    else if (
        Math.abs(data.ax) >
        1.5
    ) {

        gesture =
            data.ax > 0 ?
            "TILT X +" :
            "TILT X -";

    }


    if (
        gesture !==
        lastGesture
    ) {

        lastGesture =
            gesture;

        document.getElementById(
            "gesture"
        ).innerHTML =
            "GESTURE: " +
            gesture;

    }

}


/* =====================================================
   SENSOR DATA
   ===================================================== */

async function getSensorData() {

    try {

        let response =
            await fetch(
                "/data",
                {
                    cache: "no-store"
                }
            );


        let data =
            await response.json();


        /* ---------------------------------------------
           TELEMETRY
           --------------------------------------------- */

        document.getElementById(
            "ax"
        ).innerHTML =
            data.ax.toFixed(2);

        document.getElementById(
            "ay"
        ).innerHTML =
            data.ay.toFixed(2);

        document.getElementById(
            "az"
        ).innerHTML =
            data.az.toFixed(2);


        document.getElementById(
            "gx"
        ).innerHTML =
            data.gx.toFixed(1);

        document.getElementById(
            "gy"
        ).innerHTML =
            data.gy.toFixed(1);

        document.getElementById(
            "gz"
        ).innerHTML =
            data.gz.toFixed(1);


        /* ---------------------------------------------
           BARS
           --------------------------------------------- */

        setBar(
            "axBar",
            data.ax,
            2
        );

        setBar(
            "ayBar",
            data.ay,
            2
        );

        setBar(
            "azBar",
            data.az,
            2
        );

        setBar(
            "gxBar",
            data.gx,
            250
        );

        setBar(
            "gyBar",
            data.gy,
            250
        );

        setBar(
            "gzBar",
            data.gz,
            250
        );


        /* ---------------------------------------------
           MOTION INTENSITY
           --------------------------------------------- */

        let movement =
            Math.sqrt(
                data.gx * data.gx +
                data.gy * data.gy +
                data.gz * data.gz
            );


        let motion =
            Math.min(
                movement / 300 * 100,
                100
            );


        document.getElementById(
            "motion"
        ).innerHTML =
            motion.toFixed(0) +
            "%";


        /* ---------------------------------------------
           CUBE ROTATION
           --------------------------------------------- */

        rotX +=
            data.gx * 0.035;

        rotY +=
            data.gy * 0.035;

        rotZ +=
            data.gz * 0.035;


        cube.style.transform =

            "rotateX(" +
            rotX +
            "deg) " +

            "rotateY(" +
            rotY +
            "deg) " +

            "rotateZ(" +
            rotZ +
            "deg)";


        /* ---------------------------------------------
           ORIENTATION DISPLAY
           --------------------------------------------- */

        document.getElementById(
            "pitch"
        ).innerHTML =
            (rotX % 360).toFixed(1) +
            "°";


        document.getElementById(
            "yaw"
        ).innerHTML =
            (rotY % 360).toFixed(1) +
            "°";


        document.getElementById(
            "roll"
        ).innerHTML =
            (rotZ % 360).toFixed(1) +
            "°";


        /* ---------------------------------------------
           TRACKING
           --------------------------------------------- */

        document.getElementById(
            "tracking"
        ).innerHTML =
            "LOCKED";


        document.getElementById(
            "trackStatus"
        ).innerHTML =
            "LOCKED";


        detectGesture(data);

    }


    catch(error) {

        document.getElementById(
            "tracking"
        ).innerHTML =
            "LOST";


        document.getElementById(
            "trackStatus"
        ).innerHTML =
            "OFFLINE";

    }

}


/* =====================================================
   CALIBRATION
   ===================================================== */

async function calibrate() {

    speak(
        "Beginning core calibration. " +
        "Keep the device still."
    );


    try {

        await fetch(
            "/calibrate"
        );

        rotX = 0;

        rotY = 0;

        rotZ = 0;


        speak(
            "Calibration complete."
        );

    }

    catch(error) {

        console.log(error);

    }

}


/* =====================================================
   RESET
   ===================================================== */

function resetCube() {

    rotX = 0;

    rotY = 0;

    rotZ = 0;


    cube.style.transform =
        "rotateX(0deg) " +
        "rotateY(0deg) " +
        "rotateZ(0deg)";


    speak(
        "Orientation reset."
    );

}


/* =====================================================
   LOOP
   ===================================================== */

setInterval(
    getSensorData,
    40
);

getSensorData();

</script>

</body>

</html>

)rawliteral";


// ======================================================
// ROOT PAGE
// ======================================================

void handleRoot() {

  server.send(
    200,
    "text/html",
    webpage
  );

}


// ======================================================
// SENSOR API
// ======================================================

void handleData() {

  readMPU();

  String json = "{";

  json += "\"ax\":";
  json += String(ax, 2);

  json += ",\"ay\":";
  json += String(ay, 2);

  json += ",\"az\":";
  json += String(az, 2);

  json += ",\"gx\":";
  json += String(gx, 2);

  json += ",\"gy\":";
  json += String(gy, 2);

  json += ",\"gz\":";
  json += String(gz, 2);

  json += "}";

  server.send(
    200,
    "application/json",
    json
  );

}


// ======================================================
// CALIBRATION API
// ======================================================

void handleCalibrate() {

  calibrateGyro();

  server.send(
    200,
    "text/plain",
    "CALIBRATED"
  );

}


// ======================================================
// SETUP
// ======================================================

void setup() {

  Serial.begin(9600);

  delay(1000);


  // ---------------- I2C ----------------

  Wire.begin(
    SDA_PIN,
    SCL_PIN
  );


  // ---------------- MPU6050 ----------------

  writeMPU(
    0x6B,
    0x00
  );

  delay(100);


  // Check device

  Wire.beginTransmission(
    MPU_ADDR
  );

  byte error =
    Wire.endTransmission();


  if (error == 0) {

    Serial.println(
      "MPU6050 initialized"
    );

  }

  else {

    Serial.println(
      "MPU6050 connection failed"
    );

  }


  // ---------------- WiFi ----------------

  WiFi.softAP(
    ssid,
    password
  );

  delay(500);


  Serial.println(
    "WiFi AP started"
  );


  Serial.print(
    "IP address: "
  );

  Serial.println(
    WiFi.softAPIP()
  );


  // ---------------- Server ----------------

  server.on(
    "/",
    handleRoot
  );


  server.on(
    "/data",
    handleData
  );


  server.on(
    "/calibrate",
    handleCalibrate
  );


  server.begin();


  Serial.println(
    "Web server started"
  );

}


// ======================================================
// LOOP
// ======================================================

void loop() {

  server.handleClient();

}