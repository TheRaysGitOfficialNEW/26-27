from ultralytics import YOLO
import os

model = YOLO("yolov8n.yaml")

model.train(data=str(os.path.relpath(__file__).replace("training.py","data/data.yaml")), epochs=100)