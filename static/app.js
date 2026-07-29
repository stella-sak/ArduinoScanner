
const logElement = document.getElementById("log");
const rawLineElement = document.getElementById("raw-line");
const readingsTableBody = document.getElementById("readings-table-body");
const canvas = document.getElementById("radar-canvas");
const ctx = canvas.getContext("2d");


let scanPoints = {};
let recentReadings = [];

function addLog(message) {
    const now = new Date().toLocaleTimeString();
    logElement.textContent = `[${now}] ${message}\n` + logElement.textContent;
}
function formatValue(value, unit = "") {
    if (value === null || value === undefined) {
        return "---";
    }
    return `${value}${unit}`;
}

function setText(id, text) {
    document.getElementById(id).textContent = text;
}

function updateStatusClass(status) {
    const statusElement = document.getElementById("status");
    statusElement.className = "value";

    if (status === "SAFE") {
        statusElement.classList.add("status-safe");
    } else if (status === "WARNING") {
        statusElement.classList.add("status-warning");
    } else if (status === "STOP") {
        statusElement.classList.add("status-stop");
    } else if (status === "EMERGENCY_STOP") {
        statusElement.classList.add("status-emergency");
    }
}

function updateConnectionClass(connection) {
    const connectionElement=document.getElementById("connection");
    connectionElement.className="value";

    if(connection !== "CONNECTED") {
        connectionElement.classList.add("status-disconnected");
    }
}

function updateRecentReadings(data) {
    if (data.angle_deg === null || data.angle_deg === undefined) {
        return;
    }

    recentReadings.unshift({
        time: data.last_update || "---",
        angle: data.angle_deg,
        distance: data.distance_cm,
        status: data.status || "UNKNOWN"
    });

    recentReadings = recentReadings.slice(0,10);
    readingsTableBody.innerHTML = "";

    for (const reading of recentReadings) {
        const row = document.createElement("tr");
        const timeCell = document.createElement("td");
        timeCell.textContent = reading.time;

        const angleCell = document.createElement("td");
        angleCell.textContent = formatValue(reading.angle, "°");

        const distanceCell = document.createElement("td");
        distanceCell.textContent = reading.distance === null
            ? "---"
            : `${Number(reading.distance).toFixed(1)} cm`;

        const statusCell = document.createElement("td");
        statusCell.textContent = reading.status;
        
        row.appendChild(timeCell);
        row.appendChild(angleCell);
        row.appendChild(distanceCell);
        row.appendChild(statusCell);

        readingsTableBody.appendChild(row);
    }
}

function rememberScanPoint(data) {
    if (data.angle_deg === null || data.angle_deg === undefined) {
        return;
    }

    if (data.distance_cm === null || data.distance_cm === undefined) {
        return;
    }

    const angle = Number(data.angle_deg);
    const distance = Number(data.distance_cm);

    if (Number.isNaN(angle) || Number.isNaN(distance)) {
        return;
    }

    if (distance > 120) {
        return;
    }

    scanPoints[angle] = distance;
}

function drawRadar(data) {
    const width = canvas.width;
    const height = canvas.height;

    ctx.clearRect(0, 0, width, height);

    const centerX = width / 2;
    const centerY = height - 30;
    const maxRadius = height - 70;
    const maxDistanceCm = 120;

   
    ctx.fillStyle = "#111";
    ctx.fillRect(0, 0, width, height);

    
    ctx.strokeStyle = "#1f8f4d";
    ctx.lineWidth = 1;

    for (let r = 0.25; r <= 1.0; r += 0.25) {
        ctx.beginPath();
        ctx.arc(centerX, centerY, maxRadius * r, Math.PI, 2 * Math.PI);
        ctx.stroke();
    }

    
    for (let angle = 0; angle <= 180; angle += 30) {
        const rad = angle * Math.PI / 180;
        const x = centerX + Math.cos(rad) * maxRadius;
        const y = centerY - Math.sin(rad) * maxRadius;

        ctx.beginPath();
        ctx.moveTo(centerX, centerY);
        ctx.lineTo(x, y);
        ctx.stroke();

        ctx.fillStyle = "#9be7b3";
        ctx.font = "13px Arial";
        ctx.fillText(`${angle}°`, x - 12, y - 6);
    }

    for (const [angleText, distance] of Object.entries(scanPoints)) {
        const angle = Number(angleText);
        const rad = angle * Math.PI / 180;
        const radius = Math.min(distance / maxDistanceCm, 1) * maxRadius;
        const x = centerX + Math.cos(rad) * radius;
        const y = centerY - Math.sin(rad) * radius;

        ctx.fillStyle = "#00ff88";
        ctx.beginPath();
        ctx.arc(x, y, 4, 0, 2 * Math.PI);
        ctx.fill();
    }

    
    if (data.angle_deg !== null && data.angle_deg !== undefined) {
        const currentAngle = Number(data.angle_deg);
        const rad = currentAngle * Math.PI / 180;
        const x = centerX + Math.cos(rad) * maxRadius;
        const y = centerY - Math.sin(rad) * maxRadius;

        ctx.strokeStyle = "#ffffff";
        ctx.lineWidth = 2;
        ctx.beginPath();
        ctx.moveTo(centerX, centerY);
        ctx.lineTo(x, y);
        ctx.stroke();
    }

   
    if (data.closest_angle_deg !== null && data.closest_distance_cm !== null) {
        const angle = Number(data.closest_angle_deg);
        const distance = Number(data.closest_distance_cm);
        const rad = angle * Math.PI / 180;
        const radius = Math.min(distance / maxDistanceCm, 1) * maxRadius;
        const x = centerX + Math.cos(rad) * radius;
        const y = centerY - Math.sin(rad) * radius;

        ctx.fillStyle = "#ff4040";
        ctx.beginPath();
        ctx.arc(x, y, 8, 0, 2 * Math.PI);
        ctx.fill();
    }

    
    ctx.fillStyle = "#ffffff";
    ctx.beginPath();
    ctx.arc(centerX, centerY, 5, 0, 2 * Math.PI);
    ctx.fill();
}

async function fetchTelemetry() {
    try {
        const response = await fetch("/api/telemetry");
        const data = await response.json();

        setText("connection", data.connection || "UNKNOWN");
        setText("status", data.status || "UNKNOWN");
        setText("scan-mode", data.scanning ? "SCANNING" : "STOPPED");
        setText("angle", formatValue(data.angle_deg, "°"));

        const distanceText = data.distance_cm === null || data.distance_cm === undefined
            ? "---"
            : `${Number(data.distance_cm).toFixed(1)} cm`;
        setText("distance", distanceText);

        const closestText = data.closest_distance_cm === null || data.closest_distance_cm === undefined
            ? "---"
            : `${Number(data.closest_distance_cm).toFixed(1)} cm at ${data.closest_angle_deg}°`;
        setText("closest-object", closestText);
  
        setText("armed", data.armed === null || data.armed === undefined ? "---" : String(data.armed));
        setText("emergency-stop", data.emergency_stop === null || data.emergency_stop === undefined ? "---" : String(data.emergency_stop));
        setText("last-update", data.last_update || "---");

        rawLineElement.textContent = data.raw_line || "---";

        updateStatusClass(data.status);
        updateConnectionClass(data.connection);
        rememberScanPoint(data);
        updateRecentReadings(data);
        drawRadar(data);

    } catch (error) {
        addLog("Failed to fetch telemetry");
        console.error(error);
    }
}

async function sendCommand(command) {
    try {
        if (command === "START_SCAN" || command === "RESET") {
            scanPoints = {};
            recentReadings = [];
        }

        const response = await fetch("/api/command", {
            method: "POST",
            headers: {
                "Content-Type": "application/json"
            },
            body: JSON.stringify({ command: command })
        });

        const data = await response.json();

        if (!response.ok) {
            addLog(`Error: ${data.error}`);
            return;
        }

        addLog(`Command sent: ${command}`);
        fetchTelemetry();
    } catch (error) {
        addLog(`Failed to send command: ${command}`);
        console.error(error);
    }
}


setInterval(fetchTelemetry, 300);

fetchTelemetry();
drawRadar({});