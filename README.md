# Demo 1 — Cloud Neural Network AIoT

## Architecture

DHT22 + LDR + PIR
→ Wokwi ESP32
→ ThingSpeak RAW Channel
→ React
→ MATLAB Analysis / Neural Network Inference
→ AI Result Channel
→ ESP32 Actuator + Email + Telegram

Training:
CSV labeled dataset → Google Colab → Neural Network 4→8→3 → export weights → MATLAB Analysis.

## Files

- `diagram.json` — Wokwi hardware
- `libraries.txt` — Wokwi libraries
- `sketch.ino` — ESP32 sensor upload + AI status polling + actuator
- `sample_dataset.csv` — synthetic classroom dataset for quick test
- `train_neural_network_colab.py` — MLP training in Google Colab
- `matlab_neural_network_inference.m` — ThingSpeak inference + email alert

## ThingSpeak Channel 1 — AIoT RAW Sensor

- Field 1: Temperature
- Field 2: Humidity
- Field 3: Light
- Field 4: Motion

Update `RAW_WRITE_API_KEY` in `sketch.ino`.

## ThingSpeak Channel 2 — AIoT AI Result

- Field 1: Normal Probability
- Field 2: Warning Probability
- Field 3: Critical Probability
- Field 4: AI Status (`0=NORMAL`, `1=WARNING`, `2=CRITICAL`)
- Field 5: Confidence (%)
- Field 6: Temperature
- Field 7: Humidity
- Field 8: Light

Update:
- `AI_CHANNEL_ID`
- `AI_READ_API_KEY`

in `sketch.ino`.

Update channel IDs and keys in `matlab_neural_network_inference.m`.

## Step 1 — Wokwi

Create a new ESP32 project and replace:
- `diagram.json`
- `sketch.ino`

Add `libraries.txt`.

Run the simulator.

## Step 2 — Build Dataset

For your own model, collect sensor data from ThingSpeak and make a CSV:

```csv
temperature,humidity,light,motion,label
26,60,2000,0,NORMAL
31,74,1300,1,WARNING
40,88,500,1,CRITICAL
```

For a fast classroom demo, use `sample_dataset.csv`.

## Step 3 — Train in Google Colab

Upload:
- `train_neural_network_colab.py`
- your `labeled_dataset.csv`

Or rename `sample_dataset.csv` to `labeled_dataset.csv`.

Run the script.

It outputs:
`nn_weights_matlab.txt`

Copy these values into:
`matlab_neural_network_inference.m`

Replace:
- `inputMean`
- `inputScale`
- `W1`
- `b1`
- `W2`
- `b2`

The included MATLAB file already contains example weights trained from the included sample dataset.

## Step 4 — MATLAB Analysis

ThingSpeak:
Apps → MATLAB Analysis → New → Custom

Paste `matlab_neural_network_inference.m`.

Replace:
- RAW channel ID/read key
- AI Result channel ID/read/write key
- Alerts API Key

Save and Run.

## Step 5 — React: Run Neural Network

Apps → React → New React

Suggested settings:

- Name: `Run Cloud Neural Network`
- Condition Type: Numeric
- Channel: RAW Sensor
- Field: Temperature
- Condition: `is greater than`
- Value: `-100`
- Test Frequency: `On Data Insertion`
- Action: `MATLAB Analysis`
- Select your NN inference analysis
- Run every time condition is met

## Step 6 — Telegram

Create a Telegram Bot using `@BotFather`.

Create ThingHTTP:

- Method: POST
- URL:
  `https://api.telegram.org/bot<BOT_TOKEN>/sendMessage`
- Content type: `application/json`

Example body:

```json
{
  "chat_id": "YOUR_CHAT_ID",
  "text": "AIoT CRITICAL ALERT\nCritical P: %%channel_AI_CHANNEL_ID_field_3%%\nConfidence: %%channel_AI_CHANNEL_ID_field_5%%%\nTemperature: %%channel_AI_CHANNEL_ID_field_6%% C\nHumidity: %%channel_AI_CHANNEL_ID_field_7%%%"
}
```

Create React:

- Channel: AI Result
- Field: 4
- Condition: `== 2`
- Test Frequency: On Data Insertion
- Action: ThingHTTP
- Option: Run action only first time condition is met

## Demo Classes

### NORMAL
Example:
- Temperature 24–28 C
- Humidity 50–68 %
- Light 1500–2600 ADC

Actuator:
- Green ON
- Buzzer OFF
- Servo 0°

### WARNING
Example:
- Temperature 29–34 C
- Humidity 68–80 %
- Light 900–1800 ADC

Actuator:
- Yellow ON
- short beep
- Servo 45°

### CRITICAL
Example:
- Temperature 35–45 C
- Humidity 80–95 %
- Light 100–900 ADC

Actuator:
- Red ON
- continuous alarm
- Servo 90°
- Email
- Telegram

## Important

The sample dataset and included weights are for classroom demonstration only.
For a meaningful real-world AI system, collect representative labeled sensor data and retrain the model.
