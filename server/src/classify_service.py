from ultralytics import YOLO
import cv2
class ClassifyService:
    MODEL_PATH = "../models/tan_model_v2.pt"
    def __init__(self, model_path=MODEL_PATH):
        self.model = YOLO(model_path)

        # debug
        print(f"LOADED MODEL FRO{model_path}")
        print(f"model classes: {self.model.names}")

    def reload_model(self, model_path=MODEL_PATH):
        self.model = YOLO(model_path)

    def classify(self, image, conf: float = 0.4, get_annotated_img=True):
        results = self.model(image, conf=conf)

        if not results or not results[0].boxes:
            return None

        best_box = max(
            results[0].boxes,
            key=lambda b: float(b.conf[0])
        )

        class_id = int(best_box.cls[0])

        if get_annotated_img:
            annotated_img = results[0].plot()
            return class_id, annotated_img
        return class_id
