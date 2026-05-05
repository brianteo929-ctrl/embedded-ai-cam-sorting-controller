const express = require("express");
const multer = require("multer");

const app = express();
const upload = multer({ storage: multer.memoryStorage() });

let latestFrame = null;

// Receive image from ESP32
app.post("/upload", upload.single("image"), (req, res) => {
    if (req.file) {
        latestFrame = req.file.buffer;
    }
    res.sendStatus(200);
});

// Stream endpoint
app.get("/stream", (req, res) => {
    res.writeHead(200, {
        "Content-Type": "multipart/x-mixed-replace; boundary=frame",
        "Cache-Control": "no-cache",
        "Connection": "keep-alive",
        "Pragma": "no-cache"
    });

    const interval = setInterval(() => {
        if (latestFrame) {
            res.write(`--frame\r\nContent-Type: image/jpeg\r\n\r\n`);
            res.write(latestFrame);
            res.write("\r\n");
        }
    }, 100);

    req.on("close", () => clearInterval(interval));
});

// Homepage
app.get("/", (req, res) => {
    res.send(`
        <html>
        <body style="text-align:center;">
            <h2>ESP32 Live Stream</h2>
            <img src="/stream" width="640"/>
        </body>
        </html>
    `);
});

app.listen(3000, () => {
    console.log("🚀 Server running at http://localhost:3000");
});