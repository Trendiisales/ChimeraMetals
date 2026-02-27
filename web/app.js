(function () {
  const fleetEl = document.getElementById("fleet");
  const latGridEl = document.getElementById("latGrid");
  const clockEl = document.getElementById("clock");
  const netStateEl = document.getElementById("netState");

  // Add VPS endpoints here (public URLs).
  // You can include your current VPS:
  //  - http://185.167.119.59:7777
  const VPS = [
    { name: "VPS-01", base: window.location.origin },
    // { name: "VPS-02", base: "http://<IP>:7777" },
    // { name: "VPS-03", base: "http://<IP>:7777" }
  ];

  function fmtMs(x) {
    if (x === null || x === undefined) return "--";
    if (typeof x !== "number") return "--";
    if (x < 0) return "--";
    return x.toFixed(2) + "ms";
  }

  function pillClass(ms) {
    if (ms < 0) return "bad";
    if (ms <= 10) return "good";
    if (ms <= 25) return "warn";
    return "bad";
  }

  function badgeFromOk(ok) {
    return ok ? "ok" : "bad";
  }

  function setNetState(ok) {
    netStateEl.textContent = ok ? "CONNECTED" : "DISCONNECTED";
    netStateEl.classList.remove("ok", "bad");
    netStateEl.classList.add(ok ? "ok" : "bad");
  }

  function tickClock() {
    const d = new Date();
    const hh = String(d.getHours()).padStart(2, "0");
    const mm = String(d.getMinutes()).padStart(2, "0");
    const ss = String(d.getSeconds()).padStart(2, "0");
    clockEl.textContent = hh + ":" + mm + ":" + ss;
  }

  async function fetchJson(url) {
    const r = await fetch(url, { cache: "no-store" });
    if (!r.ok) throw new Error("HTTP " + r.status);
    return await r.json();
  }

  function renderFleet(cards) {
    fleetEl.innerHTML = "";
    for (const c of cards) {
      const tile = document.createElement("div");
      tile.className = "tile";
      const badgeCls = badgeFromOk(c.ok);

      tile.innerHTML = `
        <div class="tileTop">
          <div>
            <div class="tileName">${c.name}</div>
            <div class="tileUrl">${c.base}</div>
          </div>
          <div class="badge ${badgeCls}">${c.ok ? "OK" : "DOWN"}</div>
        </div>
        <div class="tileStatRow">
          <div class="kpi">
            <div class="kpiLabel">Uptime</div>
            <div class="kpiVal">${c.uptime_s !== null ? c.uptime_s + "s" : "--"}</div>
          </div>
          <div class="kpi">
            <div class="kpiLabel">Listen</div>
            <div class="kpiVal">${c.listen_port !== null ? c.listen_port : "--"}</div>
          </div>
        </div>
      `;
      fleetEl.appendChild(tile);
    }
  }

  function renderLatency(rows) {
    latGridEl.innerHTML = "";
    for (const r of rows) {
      const row = document.createElement("div");
      row.className = "latRow";

      const p50cls = pillClass(r.p50_ms);
      const p95cls = pillClass(r.p95_ms);

      row.innerHTML = `
        <div class="latLeft">
          <div class="latName">${r.vpsName} ? ${r.label}</div>
          <div class="latHost">${r.host}:${r.port}</div>
        </div>
        <div class="latRight">
          <div class="pill ${pillClass(r.last_ms)}">last ${fmtMs(r.last_ms)}</div>
          <div class="pill ${p50cls}">p50 ${fmtMs(r.p50_ms)}</div>
          <div class="pill ${p95cls}">p95 ${fmtMs(r.p95_ms)}</div>
          <div class="pill">n ${r.samples}</div>
        </div>
      `;
      latGridEl.appendChild(row);
    }
  }

  async function refresh() {
    const cards = [];
    const latRows = [];

    let anyOk = false;

    for (const v of VPS) {
      const url = v.base.replace(/\/+$/, "") + "/api/status";
      try {
        const j = await fetchJson(url);

        anyOk = true;
        cards.push({
          name: v.name,
          base: v.base,
          ok: true,
          uptime_s: j.uptime_s ?? null,
          listen_port: j.listen_port ?? null
        });

        if (Array.isArray(j.probes)) {
          for (const p of j.probes) {
            latRows.push({
              vpsName: v.name,
              label: p.label,
              host: p.host,
              port: p.port,
              last_ms: p.last_ms,
              p50_ms: p.p50_ms,
              p95_ms: p.p95_ms,
              samples: p.samples
            });
          }
        }
      } catch (e) {
        cards.push({
          name: v.name,
          base: v.base,
          ok: false,
          uptime_s: null,
          listen_port: null
        });
      }
    }

    setNetState(anyOk);
    renderFleet(cards);
    renderLatency(latRows);
  }

  tickClock();
  setInterval(tickClock, 1000);

  refresh();
  setInterval(refresh, 1000);
})();
