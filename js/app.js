const ESP32_IP = "192.168.1.105";

async function send(cmd) {
  const status = document.getElementById("status");
  status.textContent = `Sending ${cmd}...`;
  try {
    const r = await fetch(`http://${ESP32_IP}/command?cmd=${cmd}`);
    status.textContent = await r.text();
  } catch (e) {
    status.textContent = "ESP32 connection failed";
  }
}
