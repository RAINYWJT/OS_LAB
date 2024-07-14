import numpy as np
import pandas as pd
import xgboost as xgb
from sklearn.model_selection import train_test_split
from sklearn.metrics import mean_squared_error
import json
import csv

with open('1.json', 'r') as file:
    data = json.load(file)

filename = "data.csv"
with open(filename, mode="w", newline="") as file:
    writer = csv.writer(file)
    writer.writerow(data.keys())  
    writer.writerows(zip(*data.values()))  

print(f"数据已保存到 {filename}")

'''
X = np.array([[1, 2], [2, 3], [3, 4], [4, 5]])
y = np.array([2, 4, 6, 8])

X_train, X_test, y_train, y_test = train_test_split(X, y, test_size=0.2, random_state=42)

model = xgb.XGBRegressor(objective="reg:squarederror", n_estimators=100, learning_rate=0.1)

model.fit(X_train, y_train)

y_pred = model.predict(X_test)

mse = mean_squared_error(y_test, y_pred)
print(f"MSE: {mse:.2f}")

new_data = np.array([[5, 6]])
predicted_value = model.predict(new_data)
print(f"Predict: {predicted_value[0]}")

model_filename = 'xgboost_model.json'
model.save_model(model_filename)
print(f"Saved as: {model_filename}")

# loaded_model = xgb.XGBRegressor()
# loaded_model.load_model(model_filename)
'''
