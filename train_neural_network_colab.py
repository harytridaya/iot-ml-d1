# =========================================================
# DEMO 1 - Train Neural Network untuk Cloud AI ThingSpeak
# Bisa dijalankan langsung di Google Colab.
# =========================================================

import numpy as np
import pandas as pd

from sklearn.model_selection import train_test_split
from sklearn.preprocessing import StandardScaler
from sklearn.neural_network import MLPClassifier
from sklearn.metrics import accuracy_score, confusion_matrix, classification_report

# ---------------------------------------------------------
# 1. LOAD DATASET
# ---------------------------------------------------------
# Upload file CSV Anda ke Colab dengan nama:
# labeled_dataset.csv
#
# Kolom wajib:
# temperature, humidity, light, motion, label
#
# label harus:
# NORMAL / WARNING / CRITICAL
#
# Jika ingin mencoba source package ini terlebih dahulu,
# upload sample_dataset.csv lalu ubah nama file di bawah.

CSV_FILE = "labeled_dataset.csv"

df = pd.read_csv(CSV_FILE)

required = [
    "temperature",
    "humidity",
    "light",
    "motion",
    "label",
]

missing = [c for c in required if c not in df.columns]
if missing:
    raise ValueError(f"Kolom dataset kurang: {missing}")

# ---------------------------------------------------------
# 2. LABEL MAPPING
# ---------------------------------------------------------
label_map = {
    "NORMAL": 0,
    "WARNING": 1,
    "CRITICAL": 2,
}

df["label_id"] = (
    df["label"]
    .astype(str)
    .str.upper()
    .map(label_map)
)

if df["label_id"].isna().any():
    bad = df[df["label_id"].isna()]["label"].unique()
    raise ValueError(f"Label tidak dikenal: {bad}")

features = [
    "temperature",
    "humidity",
    "light",
    "motion",
]

X = df[features].astype(float).values
y = df["label_id"].astype(int).values

# ---------------------------------------------------------
# 3. TRAIN / TEST SPLIT
# ---------------------------------------------------------
X_train, X_test, y_train, y_test = train_test_split(
    X,
    y,
    test_size=0.25,
    random_state=42,
    stratify=y,
)

# ---------------------------------------------------------
# 4. NORMALIZATION
# ---------------------------------------------------------
scaler = StandardScaler()

X_train_scaled = scaler.fit_transform(X_train)
X_test_scaled = scaler.transform(X_test)

# ---------------------------------------------------------
# 5. NEURAL NETWORK 4 -> 8 -> 3
# ---------------------------------------------------------
model = MLPClassifier(
    hidden_layer_sizes=(8,),
    activation="relu",
    solver="adam",
    alpha=0.001,
    learning_rate_init=0.01,
    max_iter=5000,
    random_state=42,
)

model.fit(X_train_scaled, y_train)

# ---------------------------------------------------------
# 6. EVALUATION
# ---------------------------------------------------------
pred = model.predict(X_test_scaled)

print("Accuracy:", accuracy_score(y_test, pred))
print()
print("Confusion Matrix:")
print(confusion_matrix(y_test, pred))
print()
print(
    classification_report(
        y_test,
        pred,
        target_names=[
            "NORMAL",
            "WARNING",
            "CRITICAL",
        ],
    )
)

# ---------------------------------------------------------
# 7. EXPORT PARAMETERS
# ---------------------------------------------------------
# scikit-learn:
# coefs_[0] = 4 x 8
# coefs_[1] = 8 x 3
#
# MATLAB inference yang kita gunakan:
# W1 = 8 x 4
# W2 = 3 x 8
#
# Karena itu weights ditranspose.

input_mean = scaler.mean_
input_scale = scaler.scale_

W1 = model.coefs_[0].T
b1 = model.intercepts_[0]

W2 = model.coefs_[1].T
b2 = model.intercepts_[1]

def matlab_vector(name, arr):
    arr = np.asarray(arr).reshape(-1)
    values = " ".join(f"{x:.10f}" for x in arr)
    return f"{name} = [{values}]';"

def matlab_matrix(name, arr):
    arr = np.asarray(arr)
    rows = []
    for row in arr:
        rows.append(" ".join(f"{x:.10f}" for x in row))
    return f"{name} = [\n  " + ";\n  ".join(rows) + "\n];"

matlab_text = "\n".join([
    "% ===== EXPORTED FROM GOOGLE COLAB =====",
    matlab_vector("inputMean", input_mean),
    matlab_vector("inputScale", input_scale),
    matlab_matrix("W1", W1),
    matlab_vector("b1", b1),
    matlab_matrix("W2", W2),
    matlab_vector("b2", b2),
    "% Class mapping: 1=NORMAL, 2=WARNING, 3=CRITICAL",
])

print()
print(matlab_text)

with open("nn_weights_matlab.txt", "w") as f:
    f.write(matlab_text)

print()
print("Saved: nn_weights_matlab.txt")

# ---------------------------------------------------------
# 8. QUICK TEST
# ---------------------------------------------------------
test_input = np.array([
    [26.0, 60.0, 2000.0, 0],
    [31.0, 74.0, 1300.0, 1],
    [40.0, 88.0, 500.0, 1],
])

prob = model.predict_proba(
    scaler.transform(test_input)
)

for x, p in zip(test_input, prob):
    print()
    print("Input:", x)
    print(
        "NORMAL={:.3f}, WARNING={:.3f}, CRITICAL={:.3f}".format(
            p[0], p[1], p[2]
        )
    )
