console.clear()
var noble = require("noble")

var PI_IS_REQUESTING_PIN = 21
var ARDUINO_IS_READY_PIN = 20
var GPIO = require("onoff").Gpio
var piIsRequestingPin = new GPIO(PI_IS_REQUESTING_PIN, "out")
piIsRequestingPin.writeSync(0)
var arduinoIsReadyPin = new GPIO(ARDUINO_IS_READY_PIN, "in")

const SerialPort = require("serialport")
const port = new SerialPort("/dev/ttyACM0", {
    baudRate: 115200
})
port.on("error", function(error) {
    prettyLog("Serial port error: " + error)
})
var serialData = []
port.on("data", function(data) {
    serialData.push(data)
    //prettyLog("New serial data available: " + data)
    console.log(data)
})
port.on("close", function() {
    prettyLog("Serial port closed")
})

const INTERFACE_NAME = "Interface iOS"
const INTERFACE_SERVICE_UUID = "45c8c4868812466da5d20d81c4d6a8a4"
const INTERFACE_CHARACTERISTIC_UUID = ""

const DATA_BUFFER_LEN = 5
isOn = false
isConnectedToValidInterface = false
var updateInt = null
var updateRSSIint = null
var lastUpdate =  null
var data = null

setInterval(attemptConnect, 5000)
function attemptConnect() {
    if (!isConnectedToValidInterface && isOn) {
        toggleScannerOffOn()
    }
}

noble.on("stateChange", function(state) {
    prettyLog("State change: " + state)
    if (state === "poweredOn") {
        isOn = true
        noble.startScanning()
    }
    else {
        isOn = false
        noble.stopScanning()
    }
})

function toggleScannerOffOn() {
    prettyLog("Power cycling scanner")
    noble.stopScanning()
    noble.startScanning()
}

function prettyLog(str="", address="") {
    var date = new Date()
    process.stdout.write("[" + date.getHours() + ":" + date.getMinutes() + "." + date.getMilliseconds() + "]")
    if (address != "") process.stdout.write("[" + address + "]")
    process.stdout.write(" > " + str + "\n")
}

noble.on("discover", function(peripheral) {
    if (peripheral.advertisement.localName == INTERFACE_NAME) {
        noble.stopScanning()
        prettyLog("Found Interface!", peripheral.address)
        prettyLog("TX power level: " + peripheral.advertisement.txPowerLevel, peripheral.address)
        prettyLog("Manufacturer data: " + peripheral.advertisement.manufacturerData, peripheral.address)
        prettyLog("Service data: " + JSON.stringify(peripheral.advertisement.serviceData, null, 2), peripheral.address)
        prettyLog("Service UUID(s): " + peripheral.advertisement.serviceUuids, peripheral.address)
        run(peripheral)
    }
    //else prettyLog("Found non-Interface: " + peripheral.advertisement.localName)
})

function run(peripheral) {
    peripheral.on("disconnect", function() {
        if (updateRSSIint != null) {
            clearInterval(updateRSSIint)
            udpateRSSIint = null
        }
        if (updateInt != null) {
            clearInterval(updateInt)
            updateInt = null
        }
        prettyLog("Interface disconnected with address: " + peripheral.address, peripheral.address)
        prettyLog("Appempting to reconnect...", peripheral.address)
        isConnectedToValidInterface = false
        peripheral.removeAllListeners()
    })
    peripheral.connect(function(error) {
        if (error) {
            prettyLog("Error connecting to Interface: " + error, peripheral.address)
            prettyLog("Interface disconnected with address: " + peripheral.address, peripheral.address)
            prettyLog("Appempting to reconnect...", peripheral.address)
            isConnectedToValidInterface = false
            peripheral.removeAllListeners()
        }
        else {
            prettyLog("Connected to Interface", peripheral.address)
            updateRSSIint = setInterval(function() {
                peripheral.updateRssi()
            }, 1000)
            if (peripheral.advertisement.serviceUuids.indexOf(INTERFACE_SERVICE_UUID) != -1) {
                isConnectedToValidInterface = true
                peripheral.discoverServices(peripheral.advertisement.serviceUuids, function(error, services) {
                    services[0].discoverCharacteristics([], function(error, characteristics) {
                        var initialDate = new Date()
                        var initialMillis = initialDate.getTime()
                        lastUpdate = initialMillis

                        characteristics[0].subscribe(function(error) {
                            if (error) {
                                prettyLog("Unable to subscribe to characteristic, disconnecting. Error: " + error, peripheral.address)
                                peripheral.disconnect()
                            }
                        })
                        characteristics[0].on("data", function(newData) {
                            var date = new Date()
                            var millis = date.getTime()
                            lastUpdate = millis
                            prettyLog("Got new data: " + newData.toString("utf-8"), peripheral.address)
                            data = newData.toString("utf-8")
                            var toSend = []
                            if (data.length == 5) {
                                var warning = data[0] == "1" ? 1 : 0
                                var warningIndex = data.charCodeAt(1) - 48
                                var police = data[2] == "1" ? 1 : 0
                                var effect = data[3] == "1" ? 1 : 0
                                var effectIndex = data.charCodeAt(4) - 48
                                toSend.push(warning || effect)
                                var correspondingIndex = 0
                                if (warning) {
                                    if (warningIndex == 3 || warningIndex == 4 || warningIndex == 5) {
                                        correspondingIndex = 12
                                    }
                                    else correspondingIndex = warningIndex + 9
                                }
                                else if (effect) {
                                    correspondingIndex = effectIndex + 16
                                    if (correspondingIndex == 17) {
                                        if (!musicRunning) startMusic()
                                    }
                                    else if (musicRunning) stopMusic()
                                }
                                toSend.push(correspondingIndex)
                                toSend.push(police)
                            }
                            if (!musicRunning) {

                                piIsRequestingPin.writeSync(1)
                                var serialTimeoutMillis = new Date().getTime()
                                while (arduinoIsReadyPin.readSync() === 0 || (new Date().getTime() - serialTimeoutMillis < 1000));
                                if (arduinoIsReadyPin.readSync() === 1) { port.write(toSend) }
                                else prettyLog("Couldn't send data because serial timed out")
                            }
                        })
                        
                        updateInt = setInterval(function() {
                            var date = new Date()
                            var millis = date.getTime()
                            if (millis - lastUpdate >= 2000) {
                                characteristics[0].read()
                                setTimeout(function() {
                                    if (millis - lastUpdate >= 3000) {
                                        prettyLog("Read timed out, disconnecting", peripheral.address)
                                        peripheral.disconnect()
                                    }
                                }, 1000)
                            }
                        }, 1000)
                    })
                })
            }
            else {
                prettyLog("Interface's custom service UUID does not match any advertised service uuids... advertised are: ", peripheral.address)
                for(var uuidIndex = 0; uuidIndex < peripheral.advertisement.serviceUuids.length; uuidIndex++) {
                    prettyLog(peripheral.advertisement.serviceUuids[uuidIndex], peripheral.address)
                }
                prettyLog("Disconnecting from \"Interface\" and trying again", peripheral.address)
                peripheral.disconnect()
                isConnectedToValidInterface = false
            }
        }
    })
}

// Music
const { exec } = require("child_process")
var musicRunning = false
function execc(processName, command) {
    exec(command, (error, stdout, stderr) => {
        if (error) {
            prettyLog(`${processName} error: ${error.message}`)
            return
        }
        if (stderr) {
            prettyLog(`${processName} stderr: ${stderr}`)
            return
        }
        prettyLog(`${processName} stdout: ${stdout}`)
    })
}
function startMusic() {
    port.close(function() {
        execc("music", "sudo python3 ./music/music-simple.py")
        musicRunning = true
    })
}
function stopMusic() {
    execc("music", "sudo pkill python3")
    musicRunning = false
}