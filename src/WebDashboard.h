#ifndef WEB_DASHBOARD_H
#define WEB_DASHBOARD_H

#include <Arduino.h>

const char index_html[] PROGMEM = R"rawliteral(
    <!DOCTYPE HTML>
    <html>
        <head>
            <title>ESP32 Power Monitor</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <link rel="stylesheet" href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0/css/all.min.css">
            <style>
                body { font-family: Arial, sans-serif; margin: 0; padding: 20px; background-color: #f0f0f0; }
                .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; max-width: 1200px; margin: 0 auto; }
                .card { background: white; border-radius: 15px; padding: 25px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); display: flex; align-items: center; text-align: left; }
                .icon { font-size: 40px; margin-right: 25px; min-width: 50px; text-align: center; }
                .content { display: flex; flex-direction: column; }
                .label { font-size: 16px; color: #7f8c8d; margin-bottom: 5px; font-weight: bold; }
                .sublabel { font-size: 12px; color: #bdc3c7; margin-top: 3px; }
                .value { font-size: 24px; color: #2c3e50; display: flex; align-items: baseline; }
                .unit { font-size: 16px; color: #95a5a6; margin-left: 5px; }
                .fa-bolt { color: #f1c40f; } .fa-exchange-alt { color: #3498db; } .fa-plug { color: #e74c3c; }
                .fa-chart-line { color: #2ecc71; } .fa-wave-square { color: #9b59b6; } .fa-percent { color: #e67e22; }
                .fa-history { color: #16a085; } .fa-calculator { color: #d35400; } .fa-map-marker-alt { color: #e74c3c; } .fa-clock { color: #8e44ad; }
                h1 { text-align: center; margin: 30px 0; color: #2c3e50; }
            </style>
            <script>
                function updateData() {
                var xhttp = new XMLHttpRequest();
                xhttp.onreadystatechange = function() {
                    if (this.readyState == 4 && this.status == 200) {
                    var data = JSON.parse(this.responseText);
                    document.getElementById('voltage').innerHTML = data.voltage + '<span class="unit">V</span>';
                    document.getElementById('current').innerHTML = data.current + '<span class="unit">A</span>';
                    document.getElementById('power').innerHTML = data.power + '<span class="unit">W</span>';
                    document.getElementById('energy').innerHTML = data.energy + '<span class="unit">kWh</span>';
                    document.getElementById('frequency').innerHTML = data.frequency + '<span class="unit">Hz</span>';
                    document.getElementById('pf').innerHTML = data.pf;
                    document.getElementById('last_hour').innerHTML = data.last_hour + '<span class="unit">kWh</span>';
                    document.getElementById('last_hour_label').innerText = data.last_hour_label;
                    document.getElementById('predicted_hour').innerHTML = data.predicted_hour + '<span class="unit">kWh</span>';
                    document.getElementById('location').innerText = data.location;
                    document.getElementById('time').innerText = data.time;
                    }
                };
                xhttp.open("GET", "/data", true);
                xhttp.send();
                }
                setInterval(updateData, 2000);
                window.onload = updateData;
            </script>
        </head>
        <body>
            <h1><i class="fas fa-plug"></i> ESP32 Power Monitor</h1>
            <div class="grid">
                <div class="card"><i class="fas fa-map-marker-alt icon"></i><div class="content"><div class="label">LOCATION</div><div class="value" id="location">%LOCATION%</div><div class="sublabel">Auto-detected via Secure IP-API</div></div></div>
                <div class="card"><i class="fas fa-clock icon"></i><div class="content"><div class="label">SYSTEM TIME</div><div class="value" id="time">%TIME%</div><div class="sublabel">Synchronized via NTP</div></div></div>
                <div class="card"><i class="fas fa-bolt icon"></i><div class="content"><div class="label">VOLTAGE</div><div class="value" id="voltage">%VOLTAGE%<span class="unit">V</span></div></div></div>
                <div class="card"><i class="fas fa-exchange-alt icon"></i><div class="content"><div class="label">CURRENT</div><div class="value" id="current">%CURRENT%<span class="unit">A</span></div></div></div>
                <div class="card"><i class="fas fa-plug icon"></i><div class="content"><div class="label">POWER</div><div class="value" id="power">%POWER%<span class="unit">W</span></div></div></div>
                <div class="card"><i class="fas fa-chart-line icon"></i><div class="content"><div class="label">TOTAL ACCUMULATED</div><div class="value" id="energy">%ENERGY%<span class="unit">kWh</span></div></div></div>
                <div class="card"><i class="fas fa-history icon"></i><div class="content"><div class="label">LAST HOUR CONSUMPTION</div><div class="value" id="last_hour">%LAST_HOUR%<span class="unit">kWh</span></div><div class="sublabel" id="last_hour_label">%LAST_HOUR_LABEL%</div></div></div>
                <div class="card"><i class="fas fa-calculator icon"></i><div class="content"><div class="label">PREDICTED THIS HOUR</div><div class="value" id="predicted_hour">%PREDICTED_HOUR%<span class="unit">kWh</span></div><div class="sublabel">Projected end-of-hour total</div></div></div>
                <div class="card"><i class="fas fa-wave-square icon"></i><div class="content"><div class="label">FREQUENCY</div><div class="value" id="frequency">%FREQUENCY%<span class="unit">Hz</span></div></div></div>
                <div class="card"><i class="fas fa-percent icon"></i><div class="content"><div class="label">POWER FACTOR</div><div class="value" id="pf">%PF%</div></div></div>
            </div>
        </body>
    </html>
)rawliteral";

#endif