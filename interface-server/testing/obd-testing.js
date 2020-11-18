console.clear()
const SerialPort = require("serialport")
const port = new SerialPort("COM4", {
    baudRate: 38400
})

port.write("at z\r")

port.on("data", function(dataRaw) {
    data = String(dataRaw)
    if (data.includes(">")) port.write("010C\r")
    else if (data.startsWith("41 0C")) {
        //6,7 9,10
        console.log((256 * parseInt(data[6] + data[7], 16) + parseInt(data[9] + data[10], 16)) / 4)
    }
    else if (!data.includes("010C")) console.log(data)
})