const baseURL = "http://192.168.8.100:8080";

function refreshAll() {
  fetch(`${baseURL}/bpm/`)
    .then(res => res.json())
    .then(data => {
      document.getElementById("bpm").textContent = data;
    });

  fetch(`${baseURL}/bpm/min/`)
    .then(res => res.json())
    .then(data => {
      document.getElementById("bpmMin").textContent = data;
    });

  fetch(`${baseURL}/bpm/max/`)
    .then(res => res.json())
    .then(data => {
      document.getElementById("bpmMax").textContent = data;
    });
}

function setBpm() {
  const newBpm = document.getElementById("newBpm").value;
  if (!newBpm) {
    alert("Please enter a BPM value.");
    return;
  }

  fetch(`${baseURL}/bpm/`, {
    method: "PUT",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(parseInt(newBpm))
  })
  .then(res => {
    if (res.ok) {
      alert("BPM updated.");
      refreshAll();
    } else {
      alert("Failed to set BPM.");
    }
  });
}

function resetMin() {
  fetch(`${baseURL}/bpm/min/`, { method: "DELETE" })
    .then(res => {
      if (res.ok) {
        alert("Min BPM reset.");
        refreshAll();
      } else {
        alert("Failed to reset Min BPM.");
      }
    });
}

function resetMax() {
  fetch(`${baseURL}/bpm/max/`, { method: "DELETE" })
    .then(res => {
      if (res.ok) {
        alert("Max BPM reset.");
        refreshAll();
      } else {
        alert("Failed to reset Max BPM.");
      }
    });
}

// Auto-load data on page load
window.onload = refreshAll;
