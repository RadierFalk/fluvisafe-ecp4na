const el = id => document.getElementById(id);
let apiOnline = false;

const SENSOR_MAP = {
  PROA_BB: { val: "d1", bar: "b1", pres: "p1", badge: "s1badge" },
  PROA_EB: { val: "d2", bar: "b2", pres: "p2", badge: "s2badge" },
  POPA_BB: { val: "d3", bar: "b3", pres: "p3", badge: "s3badge" },
  POPA_EB: { val: "d4", bar: "b4", pres: "p4", badge: "s4badge" }
};

function updateDraft(sensor, data) {
  const ids = SENSOR_MAP[sensor];
  if (!ids) return;

  const draft = data.draft_m;
  const draftEl = el(ids.val);
  if (draftEl) draftEl.innerHTML = `${draft.toFixed(2)}<span>m</span>`;

  const barEl = el(ids.bar);
  if (barEl) barEl.style.width = `${Math.min(100, (draft / 5) * 100)}%`;

  const presEl = el(ids.pres);
  if (presEl) presEl.textContent = data.pressure_kpa.toFixed(1);

  const badgeEl = el(ids.badge);
  if (!badgeEl) return;

  if (!data.active) {
    badgeEl.textContent = "FALHA";
    badgeEl.style.color = "#f56565";
  } else if (draft > 4.2) {
    badgeEl.textContent = "ALTO";
    badgeEl.style.color = "#f56565";
  } else if (draft > 3.8) {
    badgeEl.textContent = "ATEN.";
    badgeEl.style.color = "#FAC775";
  } else {
    badgeEl.textContent = "OK";
    badgeEl.style.color = "";
  }
}

function updateIMU(imu, metrics) {
  const set = (id, value) => {
    const element = el(id);
    if (element) element.textContent = value;
  };

  set("ax", `${imu.ax >= 0 ? "+" : ""}${imu.ax.toFixed(2)} g`);
  set("ay", `${imu.ay >= 0 ? "+" : ""}${imu.ay.toFixed(2)} g`);
  set("az", `${imu.az >= 0 ? "+" : ""}${imu.az.toFixed(2)} g`);
  set("gx", `${imu.gx.toFixed(1)} °/s`);
  set("gy", `${imu.gy.toFixed(1)} °/s`);
  set("tmp", `${imu.temperature_c.toFixed(1)} °C`);
  set("rollVal", `${imu.roll.toFixed(1)}°`);
  set("trimVal", `${imu.pitch.toFixed(1)}°`);

  const stabilityEl = el("stability");
  const colors = { BOA: "#1D9E75", ATENCAO: "#FAC775", CRITICO: "#f56565" };
  if (stabilityEl) {
    stabilityEl.textContent = metrics.stability;
    stabilityEl.style.color = colors[metrics.stability] || colors.BOA;
  }

  if (el("rollStatus")) el("rollStatus").textContent = metrics.alert;
  if (el("trimStatus")) el("trimStatus").textContent = metrics.alert;
  updateBubble(imu.roll, imu.pitch);
}

function updateBubble(roll, pitch) {
  const bubble = el("attBubble");
  const bubble2 = el("attBubble2");
  if (!bubble || !bubble2) return;

  const maxOffset = 20;
  const x = Math.max(-maxOffset, Math.min(maxOffset, pitch * 0.8));
  const y = Math.max(-maxOffset, Math.min(maxOffset, roll * 0.8));
  [bubble, bubble2].forEach(item => {
    item.setAttribute("cx", x);
    item.setAttribute("cy", y);
  });
}

async function loadTelemetry() {
  try {
    const response = await fetch("/api/telemetry", { cache: "no-store" });
    if (!response.ok) throw new Error(`HTTP ${response.status}`);
    const data = await response.json();

    Object.entries(data.sensors).forEach(([name, sensor]) => updateDraft(name, sensor));
    updateIMU(data.imu, data.metrics);
    setConnectionState(true, data.scenario);
  } catch (error) {
    console.error("Falha ao consultar a API:", error);
    setConnectionState(false);
  }
}

function setConnectionState(online, scenario = "") {
  apiOnline = online;
  const liveText = document.querySelector(".header-status span:nth-child(2)");
  const dot = document.querySelector(".live-dot");
  if (liveText) liveText.textContent = online ? `SIMULAÇÃO · ${scenario.toUpperCase()}` : "SEM CONEXÃO";
  if (dot) dot.style.background = online ? "#1D9E75" : "#f56565";
}

async function changeScenario(scenario) {
  const response = await fetch(`/api/simulation/scenario/${scenario}`, { method: "POST" });
  if (!response.ok) throw new Error("Não foi possível alterar o cenário");
  await loadTelemetry();
}

function addScenarioControls() {
  const header = document.querySelector(".header-status");
  if (!header) return;

  const scenarios = {
    normal: "Normal",
    band: "Banda",
    overload: "Sobrecarga",
    sensor_failure: "Falha"
  };

  Object.entries(scenarios).forEach(([value, label]) => {
    const button = document.createElement("button");
    button.textContent = label;
    button.style.cssText = "padding:4px 8px;background:#0f1f1b;color:#9FE1CB;border:1px solid #1D9E75;border-radius:4px;cursor:pointer";
    button.addEventListener("click", () => changeScenario(value).catch(console.error));
    header.appendChild(button);
  });
}

function updateFooter() {
  const items = document.querySelectorAll(".status-item");
  if (items[0]) items[0].innerHTML = '<i class="ti ti-network"></i> API local conectada';
  if (items[1]) items[1].innerHTML = '<i class="ti ti-cpu"></i> Raspberry Pi 3 · Simulação';
}

function startClock() {
  const clockEl = el("clock");
  if (!clockEl) return;
  const update = () => { clockEl.textContent = new Date().toLocaleTimeString("pt-BR"); };
  update();
  setInterval(update, 1000);
}

document.addEventListener("DOMContentLoaded", () => {
  startClock();
  addScenarioControls();
  updateFooter();
  loadTelemetry();
  setInterval(loadTelemetry, 1000);
});
