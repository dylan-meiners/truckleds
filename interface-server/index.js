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

                            // Right now the data is an object, which appears to be a buffer of ASCII values,
                            // so convert it to a regular string
                            data = newData.toString("utf-8")
                            prettyLog("Got new data: " + data, peripheral.address)

                            // Create an empty array to store the ASCII bytes that will be sent to the Arduino
                            var toSend = []

                            // Check to make sure that the data we received from Interface is the right length
                            if (data.length == DATA_BUFFER_LEN) {

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
                                    
                                    if (warningIndex == 3 || warningIndex == 4 || warningIndex == 5) {
                                    
                                        correspondingIndex = 12
                                    }
                                    else correspondingIndex = warningIndex + 9
                                }
                                else if (effect) {
                                    
                                    correspondingIndex = effectIndex + 16
                                }
                                toSend.push(correspondingIndex)
                                toSend.push(police)
                            }
                            
                            // Signal the arduino that the Pi is ready to send data
                            dataRequestPin.writeSync(1)
                            // Save the current time for checking if there is a timeout (1 second)
                            var serialTimeoutMillis = now()
                            // Do nothing while the Pi is waiting for the Arduino to signal it is ready or while
                            // a timeout has not occurred
                            while (dataAckPin.readSync() === 0 || (now() - serialTimeoutMillis < 1000));
                            // The loop could have exited due to a timeout, so check before sending data if the Arduino
                            // is ready
                            if (dataAckPin.readSync() === 1) arduino.write(toSend)
                            else prettyLog("Couldn't send data because serial timed out")
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
const arduino = new SerialPort("/dev/ttyACM0", {
    baudRate: 115200
})
arduino.on("error", function(error) {
    prettyLog("Serial arduino error: " + error)
})
arduino.on("data", function(data) {
    serialData.push(data)
})

// Util
function prettyLog(str="", address="") {

    var date = new Date()
    process.stdout.write("[" + date.getHours() + ":" + date.getMinutes() + "." + date.getMilliseconds() + "]")
    if (address != "") process.stdout.write("[" + address + "]")
    process.stdout.write(" > " + str + "\n")
}
function now() { return new Date().getTime() }