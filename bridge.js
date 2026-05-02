/**
 * bridge.js — WebSocket ↔ C TCP bridge
 *
 * - Serves index.html on port 8080
 * - Each browser player gets one real TCP connection to the C server (port 5000)
 * - Speaks the C server's length-prefix protocol:  "<len>\n<message>"
 * - Zero changes to server.c or client.c
 */

const http       = require("http");
const fs         = require("fs");
const path       = require("path");
const net        = require("net");
const WebSocket  = require("ws");

const WEB_PORT    = 4444;   // browser connects here
const C_HOST      = "127.0.0.1";
const C_PORT      = 5000;   // your C server

// ─── HTTP server (serves index.html) ─────────────────────────────────────────
const httpServer = http.createServer((req, res) => {
  const file = path.join(__dirname, "index.html");
  fs.readFile(file, (err, data) => {
    if (err) { res.writeHead(404); res.end("Not found"); return; }
    res.writeHead(200, { "Content-Type": "text/html" });
    res.end(data);
  });
});

// ─── WebSocket server ─────────────────────────────────────────────────────────
const wss = new WebSocket.Server({ server: httpServer });

wss.on("connection", (ws) => {
  console.log("[bridge] Browser player connected");

  // Open a dedicated TCP connection to the C server for this player
  const tcp = new net.Socket();
  let tcpBuffer = "";        // accumulates raw TCP data
  let expectedLen = null;    // null = waiting for length line

  tcp.connect(C_PORT, C_HOST, () => {
    console.log("[bridge] TCP connection to C server established");
    sendToWs(ws, { type: "connected" });
  });

  tcp.on("error", (err) => {
    console.error("[bridge] TCP error:", err.message);
    sendToWs(ws, { type: "error", message: "Cannot reach C server. Is it running?" });
    ws.close();
  });

  tcp.on("close", () => {
    console.log("[bridge] TCP connection closed");
    ws.close();
  });

  // ── C protocol parser ──────────────────────────────────────────────────────
  // Format: "<length>\n<message body of exactly length bytes>"
  tcp.on("data", (chunk) => {
    tcpBuffer += chunk.toString();

    while (true) {
      if (expectedLen === null) {
        // Look for the length line
        const nl = tcpBuffer.indexOf("\n");
        if (nl === -1) break;                          // not enough data yet
        expectedLen = parseInt(tcpBuffer.slice(0, nl), 10);
        tcpBuffer   = tcpBuffer.slice(nl + 1);
      }

      if (tcpBuffer.length < expectedLen) break;       // message body not complete

      const body  = tcpBuffer.slice(0, expectedLen);
      tcpBuffer   = tcpBuffer.slice(expectedLen);
      expectedLen = null;

      console.log(`[bridge] C→Browser: ${body.trim()}`);
      // Forward the raw C message text to the browser
      sendToWs(ws, { type: "c_message", text: body });
    }
  });

  // ── Browser → C server ────────────────────────────────────────────────────
  ws.on("message", (raw) => {
    let msg;
    try { msg = JSON.parse(raw); } catch { return; }

    if (msg.type === "answer") {
      const answer = (msg.answer || "").toUpperCase()[0] || "A";
      console.log(`[bridge] Browser→C: ${answer}`);
      tcp.write(answer);          // C client sends exactly one character
    }
  });

  ws.on("close", () => {
    console.log("[bridge] Browser player disconnected");
    tcp.destroy();
  });
});

function sendToWs(ws, obj) {
  if (ws.readyState === WebSocket.OPEN)
    ws.send(JSON.stringify(obj));
}

httpServer.listen(WEB_PORT, () => {
  console.log(`\n🌐 Bridge running at http://localhost:${WEB_PORT}`);
  console.log(`   Make sure C server is running first:  ./server\n`);
});