const express = require('express');
const cors = require('cors');
const admin = require('firebase-admin');

// 1. Initialize Firebase Admin SDK
// You must download your Service Account JSON file from Firebase Project Settings -> Service Accounts -> Generate New Private Key
// Save it in the api/ folder as 'serviceAccountKey.json'
try {
  const serviceAccount = require('./serviceAccountKey.json');
  admin.initializeApp({
    credential: admin.credential.cert(serviceAccount),
    // Replace with your DB URL
    databaseURL: "https://YOUR-PROJECT-ID.firebaseio.com"
  });
  console.log("Firebase Admin Initialized successfully.");
} catch (error) {
  console.warn("WARNING: serviceAccountKey.json not found or invalid.");
  console.warn("Please generate a private key from Firebase -> Project Settings -> Service Accounts and save as api/serviceAccountKey.json");
}

const db = admin.database ? admin.database() : null;
const app = express();

app.use(cors());
app.use(express.json());

// 2. Health Check Endpoint
app.get('/api/health', (req, res) => {
  res.status(200).json({ status: "API is running" });
});

// 3. Receive Alert Endpoint (Used by Root Node)
app.post('/api/alerts', async (req, res) => {
  const { nodeId, status } = req.body;

  if (!nodeId) {
    return res.status(400).json({ error: "Missing nodeId in payload" });
  }

  // Fallback to local timestamp if db isn't initialized yet to not crash
  const alertPayload = {
    nodeId: parseInt(nodeId),
    status: status || "ALERT",
    timestamp: db ? admin.database.ServerValue.TIMESTAMP : Date.now()
  };

  try {
    if (db) {
      // Write to Firebase Realtime Database using Admin SDK
      const newAlertRef = db.ref('alerts').push();
      await newAlertRef.set(alertPayload);
      console.log(`[+] Logged alert from Node ${nodeId} to Firebase`);
    } else {
      console.log(`[DEVELOPMENT MODE] Received alert from Node ${nodeId} (Firebase not configured)`);
    }

    res.status(201).json({ success: true, message: "Alert saved" });
  } catch (error) {
    console.error("Error writing to Firebase:", error);
    res.status(500).json({ error: "Failed to write data" });
  }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, '0.0.0.0', () => {
  console.log(`\n===========================================`);
  console.log(`🔥 Mesh API running at http://0.0.0.0:${PORT}`);
  console.log(`===========================================\n`);
  console.log(`ESP8266 Root Node -> Set HTTP Target to: http://<YOUR_LOCAL_IP>:${PORT}/api/alerts\n`);
});
