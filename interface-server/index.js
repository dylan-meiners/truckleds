console.clear()

// BLE
const INTERFACE_NAME = "Interface iOS"
const INTERFACE_SERVICE_UUID = "45c8c4868812466da5d20d81c4d6a8a4"
const INTERFACE_CHARACTERISTIC_UUID = ""
const DATA_BUFFER_LEN = 5
var isOn = false
var isConnectedToValidInterface = false
var updateInt = null
var updateRSSIint = null
var lastUpdate =  null
var data = null
var newDataAvailable = false
var toSend = []
var shouldSendData = false
var timeoutTimer = null
const SIMULATE_BLE = false
var noble = require("noble")
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
noble.on("discover", function(peripheral) {
    if (peripheral.advertisement.localName == INTERFACE_NAME) {
        noble.stopScanning()
        prettyLog("Found Interface!", peripheral.address)
        prettyLog("TX power level: " + peripheral.advertisement.txPowerLevel, peripheral.address)
        prettyLog("Manufacturer data: " + peripheral.advertisement.manufacturerData, peripheral.address)
        prettyLog("Service data: " + JSON.stringify(peripheral.advertisement.serviceData, null, 2), peripheral.address)
        prettyLog("Service UUID(s): " + peripheral.advertisement.serviceUuids, peripheral.address)
        initInterface(peripheral)
    }
    //else prettyLog("Found non-Interface: " + peripheral.advertisement.localName)
})
function interfaceDisconnect(peripheral) {

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
}

function initInterface(peripheral) {

    peripheral.on("disconnect", interfaceDisconnect(peripheral))
    peripheral.connect(function(error) {

        if (error) {

            prettyLog("Error connecting to Interface: " + error, peripheral.address)
            prettyLog("Interface disconnected with address: " + peripheral.address, peripheral.address)
            prettyLog("Appempting to reconnect...", peripheral.address)
            isConnectedToValidInterface = false
            peripheral.removeAllListeners()
        }
        else {

            prettyLog("Successfully onnected to Interface", peripheral.address)
            // Create an interval which gets the RSSI of interface every second
            updateRSSIint = setInterval(function() {

                peripheral.updateRssi()
            }, 1000)
            // Check to see if Interface's characteristic UUID is one being broadcasted
            if (peripheral.advertisement.serviceUuids.indexOf(INTERFACE_SERVICE_UUID) != -1) {

                isConnectedToValidInterface = true
                // Start to discover services matching Interface's specific service UUID
                peripheral.discoverServices(peripheral.advertisement.serviceUuids, function(error, services) {

                    //Interface should only be broadcasting one UUID, so choose that one
                    services[0].discoverCharacteristics([], function(error, characteristics) {

                        // Save the time of when the characteristic was last updated
                        lastUpdate = now()

                        // There should obly be one characteristic being broadcast that matches, so subscribe to that one
                        characteristics[0].subscribe(function(error) {

                            if (error) {

                                prettyLog("Unable to subscribe to characteristic, disconnecting. Error: " + error, peripheral.address)
                                peripheral.disconnect()
                            }
                        })
                        // Set up a callback for whenever Interface updates the characteristic
                        characteristics[0].on("data", function(newData) {

                            // Save the time of when the characteristic was last updated
                            lastUpdate = now()
                            data = newData
                            newDataAvailable = true
                        })
                        // Create an interval that checks if Interface is no longer connected properly every one second
                        updateInt = setInterval(function() {

                            // If there has been more than 2 seconds without communication with Interface, try to read
                            // from the characteristic
                            currentTime = now()
                            if (currentTime - lastUpdate >= 2000) {

                                characteristics[0].read()
                                // Set a function to be called in three seconds to check if the read was successful. If not,
                                // disconnect from Interface
                                setTimeout(function() {

                                    if (currentTime - lastUpdate >= 3000) {

                                        prettyLog("Read timed out, disconnecting", peripheral.address)
                                        peripheral.disconnect()
                                    }
                                }, 1000)
                            }
                        }, 1000)
                    })
                })
            }
            // If Interface's service UUID could not be found, then list the ones that were found and disconnect
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
setInterval(function() {

    if (!isConnectedToValidInterface && isOn) {

        toggleScannerOffOn()
    }
}, 5000)
function toggleScannerOffOn() {

    prettyLog("Power cycling scanner")
    noble.stopScanning()
    noble.startScanning()
}

// Raspberry Pi GPIO
var GPIO = require("onoff").Gpio
var dataRequestPin = new GPIO(21, "out")
dataRequestPin.writeSync(0)
var dataAckPin = new GPIO(20, "in")

// Arduino
var serialData = []
const SerialPort = require("serialport")
const arduino = new SerialPort("/dev/ttyACM0", { baudRate: 115200 })
arduino.on("error", function(error) { prettyLog("Serial arduino error: " + error) })
arduino.on("data", function(data) { serialData.push(data) })

const RPM_PID = "010C\r"
const IDLE_RPM = 600
var rpm = IDLE_RPM
var rpmOn = false
var checker = null
var lastRequest = null
var obd = new SerialPort("/dev/ttyUSB0", { baudRate: 38400 })
function resetOBD() {

    prettyLog("Resetting obd")
    obd.write("atz\r")
}
obd.on("error", function(error) { prettyLog("Serial port obd error: " + error) })
obd.on("data", function(data) {

    var dataString = String(data)
    var withoutReturn = []
    for (let i = 0; i < dataString.length; ++i) {
        withoutReturn.push(dataString.charCodeAt(i))
     }
    prettyLog(withoutReturn)
    let startedUp = dataString.startsWith("\r\rELM327")
    if (startedUp) {

        prettyLog("ELM327 started up")
    }
    else if (dataString.startsWith("41 0C")) {

        // rpm data incoming
        if (dataString.length === 13 &&
            dataString[5] === " " &&
            dataString[8] === " " &&
            dataString[12] === "\r") {
            var rpmToSet = (256 * parseInt(dataString[6] + dataString[7], 16) + parseInt(dataString[9] + dataString[10], 16)) / 4
	    if (rpmToSet > 0 && rpmToSet < 16383.75) rpm = rpmToSet
            rpm = Math.round(rpm)
            if (rpm < IDLE_RPM) {
                rpm = IDLE_RPM
                prettyLog("Ummmmmm for some reason the RPM was calculated to be below 0????")
            }
            else if (rpm > 7000) {
                rpm = 7000
                prettyLog("RPM was calculated to be greater than 7000... (new engine?)")
            }
        }
    }
    if (dataString.startsWith("\r>") || startedUp) {
    	obd.write(RPM_PID)
    }
    lastRequest = now()
})

// Util
function prettyLog(str="", address="") {

    var date = new Date()
    process.stdout.write("[" + date.getHours() + ":" + date.getMinutes() + "." + date.getMilliseconds() + "]")
    if (address != "") process.stdout.write("[" + address + "]")
    process.stdout.write(" > " + str + "\n")
}
function now() { return new Date().getTime() }

// Main programm update interval
// I have found out that if update() is called inside a while loop, events are
// lost. Instead, the update function is called on an interval.
setInterval(
    function() {
        update()
    },
    1 // Given in milliseconds (1000 hz)
)

function update() {

    if (newDataAvailable) {
        
        // Right now the data is an object, which appears to be a buffer of ASCII values,
        // so convert it to a regular string
        data = newData.toString("utf-8")
        prettyLog("Got new data: " + data, "TODO: create global variable to store peripheral address")

        // Reset the send array to store the bytes that will be sent to the Arduino
        toSend = []
        // Check to make sure that the data we received from Interface is the right length
        if (data.length === DATA_BUFFER_LEN) {

            //These should only ever be "1" or "0"
            var warning = data[0] == "1" ? 1 : 0
            // Convert ASCII to decimal
            var warningIndex = data.charCodeAt(1) - 48
            var police = data[2] == "1" ? 1 : 0
            var effect = data[3] == "1" ? 1 : 0
            var effectIndex = data.charCodeAt(4) - 48

            // The first value is whether or not the LEDs should be in regular driving mode or an effect mode
            // TODO: sometime I will have to update Interface to act as if warning is the same as effect
            toSend.push(warning || effect)

            // A variable is used to store the index of the effect that the Arduino will use.
            // TODO: This system can be greatly improved for scalability later
            var correspondingIndex = 0

            if (warning) {
                
                // If the warning mode is either left director, right director, or center director
                if (warningIndex == 3 || warningIndex == 4 || warningIndex == 5) {
                
                    correspondingIndex = 12
                }
                // Otheriwse, set the corresponding index to line up with the array on the arduino side
                else correspondingIndex = warningIndex + 9
            }
            else if (effect) {
                
                // Line up the index with the array on the arduino side
                correspondingIndex = effectIndex + 16
                // If index is for rpm
                if (correspondingIndex === 20) {
                    // If we were off
                    if (!rpmOn) {
                        /*checker = setInterval(function() {
                            if (now() - lastRequest >= 100) resetOBD()
                        }, 1000)*/
                        resetOBD()
                    }
                    rpmOn = true
                }
            }

            toSend.push(correspondingIndex)
            toSend.push(police)
            
            if (correspondingIndex === 20) {

                let rpmToSend = Math.round(((rpm - 500) / 6500) * 256)
                rpmToSend = rpmToSend < 0 ? 0 : rpmToSend
                rpmToSend = rpmToSend > 255 ? 255 : rpmToSend
                toSend.push(rpmToSend)
                prettyLog(toSend)
            }
            else if (rpmOn) {
                if (checker != null) clearInterval(checker)
                checker = null
                rpmOn = false
            }
            // prettyLog(toSend)

            shouldSendData = true
            // Signal the arduino that the Pi is ready to send data
            dataRequestPin.writeSync(1)
            // Save the current time for checking if there is a timeout
            timeoutTimer = now()
        }
    }
    
    if (shouldSendData) {

        // Only move on if the Arduino has signaled it is ready or if a timeout has occurred
        if (dataAckPin.readSync() === 1 || (now() - timeoutTimer > 10)) {
            // There could have been a timeout, so check before sending data if the Arduino
            // is ready
            if (dataAckPin.readSync() === 1) {
                arduino.write(toSend)
                shouldSendData = false
            }
            else {
                prettyLog("Couldn't send data because serial timed out. Attempting to send again")
            }
            // Regardless if there was a timeout, reset the signal pin
            dataRequestPin.writeSync(0)
            // Save the current time for checking if there is a timeout
            timeoutTimer = now()
        }
    }
}