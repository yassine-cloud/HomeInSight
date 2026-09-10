#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE html>
    <html lang="en">
        <head>
            <meta charset="UTF-8">
            <meta name="viewport" content="width=device-width, initial-scale=1.0">
            <title>ESP32 Power Monitor</title>
            <style>
                :root { --bg: #0f172a; --card: #1e293b; --text: #f8fafc; --accent: #38bdf8; --danger: #ef4444; }
                body { font-family: system-ui, -apple-system, sans-serif; background: var(--bg); color: var(--text); margin: 0; padding: 20px; }
                .container { max-width: 900px; margin: 0 auto; }
                .header { display: flex; justify-content: space-between; align-items: center; border-bottom: 1px solid #334155; padding-bottom: 15px; margin-bottom: 20px; }
                .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px; }
                .card { background: var(--card); padding: 20px; border-radius: 10px; border: 1px solid #334155; }
                .card .label { font-size: 0.85rem; color: #94a3b8; text-transform: uppercase; letter-spacing: 0.05em; }
                .card .value { font-size: 1.8rem; font-weight: 700; margin-top: 5px; color: var(--accent); }
                .btn { background: #0284c7; color: white; border: none; padding: 12px 20px; border-radius: 6px; cursor: pointer; font-weight: 600; transition: 0.2s; }
                .btn:hover { opacity: 0.9; }
                .btn.active { background: var(--danger); }
                .info { font-size: 0.9rem; color: #64748b; margin-top: 15px; text-align: right; }
            </style>
        </head>
        <body>
            <div class="container">
                <div class="header">
                    <div>
                        <h1 style="margin:0;">Home In Sight Dashboard</h1>
                        <p style="margin:5px 0 0; color:#94a3b8;" id="location">%LOCATION%</p>
                    </div>
                    <button id="resetBtn" class="btn" onclick="toggleReset()">%RESET_STATUS%</button>
                </div>

                <div class="grid">
                    <div class="card"><div class="label">Voltage</div><div class="value" id="voltage">%VOLTAGE% V</div></div>
                    <div class="card"><div class="label">Current</div><div class="value" id="current">%CURRENT% A</div></div>
                    <div class="card"><div class="label">Power</div><div class="value" id="power">%POWER% W</div></div>
                    <div class="card"><div class="label">Total Energy</div><div class="value" id="energy">%ENERGY% kWh</div></div>
                    <div class="card"><div class="label">Frequency</div><div class="value" id="frequency">%FREQUENCY% Hz</div></div>
                    <div class="card"><div class="label">Power Factor</div><div class="value" id="pf">%PF%</div></div>
                    <div class="card"><div class="label">Predicted Energy (Hour)</div><div class="value" id="predicted">%PREDICTED_HOUR% kWh</div></div>
                    <div class="card"><div class="label">Last Hour (<span id="last_hour_label">%LAST_HOUR_LABEL%</span>)</div><div class="value" id="last_hour">%LAST_HOUR% kWh</div></div>
                </div>

                <div class="info">Last Synced: <span id="time">%TIME%</span></div>
            </div>

            <script>
                async function updateDashboard() {
                    try {
                        const res = await fetch('/data');
                        const data = await res.json();
                        document.getElementById('voltage').innerText = data.voltage + ' V';
                        document.getElementById('current').innerText = data.current + ' A';
                        document.getElementById('power').innerText = data.power + ' W';
                        document.getElementById('energy').innerText = data.energy + ' kWh';
                        document.getElementById('frequency').innerText = data.frequency + ' Hz';
                        document.getElementById('pf').innerText = data.pf;
                        document.getElementById('last_hour').innerText = data.last_hour + ' kWh';
                        document.getElementById('last_hour_label').innerText = data.last_hour_label;
                        document.getElementById('predicted').innerText = data.predicted_hour + ' kWh';
                        document.getElementById('location').innerText = data.location;
                        document.getElementById('time').innerText = data.time;

                        const btn = document.getElementById('resetBtn');
                        if (data.pending_reset) {
                            btn.innerText = 'Cancel Reset';
                            btn.classList.add('active');
                        } else {
                            btn.innerText = 'Schedule Reset';
                            btn.classList.remove('active');
                        }
                    } catch (err) {
                        console.error("Failed to fetch data:", err);
                    }
                }

                async function toggleReset() {
                    try {
                        const res = await POST('/api/toggle-reset');
                        updateDashboard();
                    } catch (err) {
                        fetch('/api/toggle-reset', { method: 'POST' }).then(() => updateDashboard());
                    }
                }

                setInterval(updateDashboard, 2000);
            </script>
        </body>
    </html>
)rawliteral";

#endif