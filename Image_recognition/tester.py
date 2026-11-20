from ultralytics import YOLO
import os

import cv2
import numpy as np

imgpath = str(os.path.relpath(__file__).replace("tester.py","0x2.png"))
 
image = cv2.imread(imgpath)

model = YOLO(str(os.path.relpath(__file__).replace("tester.py","runs/detect/train/weights/best.pt")))

results = model(source=imgpath, show=True, conf=0.25, save=True)

for r in results:
  for box in r.boxes:
    morality = int(box.cls)
    box = box.xyxy[0]
    print(box)
    box = ((int(box[0]),int(box[1])),(int(box[2]),int(box[3])))
    cv2.rectangle(image,box[0],box[1],(0,255*morality,255*((morality+1) %2)),5)


# print("Confidence values of all detected objects:")
# for r in results:
#   print(r.boxes.conf)

# print("Class values of all detected objects:")
# for r in results:
#   print(r.boxes.cls)

image = cv2.resize(image,(500,500))

cv2.imshow("Rectangle", image)
 
cv2.waitKey(0)
cv2.destroyAllWindows()
