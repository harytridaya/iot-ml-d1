% ========================================================
% DEMO 1 - ThingSpeak Cloud Neural Network Inference
%
% MODEL:
% 4 input -> 8 hidden ReLU -> 3 output Softmax
%
% AI RESULT CHANNEL:
% Field 1 = Normal Probability
% Field 2 = Warning Probability
% Field 3 = Critical Probability
% Field 4 = AI Status (0/1/2)
% Field 5 = Confidence (0..100)
% Field 6 = Temperature
% Field 7 = Humidity
% Field 8 = Light
%
% IMPORTANT:
% Bobot di bawah adalah model DEMO yang dilatih dari
% sample_dataset.csv dalam package ini.
% Untuk model Anda sendiri, jalankan train_neural_network_colab.py
% lalu ganti inputMean/inputScale/W1/b1/W2/b2.
% ========================================================

% -----------------------------
% CONFIGURATION
% -----------------------------
rawChannelID = 1111111;
rawReadKey = 'YOUR_RAW_READ_API_KEY';

aiChannelID = 2222222;
aiWriteKey = 'YOUR_AI_WRITE_API_KEY';
aiReadKey = 'YOUR_AI_READ_API_KEY';

alertsApiKey = 'YOUR_ALERTS_API_KEY';

% -----------------------------
% EXAMPLE TRAINED PARAMETERS
% -----------------------------
inputMean = [32.7500697690 73.5504815068 1250.4227381658 0.4777777778]';
inputScale = [6.4081171202 12.3789175386 683.4266424717 0.4995059287]';

W1 = [
  0.0000007154 -0.0000007227 -0.0000002853 0.0000002049;
  1.5678060316 1.3804904059 -0.9974047506 -0.1688937524;
  0.7442020752 -0.2703800678 -0.5145958935 0.1336912417;
  0.1433173768 1.2403497652 -0.1910767340 0.2852410304;
  -1.2896500096 -0.2426174336 0.8424636019 0.6400938604;
  -1.2483124810 -1.1008485739 0.2379918258 -0.2191070538;
  -0.4035020278 -0.2213861915 -0.5230537909 0.0607766271;
  0.0814863555 0.1486319283 0.3489552987 -1.0159090284
];
b1 = [-0.6151099361 0.0112540423 1.0045780685 1.3466017224 0.6211184335 -0.2712713423 0.0555028938 0.4058392082]';

W2 = [
  -0.0000022617 0.2026156463 -0.5144191438 -1.2671030578 1.4529312320 1.5387307391 -0.4288535831 -1.0101162681;
  -0.0000000354 -1.4403962462 0.2108721898 1.1432925108 0.7994429305 -1.4548457402 -0.3528715059 0.8516974503;
  0.0000290091 0.9626264063 0.4070093195 0.7218008130 -0.9109704829 -1.1100008439 -0.5011236876 -0.5141296187
];
b2 = [-0.6455765667 0.7198390669 -1.4652109642]';

% -----------------------------
% READ LATEST SENSOR DATA
% -----------------------------
raw = thingSpeakRead( ...
    rawChannelID, ...
    'Fields', [1 2 3 4], ...
    'NumPoints', 1, ...
    'ReadKey', rawReadKey ...
);

if isempty(raw)
    fprintf('No raw sensor data available.\n');
    return;
end

if any(~isfinite(raw))
    fprintf('Raw sensor data contains invalid values.\n');
    return;
end

temperature = raw(1);
humidity = raw(2);
light = raw(3);
motion = raw(4);

x = [
    temperature
    humidity
    light
    motion
];

% -----------------------------
% NORMALIZATION
% -----------------------------
xN = (x - inputMean) ./ inputScale;

% -----------------------------
% HIDDEN LAYER + RELU
% -----------------------------
z1 = W1 * xN + b1;
a1 = max(0, z1);

% -----------------------------
% OUTPUT + SOFTMAX
% -----------------------------
z2 = W2 * a1 + b2;

% numerically stable softmax
e = exp(z2 - max(z2));
prob = e / sum(e);

normalProb = prob(1);
warningProb = prob(2);
criticalProb = prob(3);

[confidence, classIndex] = max(prob);

% MATLAB index 1..3 -> status 0..2
aiStatus = classIndex - 1;

% -----------------------------
% READ PREVIOUS STATUS
% before writing new status
% -----------------------------
previousStatus = -1;

try
    previousData = thingSpeakRead( ...
        aiChannelID, ...
        'Fields', 4, ...
        'NumPoints', 1, ...
        'ReadKey', aiReadKey ...
    );

    if ~isempty(previousData) && isfinite(previousData(end))
        previousStatus = previousData(end);
    end
catch
    previousStatus = -1;
end

% -----------------------------
% DISPLAY
% -----------------------------
statusText = 'NORMAL';

if aiStatus == 1
    statusText = 'WARNING';
elseif aiStatus == 2
    statusText = 'CRITICAL';
end

fprintf('Temperature : %.2f C\n', temperature);
fprintf('Humidity    : %.2f %%\n', humidity);
fprintf('Light ADC   : %.0f\n', light);
fprintf('Motion      : %.0f\n', motion);

fprintf('NORMAL      : %.4f\n', normalProb);
fprintf('WARNING     : %.4f\n', warningProb);
fprintf('CRITICAL    : %.4f\n', criticalProb);

fprintf('AI Status   : %s\n', statusText);
fprintf('Confidence  : %.2f %%\n', confidence * 100);

% -----------------------------
% WRITE AI RESULT
% -----------------------------
thingSpeakWrite( ...
    aiChannelID, ...
    'Fields', [1 2 3 4 5 6 7 8], ...
    'Values', [ ...
        normalProb, ...
        warningProb, ...
        criticalProb, ...
        aiStatus, ...
        confidence * 100, ...
        temperature, ...
        humidity, ...
        light ...
    ], ...
    'WriteKey', aiWriteKey ...
);

% -----------------------------
% EMAIL ONLY ON TRANSITION
% TO CRITICAL
% -----------------------------
if aiStatus == 2 && previousStatus ~= 2

    alertUrl = ...
        "https://api.thingspeak.com/alerts/send";

    options = weboptions( ...
        "HeaderFields", ...
        ["ThingSpeak-Alerts-API-Key", alertsApiKey] ...
    );

    alertSubject = ...
        "AIoT Neural Network - CRITICAL";

    alertBody = sprintf( ...
        [ ...
        "Cloud Neural Network detected CRITICAL condition.\n\n" ...
        "Critical Probability: %.2f %%\n" ...
        "Confidence: %.2f %%\n" ...
        "Temperature: %.2f C\n" ...
        "Humidity: %.2f %%\n" ...
        "Light: %.0f\n" ...
        "Motion: %.0f\n" ...
        ], ...
        criticalProb * 100, ...
        confidence * 100, ...
        temperature, ...
        humidity, ...
        light, ...
        motion ...
    );

    try
        webwrite( ...
            alertUrl, ...
            "body", alertBody, ...
            "subject", alertSubject, ...
            options ...
        );

        fprintf('EMAIL ALERT SENT\n');

    catch someException
        fprintf( ...
            'EMAIL ALERT ERROR: %s\n', ...
            someException.message ...
        );
    end
end
