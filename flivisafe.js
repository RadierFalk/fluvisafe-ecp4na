const BAUD_RATE = 115200;

let port, reader;

const el = id => document.getElementById(id);

async function connectSerial() {
  try {
    port = await navigator.serial.requestPort();
    await port.open({ baudRate: BAUD_RATE });

    const decoder = new TextDecoderStream();
    port.readable.pipeTo(decoder.writable);

    const input = decoder.readable.getReader();
    reader = input;

    readLoop();
  } catch (err) {
    console.error("Erro ao conectar Serial:", err);
  }
}

async function readLoop() {
  let buffer = "";

  while (true) {
    const { value, done } = await reader.read();
    if (done) break;

    buffer += value;
    const lines = buffer.split("\n");
    buffer = lines.pop();

    for (const line of lines) {
      parseLine(line.trim());
    }
  }
}

function parseLine(line) {
  if (!line) return;

  if (line.startsWith("DRAFT:")) {
    const parts = line.split(":");
    const sensor   = parts[1];
    const draft    = parseFloat(parts[2]);
    const pressure = parseFloat(parts[3]);
    updateDraft(sensor, draft, pressure);
  }

  if (line.startsWith("IMU:")) {
    const parts = line.split(":");
    updateIMU({
      ax:     parseFloat(parts[1]),
      ay:     parseFloat(parts[2]),
      az:     parseFloat(parts[3]),
      gx:     parseFloat(parts[4]),
      gy:     parseFloat(parts[5]),
      gz:     parseFloat(parts[6]),
      roll:   parseFloat(parts[7]),
      pitch:  parseFloat(parts[8]),
      temp:   parseFloat(parts[9]),
      status: parts[10]
    });
  }

  if (line.startsWith("TRIM:")) {
    const trim = parseFloat(line.split(":")[1]);
    updateTrim(trim);
  }
}

function updateDraft(sensor, draft, pressure) {
  const map = {
    PROA_BB: { val: "d1", bar: "b1", pres: "p1", badge: "s1badge" },
    POPA_BB: { val: "d2", bar: "b2", pres: "p2", badge: "s2badge" },
    PROA_EB: { val: "d3", bar: "b3", pres: "p3", badge: "s3badge" },
    POPA_EB: { val: "d4", bar: "b4", pres: "p4", badge: "s4badge" }
  };

  const ids = map[sensor];
  if (!ids) return;

  const draftEl = el(ids.val);
  if (draftEl) draftEl.innerHTML = `${draft.toFixed(2)}<span>m</span>`;

  const barEl = el(ids.bar);
  if (barEl) barEl.style.width = `${(draft / 2.0) * 100}%`;

  const presEl = el(ids.pres);
  if (presEl) presEl.textContent = pressure.toFixed(1);

  const badgeEl = el(ids.badge);
  if (badgeEl) {
    if (draft > 1.8) {
      badgeEl.textContent = "ALTO";
      badgeEl.style.color = "#f56565";
    } else if (draft > 1.5) {
      badgeEl.textContent = "ATEN.";
      badgeEl.style.color = "#FAC775";
    } else {
      badgeEl.textContent = "OK";
      badgeEl.style.color = "";
    }
  }
}

function updateIMU(data) {
  const set = (id, val) => { const e = el(id); if (e) e.textContent = val; };

  set("ax",  `${data.ax  >= 0 ? "+" : ""}${data.ax.toFixed(2)} g`);
  set("ay",  `${data.ay  >= 0 ? "+" : ""}${data.ay.toFixed(2)} g`);
  set("az",  `${data.az  >= 0 ? "+" : ""}${data.az.toFixed(2)} g`);
  set("gx",  `${data.gx.toFixed(1)} °/s`);
  set("gy",  `${data.gy.toFixed(1)} °/s`);
  set("tmp", `${data.temp.toFixed(1)} °C`);

  const rollEl = el("rollVal");
  if (rollEl) rollEl.textContent = `${data.roll.toFixed(1)}°`;

  const trimEl = el("trimVal");
  if (trimEl) trimEl.textContent = `${data.pitch.toFixed(1)}°`;

  const stabilityEl = el("stability");
  const rollStatus  = el("rollStatus");
  const trimStatus  = el("trimStatus");

  const statusColors = {
    BOA:     "#1D9E75",
    ATENCAO: "#FAC775",
    CRITICO: "#f56565"
  };

  if (stabilityEl) {
    stabilityEl.textContent = data.status;
    stabilityEl.style.color = statusColors[data.status] || "#1D9E75";
  }

  if (rollStatus)  rollStatus.textContent  = data.status;
  if (trimStatus)  trimStatus.textContent  = data.status;

  updateBubble(data.roll, data.pitch);
}

function updateBubble(roll, pitch) {
  const bubble  = document.getElementById("attBubble");
  const bubble2 = document.getElementById("attBubble2");
  if (!bubble || !bubble2) return;

  const maxOffset = 20;
  const x = Math.max(-maxOffset, Math.min(maxOffset, pitch * 0.8));
  const y = Math.max(-maxOffset, Math.min(maxOffset, roll  * 0.8));

  bubble.setAttribute("cx",  x);
  bubble.setAttribute("cy",  y);
  bubble2.setAttribute("cx", x);
  bubble2.setAttribute("cy", y);
}

function updateTrim(trim) {
  const trimEl = el("trimVal");
  if (trimEl) trimEl.textContent = `${trim.toFixed(2)}°`;
}

function startClock() {
  const clockEl = el("clock");
  if (!clockEl) return;
  setInterval(() => {
    clockEl.textContent = new Date().toLocaleTimeString("pt-BR");
  }, 1000);
}

function addConnectButton() {
  const header = document.querySelector(".header-status");
  if (!header) return;

  const btn = document.createElement("button");
  btn.textContent = "Conectar ESP32";
  btn.style.cssText = `
    margin-left: 12px;
    padding: 4px 10px;
    background: #1D9E75;
    color: #fff;
    border: none;
    border-radius: 4px;
    font-size: 11px;
    font-family: inherit;
    cursor: pointer;
  `;

  btn.addEventListener("click", async () => {
    await connectSerial();
    btn.textContent = "Conectado";
    btn.style.background = "#085041";
    btn.disabled = true;
  });

  header.appendChild(btn);
}

document.addEventListener("DOMContentLoaded", () => {
  startClock();
  addConnectButton();
});